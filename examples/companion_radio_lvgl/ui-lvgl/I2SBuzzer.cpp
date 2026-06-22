#include "I2SBuzzer.h"
#ifdef HAS_I2S

#include <string.h>
#include <esp_heap_caps.h>
#include <Arduino.h>   // millis()

// Weak board hook: enable/disable an external amp around playback. Default no-op
// (T-Deck's MAX98357A is always live); CrowPanel overrides it to drive PIN_SPK_MUTE.
extern "C" void board_audio_amp_enable(bool on);

// (Built-in tune catalog moved to Rtttl.cpp / rtttlAlertNames() -- shared with the piezo.)

// ---------------------------------------------------------------------------
// I2S init (SILENT — no un-mute, no auto-startup chime)
// ---------------------------------------------------------------------------
void I2SBuzzer::begin() {
    if (_initialized) return;

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
    // _is_quiet stays true; the UI arms us (quiet(false)) and plays the startup chime.
}

// ---------------------------------------------------------------------------
// Stop and join the background audio task
// ---------------------------------------------------------------------------
void I2SBuzzer::stopTask() {
    if (!_play_task && !_playing) return;
    _playing = false;
    board_audio_amp_enable(false);   // mute the amp BEFORE halting the clock (avoid click)
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
        board_audio_amp_enable(false);   // mute BEFORE stopping the clock
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

    // Pass 1: count total samples so we allocate exactly the right amount.
    if (!_reader.begin(rtttl)) return;
    size_t total = 0;
    float freq, durSec;
    while (_reader.nextNote(freq, durSec)) {
        uint32_t ns = (uint32_t)(durSec * (float)I2S_BUZZER_SAMPLE_RATE);
        if (!ns) ns = 1;
        total += ns;
    }
    if (!total) return;

    // Allocate: try PSRAM first (ps_malloc), fall back to internal heap.
    _audio_buf = (int16_t*)ps_malloc(total * sizeof(int16_t));
    if (!_audio_buf) _audio_buf = (int16_t*)malloc(total * sizeof(int16_t));
    _buf_cap = _audio_buf ? total : 0;

    if (!_audio_buf) return;

    // Pass 2: re-parse and render into buffer.
    _reader.begin(rtttl);
    int16_t amp = (int16_t)(_volume * 1638u);
    _audio_len = 0;

    while (_reader.nextNote(freq, durSec) && _audio_len < _buf_cap) {
        uint32_t ns = (uint32_t)(durSec * (float)I2S_BUZZER_SAMPLE_RATE);
        if (!ns) ns = 1;
        size_t n = ns;
        if (_audio_len + n > _buf_cap) n = _buf_cap - _audio_len;

        int16_t* dst = _audio_buf + _audio_len;
        if (freq < 1.0f || !amp) {
            memset(dst, 0, n * sizeof(int16_t));
        } else {
            uint32_t half = (uint32_t)((float)I2S_BUZZER_SAMPLE_RATE / (2.0f * freq));
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
    board_audio_amp_enable(true); // un-mute the amp now that the clock is running
    _playing = true;
    xTaskCreatePinnedToCore(audioPlayTask, "i2s_rt", 4096, this, 5, &_play_task, 0);
}

void I2SBuzzer::quiet(bool q) {
    _is_quiet = q;
    if (q) stopTask();
}

void I2SBuzzer::setVolume(uint8_t vol) {
    _volume = (vol > 10) ? 10 : vol;
}

#endif // HAS_I2S
