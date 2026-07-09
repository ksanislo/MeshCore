/*
 * p4_radio.h — safe-to-include-from-main interface to the SX1262 radio adapter.
 *
 * main.cpp DEFINES the SX1262/XL9535 hardware globals and must NOT include
 * P4SX1262Radio.h (it would conflict with those definitions). It includes this
 * header instead: no SX1262 extern, no cpp_bus_driver types leak here.
 */
#pragma once
#include <stdint.h>

// LoRa bring-up defaults. Once MyMesh loads NodePrefs it re-applies the saved
// preset via radio_set_params(), so these only matter for the very first attach.
#define LORA_FREQ_DEFAULT     915.0f
#define LORA_BW_DEFAULT       250.0f
#define LORA_SF_DEFAULT       10
#define LORA_CR_DEFAULT       5
#define LORA_TX_POWER_DEFAULT 22

#ifdef __cplusplus
extern "C" {
#endif

// Apply MeshCore's LoRa preset to the already-begun SX1262 and enter RX.
// Call once, after the board bring-up has done SX1262->begin().
bool     meck_radio_attach(void);
void     radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void     radio_set_tx_power(uint8_t dbm);
uint32_t radio_get_rng_seed(void);       // ESP32-P4 hardware TRNG
void     meck_set_antenna(uint8_t external);

#ifdef __cplusplus
}

namespace mesh { class Radio; }
// C++-only accessor to the mesh::Radio singleton (feed this to MyMesh).
mesh::Radio& p4_get_radio();
#endif
