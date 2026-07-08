// Minimal USB-CDC sanity firmware for the T-Display-P4 bring-up. No MeshCore, no radio, no PSRAM
// dependency in code -- just prove the platform boots and the USB console enumerates + prints.
// Built by env LilyGo_TDisplay_P4_usbtest. Remove once P4 bring-up is done.
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(200);
}

void loop() {
  static uint32_t n = 0;
  Serial.printf("P4 alive %lu  (heap=%lu psram=%lu)\n",
                (unsigned long)n++,
                (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getFreePsram());
  delay(500);
}
