/*
 * p4_ui_stubs.cpp — inert P4/IDF implementations of the deferred UI subsystems.
 *
 * The shared LVGL UI (UITask) references three subsystems that are not yet ported
 * to the T-Display-P4 (they pull SdFat / LovyanGFX / WiFi on the S3 boards):
 *   - SdSvc + the `sd` handle  (SD is SDMMC on P4 -> deferred)
 *   - MapView / MapThumb       (LovyanGFX-based offline map -> deferred)
 *   - EmojiPack                (HTTPS emoji-pack downloader -> deferred, needs WiFi/C6)
 *
 * This TU provides exactly the symbols UITask links against, all as safe no-ops:
 * the card never mounts (SdSvc::ready()/ensureMounted() == false, so every guarded
 * SD access is skipped), the map renders nothing, and the emoji downloader does
 * nothing. Chat HISTORY and color emoji are therefore absent at runtime on P4;
 * live chat still flows from MeshProxy events + the RAM message ring, and the UI
 * (contacts / channels / chat / settings) builds and renders normally.
 */
#include "SdCard.h"        // pulls the P4 SdFat shim -> SdFs / FsFile / SdSvc API
#include "MapView.h"
#include "EmojiPack.h"
#include <string.h>

// ---- SdCard: the shared `sd` handle + last-mount diagnostics ----------------
SdFs sd;
volatile uint8_t sd_last_err_code = 0;
volatile uint8_t sd_last_err_data = 0;

namespace SdSvc {
  Lock::Lock() {}
  Lock::~Lock() {}
  bool begin()         { return false; }   // no card facility on P4 yet
  void registerFs()    {}                  // no 'S:' lv_fs driver (no SD-backed images)
  bool ready()         { return false; }
  void end()           {}
  bool ensureMounted() { return false; }   // never mounts -> all SD paths skipped
  void rescan()        {}
  void pollPresence()  {}
  void emojiBitmapCacheEvict() {}
  void emojiBitmapCacheStats(uint32_t* glyphs, uint32_t* bytes) {
    if (glyphs) *glyphs = 0;
    if (bytes)  *bytes  = 0;
  }
}  // namespace SdSvc

// ---- MapView / MapThumb: no-op (offline map deferred on P4) ------------------
void MapView::build(lv_obj_t* /*parent*/, uint16_t /*w*/, uint16_t /*h*/,
                    MarkerTapCb /*cb*/, void* /*user*/) {}
void MapView::destroy() {}
void MapView::onShow() {}
void MapView::onHide() {}
bool MapView::service(uint32_t /*now_ms*/) { return false; }
void MapView::markContactsDirty() {}
void MapView::panBy(int /*dx*/, int /*dy*/) {}
void MapView::selfTest() {}

void MapThumb::create(lv_obj_t* /*parent*/, int side) { _side = side; }
void MapThumb::render(double /*lat*/, double /*lon*/) {}
void MapThumb::hide() {}

// ---- EmojiPack: no-op (HTTPS downloader deferred; needs WiFi/C6 at M5) -------
namespace EmojiPack {
  void start()  {}
  void cancel() {}
  bool busy()   { return false; }
  bool fetching() { return false; }
  int  progress() { return 0; }
  void status(char* out, size_t cap) { if (out && cap) out[0] = 0; }
}  // namespace EmojiPack
