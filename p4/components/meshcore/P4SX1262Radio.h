/*
 * P4SX1262Radio.h — MeshCore mesh::Radio implementation for T-Display P4
 *
 * Wraps cpp_bus_driver::Sx126x to implement the mesh::Radio interface that
 * MeshCore's Dispatcher expects. This replaces RadioLib + CustomSX1262Wrapper
 * used on the Arduino-based (S3) targets.
 *
 * Hardware notes:
 *   - SX1262 SPI: direct GPIO (CS=24, BUSY=6, SCLK=2, MOSI=3, MISO=4)
 *   - SX1262 RST: XL9535 IO16 (I/O expander, not direct GPIO)
 *   - SX1262 DIO1: XL9535 IO17 (IRQ via I/O expander — NOT RELIABLE for polling)
 *   - SKY13453 RF switch: XL9535 IO1 (VCTL high = TX/RX path)
 *
 * NOTE: DIO1 polling through the XL9535 I2C expander does not work reliably.
 * All IRQ detection uses direct SPI reads of the SX1262 IRQ status register.
 *
 * Adapted verbatim from the Meck-P4 reference fork (GPL). Only the SD debug-log
 * printf shim (meck_log.h) was removed — we use plain printf.
 *
 * MUST NOT be included by main.cpp directly — doing so conflicts with the
 * same-named `SX1262` global definition. Only meshcore-component code includes it.
 */

#pragma once

#include <Dispatcher.h>   // for mesh::Radio interface
#include "cpp_bus_driver_library.h"
#include "t_display_p4_config.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include <math.h>
#include <memory>

// The SX1262 chip object is defined at file scope in main.cpp (external
// linkage). We reference it here so the radio adapter can drive the chip.
extern std::unique_ptr<Cpp_Bus_Driver::Sx126x> SX1262;

class P4SX1262Radio : public mesh::Radio {
public:
    P4SX1262Radio()
        : _inReceiveMode(false)
        , _lastRSSI(0)
        , _lastSNR(0)
        , _pktRecv(0)
        , _pktSent(0)
        , _currentFreq(0)
        , _currentBW(0)
        , _currentSF(0)
        , _currentCR(0)
        , _noiseFloor(-120)            // matches MeshCore's clamp / cold-start
        , _lastFloorSampleUs(0)
    {}

    // ---- mesh::Radio interface implementation ----

    void begin() override {
        // Radio hardware init is done in the board bring-up (main.cpp) + attach.
        // This is called after that, so radio should be in RX mode already.
        _inReceiveMode = true;
    }

    int recvRaw(uint8_t* bytes, int sz) override {
        // Periodic noise-floor sample, riding the recvRaw() cadence (~2 s).
        uint64_t now_us = esp_timer_get_time();
        if (now_us - _lastFloorSampleUs >= 2000000ULL) {
            _lastFloorSampleUs = now_us;
            sampleNoiseFloor();
        }

        if (!_inReceiveMode) return 0;

        // Poll IRQ status register directly via SPI (DIO1 via XL9535 unreliable)
        uint16_t irq = SX1262->get_irq_flag();
        if (irq == 0) return 0;

        Cpp_Bus_Driver::Sx126x::Irq_Status irq_status;
        if (!SX1262->parse_irq_status(irq, irq_status)) {
            clearAndResetRx();
            return 0;
        }

        if (irq_status.all_flag.crc_error) {
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::CRC_ERROR);
            resetToRx();
            return 0;
        }

