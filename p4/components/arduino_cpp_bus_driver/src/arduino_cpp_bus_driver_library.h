/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:14:18
 * @LastEditTime: 2025-08-08 12:00:44
 * @License: GPL 3.0
 */

#pragma once

#include "cpp_bus_driver_library.h"

constexpr uint8_t LOW = 0x0;
constexpr uint8_t HIGH = 0x1;

// GPIO FUNCTIONS
constexpr uint8_t INPUT = 0x01;
// Changed OUTPUT from 0x02 to behave the same as Arduino pinMode(pin,OUTPUT)
// where you can read the state of pin even when it is set as OUTPUT
constexpr uint8_t OUTPUT = 0x03;
constexpr uint8_t PULLUP = 0x04;
constexpr uint8_t INPUT_PULLUP = 0x05;
constexpr uint8_t PULLDOWN = 0x08;
constexpr uint8_t INPUT_PULLDOWN = 0x09;
constexpr uint8_t OPEN_DRAIN = 0x10;
constexpr uint8_t OUTPUT_OPEN_DRAIN = 0x13;
constexpr uint8_t ANALOG = 0xC0;

// Interrupt Modes
constexpr uint8_t DISABLED = 0x00;
constexpr uint8_t RISING = 0x01;
constexpr uint8_t FALLING = 0x02;
constexpr uint8_t CHANGE = 0x03;
constexpr uint8_t ONLOW = 0x04;
constexpr uint8_t ONHIGH = 0x05;
constexpr uint8_t ONLOW_WE = 0x0C;
constexpr uint8_t ONHIGH_WE = 0x0D;

constexpr uint8_t LSBFIRST = 0;
constexpr uint8_t MSBFIRST = 1;

constexpr uint8_t SPI_LSBFIRST = 0;
constexpr uint8_t SPI_MSBFIRST = 1;

constexpr uint8_t SPI_MODE0 = 0;
constexpr uint8_t SPI_MODE1 = 1;
constexpr uint8_t SPI_MODE2 = 2;
constexpr uint8_t SPI_MODE3 = 3;

using String = std::string;

void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
int64_t millis(void);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

void(attachInterrupt)(uint8_t pin, std::function<void(void)> intRoutine, int mode);
void detachInterrupt(uint8_t pin);