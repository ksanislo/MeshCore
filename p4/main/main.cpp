// MeshCore integration phase 1: prove the mesh core + ed25519 crypto compile/link/run under IDF 5.4.1.
// Creates a random MeshCore Identity and prints its public key. Radio/mesh/advert come next.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "Identity.h"

extern "C" void app_main(void) {
  printf("\n=== T-Display-P4: MeshCore core smoke test ===\n");
  printf("PSRAM total: %u bytes\n", (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));

  mesh::LocalIdentity id;              // identity object (exercises ed25519/core link)
  char pub[80];
  for (int i = 0; i < PUB_KEY_SIZE; i++) sprintf(pub + i * 2, "%02x", id.pub_key[i]);
  printf("MeshCore identity pubkey: %s (PUB_KEY_SIZE=%d)\n", pub, PUB_KEY_SIZE);

  int n = 0;
  while (true) {
    printf("mesh-core alive %d  heap=%u\n", n++, (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
