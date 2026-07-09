/*
 * T-Display P4 — MeshCore radio bring-up checkpoint (M3.5).
 *
 * Constructs the board hardware globals (I2C expander + SX1262 on SPI2) exactly
 * as LilyGo's firmware does, brings up the power rails, resets + begins the
 * SX1262 via cpp_bus_driver, then attaches MeshCore's radio adapter and runs a
 * simple RX/TX self-test. This re-proves the radio through the cpp_bus_driver
 * path the mesh node will use (the earlier M3 proof used LilyGo's RadioLib demo).
 *
 * NOTE: main.cpp DEFINES the SX1262/XL9535 globals; the radio adapter in the
 * meshcore component references them via extern. Do not include P4SX1262Radio.h
 * here (it would double-declare SX1262).
 */
#include <stdio.h>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "cpp_bus_driver_library.h"
#include "t_display_p4_config.h"
#include "p4_radio.h"
#include "p4_node.h"
#include "p4_display.h"     // p4_display_init(), p4_display_selftest()
#include "board_touch.h"    // board_touch_init(), board_touch_read()
#include "FS.h"             // fs_mount_spiffs()

// Boot mode:
//   0 = headless mesh node (p4_node, no UI)
//   1 = display/LVGL self-test (no radio/backend)
//   2 = full companion GUI (backend + UITask) — the real firmware
#define P4_APP_MODE 2

// The full companion entry point (companion component). Brings up the backend on
// core 0 + UITask on core 1 after the hardware is ready.
extern "C" void p4_app_run(void);

// M4d: the shared companion BACKEND (MyMesh + DataStore + MeshProxy) is compiled
// + linked but not yet driven (UITask lands later). p4_backend_smoke() is defined
// in the companion component; referencing it here (behind a volatile guard so the
// call is never elided AND never actually runs) forces the backend objects into
// the final link so undefined references surface at build time. Leave at 0.
extern "C" void p4_backend_smoke(void);
static volatile bool s_run_backend_smoke = false;

// M4d (UI): same link-forcer for the shared LVGL UI (UITask + asset TUs). Never
// runs (volatile-false); referencing it makes every UITask symbol a build-time
// link requirement. The real app_main UI wiring lands in M4d-3.
extern "C" void p4_ui_smoke(void);
static volatile bool s_run_ui_smoke = false;

// ---- Board hardware globals (external linkage; referenced by p4_radio.cpp) ----
// IIC-1 bus (SDA7/SCL8) carries the XL9535 expander (and later touch/RTC/gauge).
auto XL9535_IIC_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(
    XL9535_SDA, XL9535_SCL, I2C_NUM_0);
