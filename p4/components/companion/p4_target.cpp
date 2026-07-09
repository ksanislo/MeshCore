/*
 * p4_target.cpp — P4/IDF companion backend globals + radio glue.
 *
 * Defines the variant globals the shared MyMesh backend expects (`board`,
 * `sensors`, `radio_driver`, the `the_mesh`/`store` singletons and their
 * collaborators) and the C++-linkage radio_* entry points MeshProxy calls. The
 * concrete SX1262 hardware work lives in the meshcore component's p4_radio.cpp;
 * here we only forward to it (via extern-C p4hw_* shims) so the C++/extern-C
 * linkage of the two sides doesn't clash.
 *
 * Connectivity is OFF for this milestone (no WiFi/BLE/MQTT); this file also
 * provides p4_backend_smoke() — never called in normal boot, referenced from
 * main.cpp only to force the backend objects into the final link so undefined
 * references surface at build time.
 */
#include "target.h"
#include "MyMesh.h"
#include "MeshProxy.h"
#include "FS.h"                       // fs::InternalFS
#include <helpers/ArduinoHelpers.h>   // StdRNG, VolatileRTCClock
#include <helpers/SimpleMeshTables.h>
#include "esp_random.h"
#include <stdio.h>

// ---- Variant globals referenced by the shared backend -----------------------
P4Board        board;
SensorManager  sensors;              // base: no environment sensors on P4 yet

// Collaborators for MyMesh (mirror examples/companion_radio/main.cpp).
StdRNG            fast_rng;
VolatileRTCClock  rtc_clock;         // soft clock; battery RTC lands at M6
SimpleMeshTables  tables;
DataStore         store(fs::InternalFS, rtc_clock);

// The shared companion backend singleton (declared `extern` in MyMesh.h). No UI
// task on P4 yet (UITask is a later stage), so the AbstractUITask* arg is NULL.
MyMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store);

// ---- Backend target hooks ---------------------------------------------------
bool radio_init() {
  // The SX1262 is brought up in main.cpp (board_radio_begin + meck_radio_attach)
  // before the backend runs, so there's nothing to do here on P4.
  return true;
}

mesh::LocalIdentity radio_new_identity() {
  return mesh::LocalIdentity(&fast_rng);
}

// ---- Radio controls MeshProxy calls (C++ linkage) ---------------------------
// Forward to the extern-C hardware shims in p4_radio.cpp. Forward-declared here
// (not via p4_radio.h) so p4_radio.h's own extern-C radio_set_params/tx_power
// declarations don't clash with these C++-linkage definitions.
extern "C" void p4hw_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
extern "C" void p4hw_set_tx_power(uint8_t dbm);
extern "C" void p4hw_radio_standby(void);
extern "C" void p4hw_radio_sleep(void);

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  p4hw_set_params(freq, bw, sf, cr);
}
void radio_set_tx_power(int8_t dbm) {
  p4hw_set_tx_power((uint8_t)(dbm < 0 ? 0 : dbm));
}
void radio_sleep()   { p4hw_radio_sleep(); }
void radio_standby() { p4hw_radio_standby(); }

// ---- Link/smoke forcer ------------------------------------------------------
// NEVER called in normal boot (see main.cpp's volatile guard). Touches MyMesh,
// DataStore and MeshProxy enough to force every backend object into the final
// link so undefined references become build errors rather than silently-dropped
// archive members.
extern "C" void p4_backend_smoke(void) {
  board.begin();
  fast_rng.begin(esp_random());
  store.begin();
  the_mesh.begin(false);
  mproxy::init();
  mproxy::setBackend(the_mesh);
  mproxy::publishIfChanged(the_mesh);
  mproxy::drainCommands(the_mesh);
  mproxy::updateStats(the_mesh);
  printf("[smoke] backend name=%s\n", the_mesh.getNodeName());
}
