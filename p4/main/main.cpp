// Simplified boot proof for OUR p4/ build env (pio IDF). The fix vs every earlier silent attempt:
// sdkconfig now sets CONFIG_ESP32P4_REV_MIN_100 (rev v1.0 = our chip's silicon), and the console is
// UART_DEFAULT -> the board's CH340 USB-UART bridge (the OTHER USB-C port). Read at 115200.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"

extern "C" void app_main(void) {
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  printf("\n=== T-Display-P4 boot: OUR pio build env ===\n");
  printf("chip: cores=%d rev=%d\n", chip.cores, chip.revision);
  size_t psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  printf("PSRAM total: %u bytes (%.1f MB)\n", (unsigned)psram, psram / 1048576.0);
  int n = 0;
  while (true) {
    printf("P4 alive %d  internal_free=%u  psram_free=%u\n", n++,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