        if (irq_status.all_flag.tx_rx_timeout) {
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TIMEOUT);
            resetToRx();
            return 0;
        }

        if (!irq_status.all_flag.rx_done) {
            clearAndResetRx();
            return 0;
        }

        uint8_t recv_len = SX1262->receive_data(bytes);
        if (recv_len == 0 || recv_len > sz) {
            SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
            resetToRx();
            return 0;
        }

        Cpp_Bus_Driver::Sx126x::Packet_Metrics pm;
        if (SX1262->get_lora_packet_metrics(pm)) {
            _lastRSSI = (float)pm.lora.rssi_average;
            _lastSNR = (float)pm.lora.snr;
        }

        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        resetToRx();

        _pktRecv++;
        return (int)recv_len;
    }

    uint32_t getEstAirtimeFor(int len_bytes) override {
        // Standard LoRa airtime calculation (SX1262 datasheet 6.1.4).
        if (_currentBW <= 0 || _currentSF == 0) return 100;  // fallback

        float bw_hz = _currentBW * 1000.0f;
        float ts = powf(2.0f, (float)_currentSF) / bw_hz;  // symbol time (s)

        float preamble_symbols = (_currentSF <= 8) ? 32.0f : 16.0f;
        float t_preamble = (preamble_symbols + 4.25f) * ts;

        int de = (_currentSF >= 11 && _currentBW <= 125.0f) ? 1 : 0;  // low data rate optimize
        int cr_val = _currentCR;  // 5-8 for 4/5..4/8

        float numerator = 8.0f * len_bytes - 4.0f * _currentSF + 28.0f + 16.0f;  // CRC=ON
        float denominator = 4.0f * ((float)_currentSF - 2.0f * de);
        if (denominator <= 0) denominator = 1;

        int n_payload = 8 + (int)(ceilf(numerator / denominator) * (cr_val));
        if (n_payload < 8) n_payload = 8;

        float t_payload = (float)n_payload * ts;
        float airtime_s = t_preamble + t_payload;

        return (uint32_t)(airtime_s * 1000.0f);  // ms
    }

    float packetScore(float snr, int packet_len) override {
        return (snr + 20.0f) / 40.0f;  // -20dB -> 0.0, +20dB -> 1.0
    }

    bool startSendRaw(const uint8_t* bytes, int len) override {
        _inReceiveMode = false;

        SX1262->start_lora_transmit(
            Cpp_Bus_Driver::Sx126x::Chip_Mode::TX, 0,
            Cpp_Bus_Driver::Sx126x::Fallback_Mode::FS
        );
        SX1262->set_irq_pin_mode(
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE,
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE
        );
        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);

        // send_data takes non-const; cast is safe here.
        SX1262->send_data(const_cast<uint8_t*>(bytes), len);

        _pktSent++;
        return true;
    }

    bool isSendComplete() override {
        Cpp_Bus_Driver::Sx126x::Irq_Status irq_status;
        if (SX1262->parse_irq_status(SX1262->get_irq_flag(), irq_status)) {
            if (irq_status.all_flag.tx_done) {
                return true;
            }
        }
        return false;
    }

    void onSendFinished() override {
        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::TX_DONE);
        resetToRx();
        _inReceiveMode = true;
    }

    bool isInRecvMode() const override {
        return _inReceiveMode;
    }

    bool isReceiving() override {
        // BUSY high during RX = mid-packet. Direct GPIO read (not via XL9535).
        return (gpio_get_level((gpio_num_t)SX1262_BUSY) == 1) && _inReceiveMode;
    }

    float getLastRSSI() const override { return _lastRSSI; }
    float getLastSNR() const override { return _lastSNR; }

    int getNoiseFloor() const override {
        return _noiseFloor;
    }

    // Noise-floor sampling. Meck read a live instantaneous RSSI (GetRssiInst,
    // 0x15) which our pinned cpp_bus_driver does not expose as a standalone
    // getter. MeshCore's interference-threshold and AGC-reset features are
    // disabled by default (Dispatcher::getInterferenceThreshold/getAGCResetInterval
    // return 0), so a static -120 dBm cold-start floor is functionally safe.
    // TODO(M6): add a GetRssiInst reader for a live floor (map/diagnostics).
    void sampleNoiseFloor() {
        // no-op for now — see comment above.
    }

    void resetAGC() override {
        // cpp_bus_driver does not expose boosted-gain toggle directly; no-op.
    }

    // ---- Additional accessors for stats ----
    uint32_t getPacketsRecv() const { return _pktRecv; }
    uint32_t getPacketsSent() const { return _pktSent; }
    // RadioLib-wrapper parity for the companion node-stats screen. CRC/receive
    // error accounting isn't tracked separately yet (M6 diagnostics); return 0.
    uint32_t getPacketsRecvErrors() const { return 0; }

    // ---- Radio parameter storage (set by radio_set_params) ----
    void setParams(float freq, float bw, uint8_t sf, uint8_t cr) {
        _currentFreq = freq;
        _currentBW = bw;
        _currentSF = sf;
        _currentCR = cr;
    }

    // RadioLib-wrapper parity: the shared companion backend calls these on
    // `radio_driver`. TX power is applied by config_lora_params on the next
    // radio_set_params(); boosted-gain isn't exposed by our pinned cpp_bus_driver.
    void setTxPower(int8_t dbm) { _txPower = dbm; }
    void setRxBoostedGainMode(uint8_t on) { _rxBoosted = on; }

private:
    bool _inReceiveMode;
    float _lastRSSI;
    float _lastSNR;
    uint32_t _pktRecv;
    uint32_t _pktSent;

    float _currentFreq;
    float _currentBW;
    uint8_t _currentSF;
    uint8_t _currentCR;
    int8_t  _txPower = 22;      // stored; applied via config_lora_params on setParams()
    uint8_t _rxBoosted = 0;     // stored; boosted-gain toggle not exposed by cpp_bus_driver

    int      _noiseFloor;
    uint64_t _lastFloorSampleUs;

    void resetToRx() {
        SX1262->start_lora_transmit(Cpp_Bus_Driver::Sx126x::Chip_Mode::RX);
        SX1262->set_irq_pin_mode(
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE,
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE,
            Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::DISABLE
        );
        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
    }

    void clearAndResetRx() {
        SX1262->clear_irq_flag(Cpp_Bus_Driver::Sx126x::Irq_Mask_Flag::RX_DONE);
        resetToRx();
    }
};
