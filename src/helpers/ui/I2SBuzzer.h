#pragma once
#ifdef PIN_I2S_BCK

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// I2S-based RTTTL ringtone synthesizer for boards with an I2S amplifier
// (e.g. LilyGo T-Deck: MAX98357A on BCK=7, WS=5, DOUT=6).
//
// Architecture: play() pre-renders the entire RTTTL song into a PSRAM buffer,
// then launches a small FreeRTOS task on core 0 that streams it to the I2S DMA
// with portMAX_DELAY writes. Audio is completely decoupled from the LVGL loop
// so frame-rate spikes can never cause dropouts.

#define I2S_BUZZER_SAMPLE_RATE  44100   // At 22050 Hz, E5 (659 Hz) quantizes to 689 Hz (78 cents
                                        // sharp — sounds like a different note). At 44100 Hz the
                                        // worst-case error drops to ~25 cents and the E→B interval
                                        // ratio is preserved to within 1 cent.
#define I2S_BUZZER_BUF_SAMPLES  96000   // legacy constant; buffer is now allocated per-song

class I2SBuzzer {
public:
    void    begin(i2s_port_t port = I2S_NUM_0);
    void    play(const char* rtttl);
    void    loop();          // call from UITask::loop(); only handles deferred startup
    void    startup();
    void    shutdown();
    bool    isPlaying();
    void    quiet(bool q);
    bool    isQuiet();
    void    setVolume(uint8_t vol);
    uint8_t getVolume() const { return _volume; }

    static const char* const BUILTIN_NAMES[];
    static const char* const BUILTIN_RTTTL[];
    static int builtinCount();
    static const char* builtinByName(const char* name);

private:
    i2s_port_t   _port          = I2S_NUM_0;
    bool         _initialized   = false;
    bool         _is_quiet      = false;
    volatile bool _playing      = false;
    bool         _startup_pending = false;
    uint8_t      _volume        = 5;

    // Pre-render buffer (allocated per-song in play())
    int16_t*     _audio_buf     = nullptr;
    size_t       _buf_cap       = 0;      // actual allocation size in samples
    volatile size_t _audio_len  = 0;

    // Background playback task
    TaskHandle_t _play_task     = nullptr;

    // RTTTL copy + parser state (used during pre-render in play())
    char         _rtttl_buf[512];
    char         _startup_buf[64];
    const char*  _note_ptr  = nullptr;
    int          _def_dur   = 4;
    int          _def_oct   = 5;
    int          _bpm       = 120;
    float        _note_freq = 0.0f;
    uint32_t     _note_samps = 0;

    bool parseHeader();
    bool parseNextNote();
    void stopTask();            // stop + join the background task

    static void   audioPlayTask(void* arg);
    static float  noteFreq(int semitone, int octave);
};

#endif // PIN_I2S_BCK
