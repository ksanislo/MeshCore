#pragma once
#ifdef PIN_I2S_BCK

#include <Arduino.h>
#include <driver/i2s.h>

// I2S-based RTTTL ringtone synthesizer for boards with an I2S amplifier
// (e.g. LilyGo T-Deck: MAX98357A on BCK=7, WS=5, DOUT=6).
//
// Same public interface as genericBuzzer so UITask can use either backend
// behind the HAS_BUZZER / BUZZER_IS_I2S compile-time guards.
//
// RTTTL format: "Name:d=dur,o=oct,b=bpm:note,note,..."
// Notes: [dur]letter[#][oct][.]  e.g. "8e6", "4p", "16f#5."
// Strings are copied into an internal buffer so SD-loaded tunes don't need
// to stay resident after play() returns.

#define I2S_BUZZER_SAMPLE_RATE  8000
#define I2S_BUZZER_BUF_SAMPLES  64

class I2SBuzzer {
public:
    void    begin(i2s_port_t port = I2S_NUM_0);
    void    play(const char* rtttl);
    void    loop();
    void    startup();
    void    shutdown();
    bool    isPlaying();
    void    quiet(bool q);
    bool    isQuiet();
    void    setVolume(uint8_t vol);    // 0 = silent, 10 = loudest
    uint8_t getVolume() const { return _volume; }

    // Built-in ringtone catalogue
    static const char* const BUILTIN_NAMES[];   // null-terminated
    static const char* const BUILTIN_RTTTL[];   // parallel
    static int builtinCount();
    static const char* builtinByName(const char* name);  // nullptr if not found

private:
    i2s_port_t  _port        = I2S_NUM_0;
    bool        _initialized = false;
    bool        _is_quiet    = false;
    bool        _playing     = false;
    uint8_t     _volume      = 5;

    // Internal copy of the RTTTL string — callers need not keep it alive
    char        _rtttl_buf[512];

    // Parser state
    const char* _note_ptr  = nullptr;
    int         _def_dur   = 4;
    int         _def_oct   = 5;
    int         _bpm       = 120;

    // Synthesis state for the current note
    float       _note_freq  = 0.0f;   // 0 = rest
    uint32_t    _note_samps = 0;      // samples remaining this note
    uint32_t    _phase      = 0;      // square-wave phase accumulator

    bool parseHeader();
    bool parseNextNote();
    void writeChunk();

    static float noteFreq(int semitone, int octave);
};

#endif // PIN_I2S_BCK
