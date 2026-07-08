/*
 * Stream.h — Minimal Arduino Stream stub for ESP-IDF
 * MeshCore's Utils.h and Identity.cpp use this.
 *
 * Includes <Arduino.h> from arduino_cpp_bus_driver (provides millis,
 * Serial, String, etc.) plus our extras from arduino_compat.h.
 */
#pragma once

#include <Arduino.h>
#include "arduino_compat.h"

class Stream {
public:
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
    virtual size_t write(uint8_t b) { (void)b; return 0; }
    virtual size_t write(const uint8_t* buf, size_t len) { (void)buf; (void)len; return 0; }
    virtual void flush() {}

    size_t readBytes(uint8_t* buffer, size_t length) {
        size_t count = 0;
        while (count < length) {
            int c = read();
            if (c < 0) break;
            buffer[count++] = (uint8_t)c;
        }
        return count;
    }
    size_t readBytes(char* buffer, size_t length) {
        return readBytes((uint8_t*)buffer, length);
    }

    size_t print(const char* s) { size_t n = strlen(s); write((const uint8_t*)s, n); return n; }
    size_t println(const char* s = "") { size_t n = print(s); write('\n'); return n + 1; }
    size_t print(int v) { char b[12]; snprintf(b, sizeof(b), "%d", v); return print(b); }
    size_t println(int v) { size_t n = print(v); write('\n'); return n + 1; }

    virtual ~Stream() {}
};