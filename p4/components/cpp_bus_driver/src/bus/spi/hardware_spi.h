/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2026-01-15 09:31:09
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Spi : public Bus_Spi_Guide
    {
    public:
        int32_t _mosi, _sclk, _miso, _cs, _freq_hz;

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        spi_host_device_t _port;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        NRF_SPIM_Type *_port;
#endif

        uint8_t _mode;

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        uint32_t _flags;
        spi_clock_source_t _clock_source;

        bool _bus_init_flag = false;
        bool _device_init_flag = false;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        BitOrder _bit_order;
#endif

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        spi_device_handle_t _spi_device;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        std::unique_ptr<SPIClass> _spi_handle;
        SPISettings _spi_settings;
#endif

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        Hardware_Spi(int32_t mosi, int32_t sclk, int32_t miso = DEFAULT_CPP_BUS_DRIVER_VALUE,
                     spi_host_device_t port = SPI2_HOST, uint8_t mode = 0, uint32_t flags = DEFAULT_CPP_BUS_DRIVER_VALUE,
                     spi_clock_source_t clock_source = SPI_CLK_SRC_DEFAULT)
            : _mosi(mosi), _sclk(sclk), _miso(miso), _port(port), _mode(mode), _flags(flags), _clock_source(clock_source)
        {
        }
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        Hardware_Spi(int32_t mosi, int32_t sclk, int32_t miso = DEFAULT_CPP_BUS_DRIVER_VALUE,
                     NRF_SPIM_Type *port = NRF_SPIM3, uint8_t mode = 0, BitOrder bit_order = BitOrder::MSBFIRST)
            : _mosi(mosi), _sclk(sclk), _miso(miso), _port(port), _mode(mode), _bit_order(bit_order)
        {
        }
#endif

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool write(const void *data, size_t byte) override;
        bool read(void *data, size_t byte) override;
        bool write_read(const void *write_data, void *read_data, size_t data_byte) override;
    };
}
