#include "I2SBuzzer.h"
#ifdef PIN_I2S_BCK

#include <string.h>
#include <ctype.h>
#include <esp_heap_caps.h>
#include <Arduino.h>   // millis()

// ---------------------------------------------------------------------------
// Built-in ringtone catalogue
// ---------------------------------------------------------------------------
const char* const I2SBuzzer::BUILTIN_NAMES[] = {
    "Nokia", "Tetris", "FurElise", "Reveille", nullptr
};

const char* const I2SBuzzer::BUILTIN_RTTTL[] = {
    "Nokia:d=4,o=5,b=225:8e6,8d6,f#5,g#5,8c#6,8b5,d5,e5,8b5,8a5,c#5,e5,2a5",
    "Tetris:d=4,o=5,b=160:e6,8b5,8c6,d6,8c6,8b5,a5,8a5,8c6,e6,8d6,8c6,b5,8b5,8c6,d6,e6,c6,a5,2a5",
    "FurElise:d=8,o=5,b=125:e6,d#6,e6,d#6,e6,b5,d6,c6,4a5,p,c5,e5,a5,4b5,p,e5,g#5,b5,4c6",
    "Reveille:d=4,o=5,b=180:8g,8g,g,8g,8g,g,8g,8e,8c,8e,2g,8g,8g,8g,8e,8e,8g,8e,2c",
    nullptr
};

int I2SBuzzer::builtinCount() {
    int n = 0; while (BUILTIN_NAMES[n]) n++; return n;
}

const char* I2SBuzzer::builtinByName(const char* name) {
    if (!name || !*name) return BUILTIN_RTTTL[1];   // default: Tetris (index 1)
    for (int i = 0; BUILTIN_NAMES[i]; i++)
        if (strcasecmp(name, BUILTIN_NAMES[i]) == 0) return BUILTIN_RTTTL[i];
    return nullptr;
}

// ---------------------------------------------------------------------------
// Note frequency table (octave 4 reference, semitone 0=C … 11=B)
// ---------------------------------------------------------------------------
static const float NOTE_FREQ_OCT4[12] = {
    261.626f, 277.183f, 293.665f, 311.127f, 329.628f, 349.228f,
    369.994f, 391.995f, 415.305f, 440.000f, 466.164f, 493.883f
};

float I2SBuzzer::noteFreq(int semitone, int octave) {
    float f = NOTE_FREQ_OCT4[semitone % 12];
    // RTTTL octave 5 = C at 523 Hz (scientific pitch C5). Our table is C4 (261 Hz),
    // so shift by (octave - 5) not (octave - 4) — otherwise all notes play one octave high.
    int d = octave - 5;
    if (d > 0)      f *= (float)(1 << d);
    else if (d < 0) f /= (float)(1 << (-d));
    return f;
}

static int letterSemitone(char c) {
    switch (tolower((unsigned char)c)) {
        case 'c': return 0; case 'd': return 2; case 'e': return 4;
        case 'f': return 5; case 'g': return 7; case 'a': return 9;
        case 'b': return 11;
        default:  return -1;
    }
}

// ---------------------------------------------------------------------------
// I2S init
// ---------------------------------------------------------------------------
void I2SBuzzer::begin(i2s_port_t port) {
    _port = port;
    if (_initialized) return;

    // Buffer is now allocated per-song in play(); nothing to allocate here.

    i2s_config_t cfg        = {};
    cfg.mode                = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate         = I2S_BUZZER_SAMPLE_RATE;
    cfg.bits_per_sample     = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format      = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format= I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags    = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count       = 4;
    cfg.dma_buf_len         = 64;   // small: task keeps DMA fed; writes use 100 ms timeout
    cfg.use_apll            = false;
    cfg.tx_desc_auto_clear  = true;

    i2s_driver_install(_port, &cfg, 0, nullptr);

    i2s_pin_config_t pins   = {};
    pins.bck_io_num         = PIN_I2S_BCK;
    pins.ws_io_num          = PIN_I2S_WS;
    pins.data_out_num       = PIN_I2S_DATA;
    pins.data_in_num        = I2S_PIN_NO_CHANGE;
    i2s_set_pin(_port, &pins);

    i2s_stop(_port);
    _initialized = true;
    quiet(false);

    strncpy(_startup_buf, "Startup:d=4,o=5,b=160:16c6,16e6,8g6", sizeof(_startup_buf) - 1);
    _startup_buf[sizeof(_startup_buf) - 1] = '\0';
    _startup_pending = true;
}

