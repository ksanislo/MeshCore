// Minimal boot proof. The only change from the earlier silent attempt is the sdkconfig now sets
// CONFIG_IDF_EXPERIMENTAL_FEATURES=y (required to unlock 200MHz HEX PSRAM). If this prints over the
// USB-Serial/JTAG console and psram_total reads ~32MB, the board boots and that flag was the blocker.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"

extern "C" void app_main(void) {
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  printf("\n=== T-Display-P4 boot (cores=%d rev=%d) ===\n", chip.cores, chip.revision);
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
