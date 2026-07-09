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
    printf("\n=== T-Display-P4: MeshCore radio bring-up (M3.5) ===\n");
    printf("PSRAM total: %u bytes\n",
           (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

    board_power_up();
    if (!board_radio_begin()) {
        printf("radio bring-up failed; halting.\n");
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    meck_radio_attach();

    // Bring up the MeshCore node (identity + mesh + advert on its own task).
    p4_node_start();

    // app_main can idle; the mesh runs on mesh_task.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("[main] alive  heap=%u\n",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    }
}
