/*
 * @Description: Print
 * @Author: LILYGO_L
 * @Date: 2025-08-09 11:43:38
 * @LastEditTime: 2025-08-09 14:22:23
 * @License: GPL 3.0
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "Printable.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print
{
private:
    int write_error;
    size_t printNumber(unsigned long, uint8_t);
    size_t printNumber(unsigned long long, uint8_t);
    size_t printFloat(double, uint8_t);

protected:
    void setWriteError(int err = 1)
    {
        write_error = err;
    }

public:
    Print() : write_error(0)
    {
    }

    int getWriteError()
    {
        return write_error;
    }
    void clearWriteError()
    {
        setWriteError(0);
    }

    virtual size_t write(uint8_t) = 0;
    size_t write(const char *str)
    {
        if (str == NULL)
        {
            return 0;
        }
        return write((const uint8_t *)str, strlen(str));
    }
    virtual size_t write(const uint8_t *buffer, size_t size);
    size_t write(const char *buffer, size_t size)
    {
        return write((const uint8_t *)buffer, size);
    }

    size_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

    // add availableForWrite to make compatible with Arduino Print.h
    // default to zero, meaning "a single write may block"
    // should be overriden by subclasses with buffering
    // virtual int availableForWrite() { return 0; }
    // size_t print(const __FlashStringHelper *ifsh) { return print(reinterpret_cast<const char *>(ifsh)); }
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(long long, int = DEC);
    size_t print(unsigned long long, int = DEC);
    size_t print(double, int = 2);
    size_t print(const Printable &);
    size_t print(struct tm *timeinfo, const char *format = NULL);

    // size_t println(const __FlashStringHelper *ifsh) { return println(reinterpret_cast<const char *>(ifsh)); }
    size_t println(const String &s);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(long long, int = DEC);
    size_t println(unsigned long long, int = DEC);
    size_t println(double, int = 2);
    size_t println(const Printable &);
    size_t println(struct tm *timeinfo, const char *format = NULL);
    size_t println(void);

    virtual void flush() { /* Empty implementation for backward compatibility */ }
};

class Serial_Default : public Print
{
public:
    size_t write(uint8_t data) override;
    size_t write(const uint8_t *buffer, size_t size) override;
};

extern Serial_Default Serial;
