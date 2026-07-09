/*
 * RTClib.h — stub for the P4/IDF companion backend.
 *
 * MyMesh.h includes <RTClib.h> unconditionally (Adafruit RTClib, an S3/Arduino
 * lib dep) but the companion backend never touches its DateTime/RTC_* types on
 * the no-connectivity path — the hardware RTC lives behind AutoDiscoverRTCClock,
 * which we don't compile on P4. This stub only needs to make the include resolve.
 *
 * A tiny DateTime is provided in case a future backend path references it; the
 * real RTC chip driver is added at M6.
 *
 * P4 build only — the S3 build uses the real Adafruit RTClib.
 */
#pragma once
#include <stdint.h>

class DateTime {
  uint32_t _unix;
public:
  DateTime(uint32_t t = 0) : _unix(t) {}
  uint32_t unixtime() const { return _unix; }
  bool     isValid()  const { return _unix != 0; }
};
