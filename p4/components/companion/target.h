/*
 * target.h — P4/IDF companion "board" target for the shared MyMesh backend.
 *
 * The shared companion backend (examples/companion_radio/MyMesh.{h,cpp}) includes
 * <target.h> and references a handful of variant-provided globals: `board`
 * (mesh::MainBoard), `sensors` (SensorManager), `radio_driver` (the concrete
 * mesh::Radio with the RadioLib-style accessors MyMesh calls), plus the
 * radio_init()/radio_new_identity() hooks. On the S3 side each variant supplies
 * these; here we provide a minimal P4 equivalent. Definitions live in
 * p4_target.cpp.
 *
 * Connectivity (WiFi/BLE/MQTT) is compiled OFF for this milestone, so this
 * target is deliberately small: no serial-interface/OTA/preset plumbing.
 */
#pragma once

#include <MeshCore.h>
#include <Mesh.h>
#include <helpers/SensorManager.h>
#include "P4SX1262Radio.h"   // concrete radio type MyMesh's `radio_driver` refers to

// Minimal board for the T-Display P4. Battery sense (BQ27220) lands at M6; for now
// getBattMilliVolts() returns 0 (== "not supported", same as the base ESP32 board).
class P4Board : public mesh::MainBoard {
  uint8_t _startup_reason = BD_STARTUP_NORMAL;
public:
  void begin() { _startup_reason = BD_STARTUP_NORMAL; }
  uint16_t getBattMilliVolts() override { return 0; }          // M6: BQ27220 fuel gauge
  const char* getManufacturerName() const override { return "LilyGo T-Display P4"; }
  void reboot() override { esp_restart(); }
  uint8_t getStartupReason() const override { return _startup_reason; }
};

extern P4Board          board;
extern SensorManager    sensors;        // base manager (no env sensors on P4 yet)
extern P4SX1262Radio    radio_driver;   // defined in p4_radio.cpp

// Backend hooks (mirror the S3 variant target surface used by MyMesh).
bool radio_init();
mesh::LocalIdentity radio_new_identity();
