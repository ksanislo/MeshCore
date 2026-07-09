/*
 * p4_ui_smoke.cpp — link-forcer for the shared LVGL UI (UITask) on P4, plus the
 * P4 board hooks the UI needs.
 *
 *  - board_set_backlight(): strong override of UITask's weak hook, forwarding to
 *    the esp_lcd panel brightness (DCS 0x51) via p4_display_set_brightness().
 *
 *  - p4_ui_smoke(): NEVER called in normal boot (main.cpp gates it behind a
 *    volatile-false flag). Referencing it forces UITask + its asset TUs into the
 *    final link so any undefined reference surfaces as a BUILD error rather than a
 *    silently-dropped archive member. It constructs a UITask and calls begin();
 *    the real app_main UI wiring is a later milestone (M4d-3).
 */
#include "target.h"          // P4Board `board`, SensorManager `sensors`
#include "UITask.h"          // the shared LVGL UI
#include <helpers/BaseSerialInterface.h>
#include "p4_display.h"      // p4_display_set_brightness()

// ---- Backlight: UITask's weak hook -> esp_lcd panel brightness --------------
extern "C" void board_set_backlight(uint8_t duty) {
  p4_display_set_brightness(duty);
}

// ---- Link/smoke forcer for the UI -------------------------------------------
// A minimal serial interface (never enabled) to satisfy the UITask ctor.
namespace {
class NullSerial : public BaseSerialInterface {
public:
  void enable() override {}
  void disable() override {}
  bool isEnabled() const override { return false; }
  bool isConnected() const override { return false; }
  bool isWriteBusy() const override { return false; }
  size_t writeFrame(const uint8_t* /*src*/, size_t /*len*/) override { return 0; }
  size_t checkRecvFrame(uint8_t* /*dest*/) override { return 0; }
};
}  // namespace

extern "C" void p4_ui_smoke(void) {
  static NullSerial serial;
  static UITask ui(&board, &serial);
  // Never actually run: begin() would init the panel/LVGL a second time. This
  // call exists only so the linker pulls in every UITask symbol.
  ui.begin(nullptr, &sensors, nullptr);
  ui.loop();
}
