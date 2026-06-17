#include "I2SBuzzer.h"
#ifdef PIN_I2S_BCK

#include <string.h>
#include <ctype.h>

// ---------------------------------------------------------------------------
// Built-in ringtone catalogue
// ---------------------------------------------------------------------------
const char* const I2SBuzzer::BUILTIN_NAMES[] = {
    "Nokia",
    "Tetris",
    "FurElise",
    "Reveille",
    nullptr
};

const char* const I2SBuzzer::BUILTIN_RTTTL[] = {
    // Nokia Grande Valse (the one everyone knows)
    "Nokia:d=4,o=5,b=225:8e6,8d6,f#5,g#5,8c#6,8b5,d5,e5,8b5,8a5,c#5,e5,2a5",
    // Tetris theme A
    "Tetris:d=4,o=5,b=160:e6,8b5,8c6,d6,8c6,8b5,a5,8a5,8c6,e6,8d6,8c6,b5,8b5,8c6,d6,e6,c6,a5,2a5",
    // Fur Elise opening
    "FurElise:d=8,o=5,b=125:e6,d#6,e6,d#6,e6,b5,d6,c6,4a5,p,c5,e5,a5,4b5,p,e5,g#5,b5,4c6",
    // Bugle Reveille
    "Reveille:d=4,o=5,b=180:8g,8g,g,8g,8g,g,8g,8e,8c,8e,2g,8g,8g,8g,8e,8e,8g,8e,2c",
    nullptr
};

int I2SBuzzer::builtinCount() {
    int n = 0;
    while (BUILTIN_NAMES[n]) n++;
    return n;
}

