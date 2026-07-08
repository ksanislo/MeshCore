/*
 * @Description: SPI
 * @Author: LILYGO_L
 * @Date: 2025-08-05 11:23:01
 * @LastEditTime: 2025-08-09 10:28:54
 * @License: GPL 3.0
 */
#pragma once

#include "arduino_cpp_bus_driver_library.h"

class SPISettings
{
public:
    SPISettings() : _clock(1000000), _bitOrder(SPI_MSBFIRST), _dataMode(SPI_MODE0) {}
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
        : _clock(clock), _bitOrder(bitOrder), _dataMode(dataMode)
    {
    }
    uint32_t _clock;
    uint8_t _bitOrder;
    uint8_t _dataMode;
};

class SPIClass
{
private:
    bool init_flag = false;

    int8_t _spi_num;
    // bool _use_hw_ss;
    // uint32_t _div;
    //     bool _inTransaction;
    // #if !CONFIG_DISABLE_HAL_LOCKS
    //     SemaphoreHandle_t paramLock = NULL;
    // #endif
    // void writePattern_(const uint8_t *data, uint8_t size, uint8_t repeat);

    std::vector<uint8_t> _tx_buffer;

    uint8_t *_rx_buffer;
    size_t _rx_length;

public:
    std::shared_ptr<Cpp_Bus_Driver::Hardware_Spi> _bus = std::make_shared<Cpp_Bus_Driver::Hardware_Spi>(-1, -1);

    SPIClass(uint8_t spi_bus = 1)
        : _spi_num(spi_bus) /*, _use_hw_ss(false), _div(0), _inTransaction(false)*/
    {
    }

    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
    // void end();

    // void setHwCs(bool use);
    // void setBitOrder(uint8_t bitOrder);
    // void setDataMode(uint8_t dataMode);
    void setFrequency(uint32_t freq);
    // void setClockDivider(uint32_t clockDiv);

    // uint32_t getClockDivider();

    void beginTransaction(SPISettings settings);
    void endTransaction(void);
    void transfer(void *data, uint32_t size);
    uint8_t transfer(uint8_t data);
    // uint16_t transfer16(uint16_t data);
    // uint32_t transfer32(uint32_t data);

    void transferBytes(const uint8_t *data, uint8_t *out, uint32_t size);
    // void transferBits(uint32_t data, uint32_t *out, uint8_t bits);

    // void write(uint8_t data);
    // void write16(uint16_t data);
    // void write32(uint32_t data);
    // void writeBytes(const uint8_t *data, uint32_t size);
    // void writePixels(const void *data, uint32_t size); // ili9341 compatible
    // void writePattern(const uint8_t *data, uint8_t size, uint32_t repeat);

    int8_t pinSS() { return _bus->_cs; }
};

extern SPIClass SPI;
