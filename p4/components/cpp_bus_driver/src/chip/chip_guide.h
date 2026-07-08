/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-17 16:23:02
 * @LastEditTime: 2026-01-20 13:40:10
 * @License: GPL 3.0
 */
#pragma once

#include "../bus/bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Iic_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iic_Guide> _bus;

    private:
        int16_t _address;

    public:
        Iic_Guide(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address)
            : _bus(bus), _address(address)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);
        virtual bool end(void);

        bool init_list(const uint8_t *list, size_t length);
        bool init_list(const uint16_t *list, size_t length);
    };

    class Spi_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Spi_Guide> _bus;

        int32_t _cs;

    public:
        Spi_Guide(std::shared_ptr<Bus_Spi_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool init_list(const uint8_t *list, size_t length);
    };

    class Qspi_Guide : public Tool
    {
    protected:
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        enum class Spi_Trans
        {
            MODE_DIO = SPI_TRANS_MODE_DIO,
            MODE_QIO = SPI_TRANS_MODE_QIO,
            USE_RXDATA = SPI_TRANS_USE_RXDATA,
            USE_TXDATA = SPI_TRANS_USE_TXDATA,
            MODE_DIOQIO_ADDR = SPI_TRANS_MODE_DIOQIO_ADDR,
            MULTILINE_ADDR = SPI_TRANS_MULTILINE_ADDR,
            VARIABLE_CMD = SPI_TRANS_VARIABLE_CMD,
            VARIABLE_ADDR = SPI_TRANS_VARIABLE_ADDR,
            VARIABLE_DUMMY = SPI_TRANS_VARIABLE_DUMMY,
            CS_KEEP_ACTIVE = SPI_TRANS_CS_KEEP_ACTIVE,
            MULTILINE_CMD = SPI_TRANS_MULTILINE_CMD,
            MODE_OCT = SPI_TRANS_MODE_OCT,
        };
#else
        enum class Spi_Trans
        {
            MODE_DIO,
            MODE_QIO,
            USE_RXDATA,
            USE_TXDATA,
            MODE_DIOQIO_ADDR,
            MULTILINE_ADDR,
            VARIABLE_CMD,
            VARIABLE_ADDR,
            VARIABLE_DUMMY,
            CS_KEEP_ACTIVE,
            MULTILINE_CMD,
            MODE_OCT,
        };
#endif

        std::shared_ptr<Bus_Qspi_Guide> _bus;

        int32_t _cs;

    public:
        Qspi_Guide(std::shared_ptr<Bus_Qspi_Guide> bus, int32_t cs = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : _bus(bus), _cs(cs)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);

        bool init_list(const uint32_t *list, size_t length);
    };

    class Uart_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Uart_Guide> _bus;

    public:
        Uart_Guide(std::shared_ptr<Bus_Uart_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t baud_rate = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };

    class Iis_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Iis_Guide> _bus;

    public:
        Iis_Guide(std::shared_ptr<Bus_Iis_Guide> bus)
            : _bus(bus)
        {
        }
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        virtual bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) = 0;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        virtual bool begin(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz, nrf_i2s_swidth_t data_bit_width, nrf_i2s_channels_t channel) = 0;
#endif
    };

    class Sdio_Guide : public Tool
    {
    protected:
        std::shared_ptr<Bus_Sdio_Guide> _bus;

    public:
        Sdio_Guide(std::shared_ptr<Bus_Sdio_Guide> bus)
            : _bus(bus)
        {
        }

        virtual bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE);
    };
}