const char* I2SBuzzer::builtinByName(const char* name) {
    if (!name || !*name) return BUILTIN_RTTTL[0];   // default: Nokia
    for (int i = 0; BUILTIN_NAMES[i]; i++) {
        if (strcasecmp(name, BUILTIN_NAMES[i]) == 0) return BUILTIN_RTTTL[i];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Note frequency lookup: semitone 0=C, 1=C#, ..., 11=B in octave 4
// ---------------------------------------------------------------------------
static const float NOTE_FREQ_OCT4[12] = {
    261.626f, 277.183f, 293.665f, 311.127f, 329.628f, 349.228f,
    369.994f, 391.995f, 415.305f, 440.000f, 466.164f, 493.883f
};

float I2SBuzzer::noteFreq(int semitone, int octave) {
    float f = NOTE_FREQ_OCT4[semitone & 11];
    int d = octave - 4;
    if (d > 0)      f *= (float)(1 << d);
    else if (d < 0) f /= (float)(1 << (-d));
    return f;
}

// RTTTL letter → semitone (c=0, d=2, e=4, f=5, g=7, a=9, b=11); p/-1 = rest
static int letterSemitone(char c) {
    switch (tolower((unsigned char)c)) {
        case 'c': return 0;   case 'd': return 2;   case 'e': return 4;
        case 'f': return 5;   case 'g': return 7;   case 'a': return 9;
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

    i2s_config_t cfg        = {};
    cfg.mode                = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate         = I2S_BUZZER_SAMPLE_RATE;
    cfg.bits_per_sample     = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format      = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format= I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags    = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count       = 4;
    cfg.dma_buf_len         = I2S_BUZZER_BUF_SAMPLES;
    cfg.use_apll            = false;
    cfg.tx_desc_auto_clear  = true;   // silence on DMA underrun, no popping

    i2s_driver_install(_port, &cfg, 0, nullptr);

    i2s_pin_config_t pins   = {};
    pins.bck_io_num         = PIN_I2S_BCK;
    pins.ws_io_num          = PIN_I2S_WS;
    pins.data_out_num       = PIN_I2S_DATA;
    pins.data_in_num        = I2S_PIN_NO_CHANGE;
    i2s_set_pin(_port, &pins);

    i2s_stop(_port);   // idle until play() is called
    _initialized = true;

    quiet(false);
    startup();
}

// ---------------------------------------------------------------------------
// RTTTL header parser: "Name:d=N,o=N,b=N:..."
// Leaves _note_ptr pointing at the start of the note list.
// ---------------------------------------------------------------------------
bool I2SBuzzer::parseHeader() {
    const char* s = _rtttl_buf;

    // Skip name
    const char* p = strchr(s, ':');
    if (!p) return false;
    p++;

    _def_dur = 4; _def_oct = 5; _bpm = 120;

    while (*p && *p != ':') {
        while (*p == ' ' || *p == ',') p++;
        if (!*p || *p == ':') break;
        char key = (char)tolower((unsigned char)*p++);
        if (*p == '=') p++;
        int val = 0;
        while (*p >= '0' && *p <= '9') { val = val * 10 + (*p - '0'); p++; }
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
// Parse the next note from _note_ptr and set synthesis state.
// Returns false when the melody is exhausted.
// ---------------------------------------------------------------------------
bool I2SBuzzer::parseNextNote() {
    if (!_note_ptr || !*_note_ptr) return false;

    const char* p = _note_ptr;
    while (*p == ',' || *p == ' ' || *p == '\t') p++;
    if (!*p) return false;

    // Optional duration digits
    int dur = _def_dur;
    if (*p >= '1' && *p <= '9') {
        dur = 0;
        while (*p >= '0' && *p <= '9') { dur = dur * 10 + (*p - '0'); p++; }
        if (!dur) dur = _def_dur;
    }

    // Note letter (p = pause)
    int semi   = letterSemitone(*p);
    bool rest  = (*p == 'p' || *p == 'P');
    p++;

    // Optional sharp
    if (*p == '#') { if (!rest && semi >= 0) semi++; p++; }

    // Optional dot before octave
    bool dot = false;
    if (*p == '.') { dot = true; p++; }

    // Optional octave digit 4-7
    int oct = _def_oct;
    if (*p >= '4' && *p <= '7') { oct = *p - '0'; p++; }

    // Optional dot after octave
    if (*p == '.') { dot = true; p++; }

    _note_ptr = p;

    // Duration in samples
    float secs = (4.0f / (float)dur) * (60.0f / (float)_bpm);
    if (dot) secs *= 1.5f;
    _note_samps = (uint32_t)(secs * (float)I2S_BUZZER_SAMPLE_RATE);
    if (!_note_samps) _note_samps = 1;

    _note_freq  = (rest || semi < 0) ? 0.0f : noteFreq(semi, oct);
    _phase      = 0;
    return true;
}

// ---------------------------------------------------------------------------
// Generate and push up to I2S_BUZZER_BUF_SAMPLES samples, non-blocking.
// ---------------------------------------------------------------------------
void I2SBuzzer::writeChunk() {
    uint32_t n = (_note_samps < (uint32_t)I2S_BUZZER_BUF_SAMPLES)
                 ? _note_samps
                 : (uint32_t)I2S_BUZZER_BUF_SAMPLES;

    int16_t buf[I2S_BUZZER_BUF_SAMPLES];
    int16_t amp = (int16_t)(_volume * 1638u);   // 0..10 → 0..16380

    if (_note_freq < 1.0f || _is_quiet || !amp) {
        memset(buf, 0, n * sizeof(int16_t));
    } else {
        uint32_t half = (uint32_t)((float)I2S_BUZZER_SAMPLE_RATE / (2.0f * _note_freq));
        if (!half) half = 1;
        uint32_t period = half * 2;
        for (uint32_t i = 0; i < n; i++) {
            buf[i] = (_phase < half) ? amp : -amp;
            if (++_phase >= period) _phase = 0;
        }
    }

    size_t written = 0;
    // timeout=0: non-blocking; if DMA is full we'll retry next loop() tick
    i2s_write(_port, buf, n * sizeof(int16_t), &written, 0);
    uint32_t done = written / sizeof(int16_t);
    _note_samps -= (done < _note_samps) ? done : _note_samps;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void I2SBuzzer::play(const char* rtttl) {
    if (_playing) {
        _playing = false;
        i2s_stop(_port);
    }
    if (!rtttl || !*rtttl || _is_quiet) return;

    strncpy(_rtttl_buf, rtttl, sizeof(_rtttl_buf) - 1);
    _rtttl_buf[sizeof(_rtttl_buf) - 1] = '\0';

    if (!parseHeader() || !parseNextNote()) return;

    i2s_start(_port);
    _playing = true;
}

void I2SBuzzer::loop() {
    if (!_playing) return;

    if (_note_samps == 0) {
        if (!parseNextNote()) {
            _playing = false;
            i2s_stop(_port);
            return;
        }
    }
    writeChunk();
}

bool I2SBuzzer::isPlaying() { return _playing; }

void I2SBuzzer::quiet(bool q) {
    _is_quiet = q;
    if (q && _playing) {
        _playing = false;
        i2s_stop(_port);
    }
}

bool I2SBuzzer::isQuiet() { return _is_quiet; }

void I2SBuzzer::setVolume(uint8_t vol) {
    _volume = (vol > 10) ? 10 : vol;
}

void I2SBuzzer::startup()  { play("Startup:d=4,o=5,b=160:16c6,16e6,8g6"); }
void I2SBuzzer::shutdown() { play("Shutdown:d=4,o=5,b=100:8g5,16e5,16c5"); }

#endif // PIN_I2S_BCK
