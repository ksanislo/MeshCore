#pragma once

// LilyGo T-Display-P4 (ESP32-P4) target -- EXPERIMENTAL. Milestone 1a: headless SX1262 companion,
// no display. Mirrors the minimal ebyte_eora_s3 target. The SX1262 RST/DIO1 lines are on the XL9535
// I2C GPIO expander (milestone 1b), so radio bring-up is NOT functional yet -- this compiles the
// platform + RadioLib + companion. See ESP32P4-TDISPLAY-PORT-SCOPING.md.

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/ESP32Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>

extern ESP32Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;

bool radio_init();
mesh::LocalIdentity radio_new_identity();
