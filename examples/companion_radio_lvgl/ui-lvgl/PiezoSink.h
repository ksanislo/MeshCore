#pragma once
#include "AudioSink.h"
#ifdef HAS_PIEZO

#include "Rtttl.h"

// Passive-piezo backend: drives PIN_PIEZO with tone()/noTone(), stepping one note
// per loop() tick on the UI core (a fork-owned replacement for the upstream
// genericBuzzer + NonBlockingRtttl, but with a SILENT begin() — no boot-time arm).
class PiezoSink : public AudioSink {
public:
    void begin() override;
    void play(const char* rtttl) override;
    void loop() override;
    void quiet(bool q) override;
    bool isQuiet() override   { return _is_quiet; }
    bool isPlaying() override { return _playing; }
    // setVolume: a passive buzzer has no volume control -> AudioSink no-op default.

private:
    RtttlReader _reader;
    bool        _is_quiet    = true;   // silent until the UI arms us
    bool        _playing     = false;
    uint32_t    _note_end_ms = 0;      // millis() when the current note ends

    void stop();
    bool advance();   // pull next note, drive tone(); false at end of song
};

#endif // HAS_PIEZO
