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
#include "UITask.h"                   // the shared LVGL UI (AbstractUITask for MyMesh)
#include <helpers/BaseSerialInterface.h>
#include "FS.h"                       // fs::InternalFS, fs_mount_spiffs()
#include <helpers/ArduinoHelpers.h>   // StdRNG, VolatileRTCClock
#include <helpers/SimpleMeshTables.h>
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

// ---- Variant globals referenced by the shared backend -----------------------
P4Board        board;
SensorManager  sensors;              // base: no environment sensors on P4 yet

// Companion transport. No BLE/WiFi on P4 yet (C6 co-processor is M5) — a null
// serial interface satisfies the UITask/MyMesh dependency; the phone companion
// comes online at M5.
namespace {
class NullSerial : public BaseSerialInterface {
public:
  void   enable() override {}
  void   disable() override {}
  bool   isEnabled() const override { return false; }
  bool   isConnected() const override { return false; }
  bool   isWriteBusy() const override { return false; }
  size_t writeFrame(const uint8_t*, size_t) override { return 0; }
  size_t checkRecvFrame(uint8_t*) override { return 0; }
};
}  // namespace
static NullSerial serial_interface;

// The UI task. Constructed BEFORE the_mesh so it can be handed in as the mesh's
// AbstractUITask — MyMesh derefs _ui (unguarded, e.g. newMsg) so it must be
// non-NULL; in MESH_PROXY mode those callbacks cook+enqueue to MeshProxy from
// core 0 and UITask::drainEvents() applies them on core 1.
UITask ui_task(&board, &serial_interface);

// Collaborators for MyMesh (mirror examples/companion_radio/main.cpp).
StdRNG            fast_rng;
VolatileRTCClock  rtc_clock;         // soft clock; battery RTC lands at M6
SimpleMeshTables  tables;
DataStore         store(fs::InternalFS, rtc_clock);

// The shared companion backend singleton (declared `extern` in MyMesh.h).
MyMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store, &ui_task);

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

// ---- The live companion: backend on core 0, UI on core 1 --------------------
static volatile bool s_ui_ready = false;

// Backend loop, pinned to core 0 (mirrors examples/companion_radio/main.cpp).
static void meshTask(void*) {
  while (!s_ui_ready) vTaskDelay(1);   // let the UI finish init first
  for (;;) {
    if (mproxy::radioPauseRequested()) { mproxy::setRadioIdle(true); vTaskDelay(2); continue; }
    mproxy::setRadioIdle(false);
    mproxy::drainCommands(the_mesh);                 // UI-posted commands -> the_mesh
    if (!the_mesh.getNodePrefs()->radio_off) the_mesh.loop();
    mproxy::publishIfChanged(the_mesh);              // republish snapshot on change
    mproxy::updateStats(the_mesh);                   // live counters
    vTaskDelay(1);
  }
}

// LVGL + UITask, pinned to core 1. begin() brings up LVGL over esp_lcd
// (p4_display_lvgl_begin via the UI_DISPLAY_ESP_LCD seam) and builds the UI.
static void uiTask(void*) {
  ui_task.begin(nullptr, &sensors, the_mesh.getNodePrefs());
  s_ui_ready = true;                                 // release the core-0 backend
  for (;;) {
    ui_task.loop();
    rtc_clock.tick();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Called from app_main AFTER the hardware is up (rails, panel init, touch, radio
// attach). Seeds the backend + snapshot, then spawns the two cores' tasks.
extern "C" void p4_app_run(void) {
  fast_rng.begin(esp_random());
  store.begin();
  the_mesh.begin(false);

  // Attach the (null) companion transport so MyMesh::checkSerialInterface has a
  // valid _serial. The real BLE/WiFi companion arrives with the C6 at M5.
  the_mesh.startInterface(serial_interface);

  if (!mproxy::init()) printf("[app] MeshProxy init failed\n");
  mproxy::setBackend(the_mesh);
  mproxy::publishIfChanged(the_mesh);   // seed snapshot before the UI reads it

  // Reserve the backend task stack before the UI allocates (S3 heap-order habit).
  xTaskCreatePinnedToCore(meshTask, "mesh", 16384, nullptr, 1, nullptr, 0);   // core 0
  xTaskCreatePinnedToCore(uiTask,   "ui",   32768, nullptr, 2, nullptr, 1);   // core 1
  printf("[app] companion running: backend core0, UI core1\n");
}

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
