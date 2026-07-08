// M2 boot proof: bring up TinyUSB CDC on the USB-OTG (the plugged USB-C port) exactly like the
// Meck-P4 reference, then redirect stdout to it so printf reaches a console. If "P4 alive" prints
// and psram_total reads ~32MB, the HEX-PSRAM boot + the board's real USB path are both proven.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "driver/i2c.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"

// --- XL9535 (TCA9535-class) I2C GPIO expander at 0x20 on I2C-1 (SDA=7, SCL=8) -----------------
// On the T-Display-P4 the power rails (and SX1262 RST/DIO1, RF switch, resets) hang off this
// expander; nothing responds until it enables the rails. Reg map: 0x02/0x03 output, 0x06/0x07 dir
// (0=output). Rails (from Meck-P4's radio example): IO6 5V=HIGH, IO0 3.3V=LOW(active-low), IO10 VCCA=HIGH.
#define XL9535_ADDR 0x20
static void xl_write(uint8_t reg, uint8_t val) {
  uint8_t buf[2] = { reg, val };
  i2c_master_write_to_device(I2C_NUM_0, XL9535_ADDR, buf, 2, pdMS_TO_TICKS(100));
}
static void board_power_up(void) {
  i2c_config_t c = {};
  c.mode = I2C_MODE_MASTER;
  c.sda_io_num = 7; c.scl_io_num = 8;
  c.sda_pullup_en = GPIO_PULLUP_ENABLE; c.scl_pullup_en = GPIO_PULLUP_ENABLE;
  c.master.clk_speed = 400000;
  i2c_param_config(I2C_NUM_0, &c);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
  // output latches first (avoid glitch), then set those pins as outputs.
  xl_write(0x02, 0x40);   // port0 out: IO6=1 (5V on),  IO0=0 (3.3V on, active-low)
  xl_write(0x03, 0x04);   // port1 out: IO10=1 (P4 VCCA on)
  xl_write(0x06, 0xBE);   // port0 dir: IO0 & IO6 = outputs (0), rest inputs
  xl_write(0x07, 0xFB);   // port1 dir: IO10 = output (0), rest inputs
  vTaskDelay(pdMS_TO_TICKS(150));   // let the rails settle before USB PHY / peripherals
}

extern "C" void app_main(void) {
  board_power_up();

  const tinyusb_config_t tusb_cfg = {
    .device_descriptor = NULL,
    .string_descriptor = NULL,
    .external_phy = false,          // P4 uses the internal HS USB PHY
#if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = NULL,
    .hs_configuration_descriptor = NULL,
    .qualifier_descriptor = NULL,
#else
    .configuration_descriptor = NULL,
#endif
  };
  tinyusb_driver_install(&tusb_cfg);

  tinyusb_config_cdcacm_t acm_cfg = {
    .usb_dev = TINYUSB_USBDEV_0,
    .cdc_port = TINYUSB_CDC_ACM_0,
    .rx_unread_buf_sz = 64,
    .callback_rx = NULL,
    .callback_rx_wanted_char = NULL,
    .callback_line_state_changed = NULL,
    .callback_line_coding_changed = NULL,
  };
  tusb_cdc_acm_init(&acm_cfg);
  esp_tusb_init_console(TINYUSB_CDC_ACM_0);   // stdout -> USB CDC

  size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  int n = 0;
  while (true) {
    printf("P4 alive %d  internal_free=%u  psram_total=%u  psram_free=%u\n", n++,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)psram_total,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