// ---------------------------------------------------------------------------
// RTTTL header parser
// ---------------------------------------------------------------------------
bool I2SBuzzer::parseHeader() {
    const char* p = strchr(_rtttl_buf, ':');
    if (!p) return false;
    p++;

    _def_dur = 4; _def_oct = 5; _bpm = 120;

    while (*p && *p != ':') {
        while (*p == ' ' || *p == ',') p++;
        if (!*p || *p == ':') break;
        char key = (char)tolower((unsigned char)*p++);
        if (*p == '=') p++;
        int val = 0;
        while (*p >= '0' && *p <= '9') { val = val*10 + (*p - '0'); p++; }
        switch (key) {
            case 'd': if (val) _def_dur = val; break;
            case 'o': if (val) _def_oct = val; break;
            case 'b': if (val) _bpm     = val; break;
        }
    }
    if (*p == ':') p++;
    _note_ptr = p;
    return *p != '\0';
}

// ---------------------------------------------------------------------------
// Parse next note into _note_freq / _note_samps
// ---------------------------------------------------------------------------
bool I2SBuzzer::parseNextNote() {
    if (!_note_ptr || !*_note_ptr) return false;

    const char* p = _note_ptr;
    while (*p == ',' || *p == ' ' || *p == '\t') p++;
    if (!*p) return false;

    int dur = _def_dur;
    if (*p >= '1' && *p <= '9') {
        dur = 0;
        while (*p >= '0' && *p <= '9') { dur = dur*10 + (*p - '0'); p++; }
        if (!dur) dur = _def_dur;
    }

    int semi  = letterSemitone(*p);
    bool rest = (*p == 'p' || *p == 'P');
    p++;

    if (*p == '#') { if (!rest && semi >= 0) semi++; p++; }

    bool dot = false;
    if (*p == '.') { dot = true; p++; }

    int oct = _def_oct;
    if (*p >= '4' && *p <= '7') { oct = *p - '0'; p++; }
    if (*p == '.') { dot = true; p++; }

    _note_ptr = p;

    float secs = (4.0f / (float)dur) * (60.0f / (float)_bpm);
    if (dot) secs *= 1.5f;
    _note_samps = (uint32_t)(secs * (float)I2S_BUZZER_SAMPLE_RATE);
    if (!_note_samps) _note_samps = 1;

    _note_freq = (rest || semi < 0) ? 0.0f : noteFreq(semi, oct);
    return true;
}

// ---------------------------------------------------------------------------
// Stop and join the background audio task
// ---------------------------------------------------------------------------
void I2SBuzzer::stopTask() {
    if (!_play_task && !_playing) return;
    _playing = false;
    i2s_stop(_port);
    // Give the task up to 60 ms to notice and exit cleanly.
    uint32_t t0 = millis();
    while (_play_task && (millis() - t0) < 60) vTaskDelay(pdMS_TO_TICKS(5));
    if (_play_task) { vTaskDelete(_play_task); _play_task = nullptr; }
}

