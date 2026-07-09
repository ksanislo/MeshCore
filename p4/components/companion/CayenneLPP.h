/*
 * CayenneLPP.h — minimal CayenneLPP shim for the P4/IDF companion backend.
 *
 * The real CayenneLPP library (a PlatformIO lib dep on the S3 side) pulls in
 * ArduinoJson on the IDF path and a pile of message/polyline helpers we don't
 * use. The companion backend only needs the tiny slice used by MyMesh's
 * telemetry replies (reset / addVoltage / addPercentage / getSize / getBuffer)
 * plus the base SensorManager's querySensors(CayenneLPP&) signature. This shim
 * implements exactly that with standard LPP framing, no external deps.
 *
 * P4 build only — the S3 build uses the real library.
 */
#pragma once
#include <stdint.h>
#include <string.h>

#ifndef LPP_VOLTAGE
#define LPP_VOLTAGE     116   // 2 bytes, 0.01 V unsigned
#endif
#ifndef LPP_PERCENTAGE
#define LPP_PERCENTAGE  120   // 1 byte, 1 % unsigned
#endif

class CayenneLPP {
  uint8_t  _buf[64];
  uint8_t  _len;
  uint8_t  _cap;
public:
  CayenneLPP(uint8_t size = 64) : _len(0) {
    _cap = (size > sizeof(_buf)) ? (uint8_t)sizeof(_buf) : size;
  }

  void     reset()            { _len = 0; }
  uint8_t  getSize() const    { return _len; }
  uint8_t* getBuffer()        { return _buf; }
  uint8_t  copy(uint8_t* dst) const { memcpy(dst, _buf, _len); return _len; }

  // Voltage: LPP type 116, 2 bytes, 0.01 V units.
  uint8_t addVoltage(uint8_t channel, float voltage) {
    if (_len + 4 > _cap) return 0;
    int16_t v = (int16_t)(voltage * 100.0f + 0.5f);
    _buf[_len++] = channel;
    _buf[_len++] = LPP_VOLTAGE;
    _buf[_len++] = (uint8_t)(v >> 8);
    _buf[_len++] = (uint8_t)(v & 0xFF);
    return _len;
  }

  // Percentage: LPP type 120, 1 byte, 1 % units.
  uint8_t addPercentage(uint8_t channel, uint32_t percentage) {
    if (_len + 3 > _cap) return 0;
    _buf[_len++] = channel;
    _buf[_len++] = LPP_PERCENTAGE;
    _buf[_len++] = (uint8_t)(percentage & 0xFF);
    return _len;
  }
};
