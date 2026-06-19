#include "PiezoSink.h"
#ifdef HAS_PIEZO

#include <Arduino.h>

void PiezoSink::begin() {
    pinMode(PIN_PIEZO, OUTPUT);
    digitalWrite(PIN_PIEZO, LOW);
    // SILENT init: do NOT un-mute and do NOT auto-play. _is_quiet stays true until
    // the UI explicitly arms us (quiet(false)) after prefs/mute are resolved.
}

void PiezoSink::stop() {
    if (_playing) {
        noTone(PIN_PIEZO);
        digitalWrite(PIN_PIEZO, LOW);
        _playing = false;
    }
}

void PiezoSink::play(const char* rtttl) {
    stop();
    if (!rtttl || !*rtttl || _is_quiet) return;
    if (!_reader.begin(rtttl)) return;
    _playing = true;
    advance();   // start the first note immediately; loop() advances the rest
}

bool PiezoSink::advance() {
    float freq, durSec;
    if (!_reader.nextNote(freq, durSec)) { stop(); return false; }
    uint32_t durMs = (uint32_t)(durSec * 1000.0f);
    if (!durMs) durMs = 1;
    if (freq < 1.0f) noTone(PIN_PIEZO);                       // rest
    // No duration: the note sustains until the NEXT note's tone() replaces it, so
    // transitions are gapless even if loop() is a little late (a late loop() only
    // stretches a note, never leaves a silent gap). The boot stuck-note case is
    // handled by deferring the startup chime until loop() is already cycling.
    else             tone(PIN_PIEZO, (unsigned int)(freq + 0.5f));
    _note_end_ms = millis() + durMs;
    return true;
}

void PiezoSink::loop() {
    if (!_playing) return;
    if ((int32_t)(millis() - _note_end_ms) >= 0) advance();   // next note, or stop at end
}

void PiezoSink::quiet(bool q) {
    _is_quiet = q;
    if (q) stop();
}

#endif // HAS_PIEZO
