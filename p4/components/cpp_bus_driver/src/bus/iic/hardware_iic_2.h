/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-09-04 10:21:51
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
    class Hardware_Iic_2 : public Bus_Iic_Guide
    {
    public:
        int32_t _sda, _scl;
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        i2c_port_t _port;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        TwoWire *_iic_handle;
#endif
        int16_t _address = DEFAULT_CPP_BUS_DRIVER_VALUE;
        int32_t _freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE;

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        Hardware_Iic_2(int32_t sda, int32_t scl, i2c_port_t port = I2C_NUM_0)
            : _sda(sda), _scl(scl), _port(port)
        {
        }
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        Hardware_Iic_2(int32_t sda, int32_t scl, TwoWire *iic_handle = &Wire)
            : _sda(sda), _scl(scl), _iic_handle(iic_handle)
        {
        }
#endif

        bool begin(uint32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, uint16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool end(void) override;
        bool read(uint8_t *data, size_t length) override;
        bool write(const uint8_t *data, size_t length) override;
        bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) override;

        bool probe(const uint16_t address) override;

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        i2c_cmd_handle_t cmd_link_create(void) override;
        bool start_transmit(i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en = true) override;
        bool read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack = I2C_MASTER_LAST_NACK) override;
        bool write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en = true) override;
        bool write(i2c_cmd_handle_t cmd_handle, const uint8_t *data, size_t data_len, bool ack_en = true) override;
        bool stop_transmit(i2c_cmd_handle_t cmd_handle) override;
#endif
    };
}
