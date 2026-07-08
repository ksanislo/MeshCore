#ifndef DEFAULT_CPP_BUS_DRIVER_VALUE
#define DEFAULT_CPP_BUS_DRIVER_VALUE (-1)
#endif

/*
 * arduino_compat.h — Arduino API extras for MeshCore on ESP-IDF T-Display P4
 *
 * The arduino_cpp_bus_driver component (transitive dep via private_library)
 * already provides: millis(), delay(), delayMicroseconds(), Serial, String.
 * We do NOT redefine those here.
 *
 * This file adds only what arduino_cpp_bus_driver doesn't provide:
 *   - BOARD_HAS_PSRAM + ps_calloc/ps_malloc (for BaseChatMesh contact arrays)
 *   - ESPClass (ESP.getFreeHeap(), ESP.getMaxAllocHeap())
 *   - micros()
 *   - yield()
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- micros (not provided by arduino_cpp_bus_driver) ----
static inline unsigned long micros(void) {
    return (unsigned long)esp_timer_get_time();
}

// ---- PSRAM allocation (ESP32-P4 has 32MB PSRAM) ----
#define BOARD_HAS_PSRAM  1

static inline void* ps_calloc(size_t n, size_t sz) {
    return heap_caps_calloc(n, sz, MALLOC_CAP_SPIRAM);
}

static inline void* ps_malloc(size_t sz) {
    return heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
}

// ---- Yield ----
static inline void yield(void) {
    taskYIELD();
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// ESP class stub — MeshCore uses ESP.getFreeHeap(), ESP.getMaxAllocHeap()
class ESPClass {
public:
    uint32_t getFreeHeap() { return esp_get_free_heap_size(); }
    uint32_t getMaxAllocHeap() { return heap_caps_get_largest_free_block(MALLOC_CAP_8BIT); }
    void restart() { esp_restart(); }
};

extern ESPClass ESP;

// ltoa helper
static inline char* ltoa(long value, char* str, int base) {
    if (base == 10) sprintf(str, "%ld", value);
    else if (base == 16) sprintf(str, "%lx", value);
    return str;
}

#endif // __cplusplus