// ---------------------------------------------------------------------------
// Background task: streams the pre-rendered buffer to I2S (core 0)
// ---------------------------------------------------------------------------
void I2SBuzzer::audioPlayTask(void* arg) {
    I2SBuzzer* self = (I2SBuzzer*)arg;

    // Track wall-clock start so we can wait for the DMA ring buffer to drain
    // after all writes complete (i2s_write may return before the DAC is done).
    uint32_t t_start = millis();
    uint32_t play_ms = (uint32_t)((uint64_t)self->_audio_len * 1000UL / I2S_BUZZER_SAMPLE_RATE);

    // One silence chunk so the amp settles before audio starts.
    int16_t sil[64] = {};
    size_t w = 0;
    i2s_write(self->_port, sil, sizeof(sil), &w, pdMS_TO_TICKS(100));

    // Write all audio. Use a finite timeout so stopTask() → i2s_stop() can
    // reliably unblock this call; portMAX_DELAY leaves the task stuck if the
    // DMA is halted, which force-kills it and corrupts the driver state.
    size_t offset = 0;
    while (offset < self->_audio_len && self->_playing) {
        size_t n = self->_audio_len - offset;
        if (n > 256) n = 256;
        i2s_write(self->_port, self->_audio_buf + offset,
                  n * sizeof(int16_t), &w, pdMS_TO_TICKS(100));
        if (w > 0) offset += w / sizeof(int16_t);
    }

    // Wait until the DMA has actually finished playing all queued audio.
    // Add 300 ms margin to account for the silence head and DMA latency.
    uint32_t play_end = t_start + play_ms + 300;
    while (self->_playing && ((int32_t)(play_end - millis()) > 0)) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // One silence chunk so the amp ramps down without a pop.
    if (self->_playing) {
        memset(sil, 0, sizeof(sil));
        i2s_write(self->_port, sil, sizeof(sil), &w, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(20));
        i2s_stop(self->_port);
        self->_playing = false;
    }

    self->_play_task = nullptr;
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void I2SBuzzer::play(const char* rtttl) {
    stopTask();

    // Free previous buffer now that the old task is gone.
    if (_audio_buf) { free(_audio_buf); _audio_buf = nullptr; _buf_cap = 0; }

    if (!rtttl || !*rtttl || _is_quiet) return;

    strncpy(_rtttl_buf, rtttl, sizeof(_rtttl_buf) - 1);
    _rtttl_buf[sizeof(_rtttl_buf) - 1] = '\0';

    if (!parseHeader()) return;

    // Pass 1: count total samples so we allocate exactly the right amount.
    size_t total = 0;
    while (parseNextNote()) total += _note_samps;

    if (!total) return;

    // Allocate: try PSRAM first (ps_malloc), fall back to internal heap.
    _audio_buf = (int16_t*)ps_malloc(total * sizeof(int16_t));
    if (!_audio_buf) _audio_buf = (int16_t*)malloc(total * sizeof(int16_t));
    _buf_cap = _audio_buf ? total : 0;

    if (!_audio_buf) return;

    // Pass 2: re-parse and render into buffer.
    parseHeader();
    int16_t amp = (int16_t)(_volume * 1638u);
    _audio_len = 0;

    while (parseNextNote() && _audio_len < _buf_cap) {
        size_t n = _note_samps;
        if (_audio_len + n > _buf_cap) n = _buf_cap - _audio_len;

        int16_t* dst = _audio_buf + _audio_len;
        if (_note_freq < 1.0f || !amp) {
            memset(dst, 0, n * sizeof(int16_t));
        } else {
            uint32_t half = (uint32_t)((float)I2S_BUZZER_SAMPLE_RATE / (2.0f * _note_freq));
            if (!half) half = 1;
            uint32_t period = half * 2;
            uint32_t ph = 0;
            for (size_t i = 0; i < n; i++) {
                dst[i] = (ph < half) ? amp : -amp;
                if (++ph >= period) ph = 0;
            }
        }
        _audio_len += n;
    }

    if (_audio_len == 0) return;

    i2s_zero_dma_buffer(_port);   // flush stale DMA data from previous (possibly force-killed) task
    i2s_start(_port);
    _playing = true;
    xTaskCreatePinnedToCore(audioPlayTask, "i2s_rt", 4096, this, 5, &_play_task, 0);
}

void I2SBuzzer::loop() {
    // Deferred startup: play on first loop() call so the DMA is being fed
    // before the chime notes arrive (avoids init noise).
    if (_startup_pending) {
        _startup_pending = false;
        play(_startup_buf);
    }
    // No per-tick DMA fill needed — audioPlayTask handles it on core 0.
}

bool I2SBuzzer::isPlaying() { return _playing; }

void I2SBuzzer::quiet(bool q) {
    _is_quiet = q;
    if (q) stopTask();
}

bool I2SBuzzer::isQuiet() { return _is_quiet; }

void I2SBuzzer::setVolume(uint8_t vol) {
    _volume = (vol > 10) ? 10 : vol;
}

void I2SBuzzer::startup()  { play("Startup:d=4,o=5,b=160:16c6,16e6,8g6"); }
void I2SBuzzer::shutdown() { play("Shutdown:d=4,o=5,b=100:8g5,16e5,16c5"); }

#endif // PIN_I2S_BCK
