#pragma once
#include <stdint.h>

// Fork-owned audio abstraction. The whole audio subsystem lives here in ui-lvgl/
// (NOT in upstream src/helpers/ui), so upstream MeshCore compiles as if there is
// no audio hardware and future upstream pulls never conflict with our work.
//
// Backend-selection macros, keyed on the board's *fork-owned* pin macros:
//   PIN_PIEZO    -> a passive piezo buzzer (tone()),     backend = PiezoSink
//   PIN_I2S_BCK  -> an I2S amplifier (MAX98357A/etc.),   backend = I2SBuzzer
// A board may define neither (no audio), one, or BOTH (runtime-selectable).
#if defined(PIN_PIEZO)
  #define HAS_PIEZO 1
#endif
#if defined(PIN_I2S_BCK)
  #define HAS_I2S 1
#endif
#if defined(HAS_PIEZO) || defined(HAS_I2S)
  #define HAS_BUZZER 1
#endif
#if defined(HAS_PIEZO) && defined(HAS_I2S)
  #define BUZZER_DUAL 1     // both present -> user picks at runtime (e.g. CrowPanel 3.5)
#endif

// Common interface for every audio backend. UITask holds an AudioSink* and never
// cares which concrete backend is behind it.
//
// CONTRACT (fixes the upstream buzzer's boot bug):
//  - begin() is SILENT: configure the pin/peripheral to a quiet idle state and
//    leave the sink muted (isQuiet() == true). It MUST NOT un-mute or auto-play.
//    The UI arms + plays the startup chime explicitly, on the UI core, only after
//    prefs/mute are resolved.
//  - sound is emitted from a single owned context (piezo: UI-core loop(); I2S: its
//    own core-0 task). The mesh backend must never reach in and play() cross-core.
class AudioSink {
public:
  virtual ~AudioSink() {}
  virtual void begin() = 0;
  virtual void play(const char* rtttl) = 0;
  virtual void loop() = 0;
  virtual void quiet(bool q) = 0;
  virtual bool isQuiet() = 0;
  virtual bool isPlaying() = 0;
  virtual void setVolume(uint8_t v) { (void)v; }   // no-op default (piezo has no volume)
};
