#pragma once
#include "AudioSink.h"
#ifdef HAS_I2S

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Rtttl.h"

// I2S-based RTTTL ringtone synthesizer for boards with an I2S amplifier
// (e.g. LilyGo T-Deck: MAX98357A on BCK=7, WS=5, DOUT=6; CrowPanel 3.5 speaker amp).
//
// Architecture: play() pre-renders the entire RTTTL song (via the shared RtttlReader)
// into a PSRAM buffer, then launches a small FreeRTOS task on core 0 that streams it
// to the I2S DMA. Audio is fully decoupled from the LVGL loop so frame-rate spikes
// can't cause dropouts. begin() is SILENT (see AudioSink contract).

#define I2S_BUZZER_SAMPLE_RATE  44100   // At 22050 Hz, E5 (659 Hz) quantizes to 689 Hz (78 cents
                                        // sharp — sounds like a different note). At 44100 Hz the
                                        // worst-case error drops to ~25 cents and the E→B interval
                                        // ratio is preserved to within 1 cent.

class I2SBuzzer : public AudioSink {
public:
    void    begin() override;
    void    play(const char* rtttl) override;
    void    loop() override {}        // streaming runs on the core-0 task; nothing per-tick
    bool    isPlaying() override { return _playing; }
    void    quiet(bool q) override;
    bool    isQuiet() override   { return _is_quiet; }
    void    setVolume(uint8_t vol) override;
    uint8_t getVolume() const { return _volume; }

    static const char* const BUILTIN_NAMES[];
    static const char* const BUILTIN_RTTTL[];
    static int builtinCount();
    static const char* builtinByName(const char* name);

private:
    i2s_port_t   _port          = I2S_NUM_0;
    bool         _initialized   = false;
    bool         _is_quiet      = true;     // SILENT until the UI arms us
    volatile bool _playing      = false;
    uint8_t      _volume        = 5;

    // Pre-render buffer (allocated per-song in play())
    int16_t*     _audio_buf     = nullptr;
    size_t       _buf_cap       = 0;        // actual allocation size in samples
    volatile size_t _audio_len  = 0;

    // Background playback task
    TaskHandle_t _play_task     = nullptr;

    // Shared RTTTL parser (re-init each pass during pre-render in play())
    RtttlReader  _reader;

    void stopTask();            // stop + join the background task
    static void audioPlayTask(void* arg);
};

#endif // HAS_I2S