std::unique_ptr<Cpp_Bus_Driver::Xl95x5> XL9535 =
    std::make_unique<Cpp_Bus_Driver::Xl95x5>(
        XL9535_IIC_Bus, XL9535_IIC_ADDRESS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// SPI2 dedicated to the SX1262 (SCLK2/MOSI3/MISO4).
auto SX1262_SPI_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(
    SX1262_MOSI, SX1262_SCLK, SX1262_MISO, SPI2_HOST, 0);
std::unique_ptr<Cpp_Bus_Driver::Sx126x> SX1262 =
    std::make_unique<Cpp_Bus_Driver::Sx126x>(
        SX1262_SPI_Bus, Cpp_Bus_Driver::Sx126x::Chip_Type::SX1262,
        SX1262_BUSY, SX1262_CS, DEFAULT_CPP_BUS_DRIVER_VALUE);

// GT9895 capacitive touch on IIC-1 (shares the bus with XL9535). The scale
// factors map the raw 1060x2400 grid onto the 568x1232 panel.
auto GT9895_Bus = std::make_shared<Cpp_Bus_Driver::Hardware_Iic_1>(
    GT9895_TOUCH_SDA, GT9895_TOUCH_SCL, I2C_NUM_0);
std::unique_ptr<Cpp_Bus_Driver::Gt9895> GT9895 =
    std::make_unique<Cpp_Bus_Driver::Gt9895>(
        GT9895_Bus, GT9895_IIC_ADDRESS, GT9895_X_SCALE_FACTOR, GT9895_Y_SCALE_FACTOR,
        DEFAULT_CPP_BUS_DRIVER_VALUE);

// ---- Touch: reset + share XL9535's I2C bus handle + begin --------------------
extern "C" void board_touch_init(void) {
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(50));
    XL9535->pin_write(XL9535_TOUCH_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
    // GT9895 shares XL9535's already-initialised IIC-1 bus.
    GT9895_Bus->set_bus_handle(XL9535_IIC_Bus->get_bus_handle());
    printf("[touch] GT9895 begin %s\n", GT9895->begin() ? "success" : "FAIL");
}

extern "C" bool board_touch_read(int16_t *x, int16_t *y) {
    if (!GT9895) return false;
    Cpp_Bus_Driver::Gt9895::Touch_Point tp;
    if (GT9895->get_single_touch_point(tp)) {
        if (x) *x = (int16_t)tp.info[0].x;
        if (y) *y = (int16_t)tp.info[0].y;
        return true;
    }
    return false;
}

// ---- Power rails + peripheral reset sequence (from LilyGo's app_main) --------
static void board_power_up(void) {
    XL9535->begin();

    XL9535->pin_mode(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_TOUCH_RST,  Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_write(XL9535_TOUCH_RST,  Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_mode(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_5_0_V_POWER_EN,        Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_3_3_V_POWER_EN,        Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_mode(XL9535_GPS_WAKE_UP,           Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_GPS_WAKE_UP,          Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_mode(XL9535_ESP32C6_EN,            Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_ESP32C6_EN,           Cpp_Bus_Driver::Xl95x5::Value::LOW);

    XL9535->pin_write(XL9535_ESP32P4_VCCA_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);

    // 5V/3V3 rail power-sequencing dance (200 ms steps), per LilyGo.
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_5_0_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    XL9535->pin_write(XL9535_3_3_V_POWER_EN, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
}

// Pulse the panel hardware reset (XL9535 IO2). Passed to p4_display_init so it
// fires in the reference order (after the DPHY LDO, before DSI/panel create).
extern "C" void board_screen_reset_pulse(void) {
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    XL9535->pin_write(XL9535_SCREEN_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
}

// ---- SX1262 reset + begin over cpp_bus_driver --------------------------------
static bool board_radio_begin(void) {
    // DIO1 (via expander) as input; RST pulse via expander IO16.
    XL9535->pin_mode(XL9535_SX1262_DIO1, Cpp_Bus_Driver::Xl95x5::Mode::INPUT);
    XL9535->pin_mode(XL9535_SX1262_RST,  Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::LOW);
    vTaskDelay(pdMS_TO_TICKS(10));
    XL9535->pin_write(XL9535_SX1262_RST, Cpp_Bus_Driver::Xl95x5::Value::HIGH);
    vTaskDelay(pdMS_TO_TICKS(10));

    // SKY13453 RF switch -> onboard antenna.
    XL9535->pin_mode(XL9535_SKY13453_VCTL, Cpp_Bus_Driver::Xl95x5::Mode::OUTPUT);
    meck_set_antenna(0);

    if (!SX1262->begin(10000000)) {
        printf("sx1262 begin FAIL\n");
        return false;
    }
    printf("sx1262 begin success\n");
    return true;
}

extern "C" void app_main(void) {
    // Never true — forces the companion backend into the link (see declaration above).
    if (s_run_backend_smoke) p4_backend_smoke();
    if (s_run_ui_smoke) p4_ui_smoke();

    printf("\n=== T-Display-P4: MeshCore radio bring-up (M3.5) ===\n");
    printf("PSRAM total: %u bytes\n",
           (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

    // Mount storage FIRST, before the radio is powered — the one-time SPIFFS
    // format is a long flash erase; doing it with the radio drawing current can
    // brown the board out. (Idempotent from the node's perspective.)
    printf("[main] mounting storage...\n");
    if (!fs_mount_spiffs()) {
        printf("[main] SPIFFS mount FAILED (identity will not persist)\n");
    } else {
        printf("[main] SPIFFS mounted\n");
    }

    board_power_up();

#if P4_APP_MODE == 1
    // Display-only proof. p4_display_init pulses XL9535_SCREEN_RST (via this
    // callback) in the reference order: LDO up -> settle -> reset -> panel.
    printf("[main] display self-test...\n");
    if (!p4_display_init(board_screen_reset_pulse)) {
        printf("[main] p4_display_init FAILED; halting.\n");
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    board_touch_init();
    p4_display_selftest();

#elif P4_APP_MODE == 0
    if (!board_radio_begin()) {
        printf("radio bring-up failed; halting.\n");
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    meck_radio_attach();
    p4_node_start();

#else  // P4_APP_MODE == 2: full companion GUI
    // Panel + touch first (UITask.begin -> p4_display_lvgl_begin needs the panel
    // up), then the radio, then hand off to the companion (backend + UI tasks).
    printf("[main] bringing up display + touch + radio...\n");
    if (!p4_display_init(board_screen_reset_pulse)) {
        printf("[main] p4_display_init FAILED; halting.\n");
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    board_touch_init();
    if (!board_radio_begin()) {
        printf("[main] radio bring-up failed; continuing (UI still usable)\n");
    } else {
        meck_radio_attach();
    }
    p4_app_run();
#endif

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("[main] alive  heap=%u\n",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}
