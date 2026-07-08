/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-09-04 10:50:36
 * @License: GPL 3.0
 */
#include "software_iic.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Software_Iic::begin(uint32_t freq_hz, uint16_t address)
    {
        if (freq_hz == DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ;
        }

        uint32_t buffer_transmit_delay_us = static_cast<uint32_t>((1000000.0 / static_cast<double>(freq_hz)) / 2.0 + 0.5);

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _sda: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _scl: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config freq_hz: %d hz\n", freq_hz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "software_iic config _transmit_delay_us: %d us\n", buffer_transmit_delay_us);

        if (pin_mode(_sda, Pin_Mode::INPUT_OUTPUT_OD, Pin_Status::PULLUP) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_mode fail\n");
            return false;
        }

        if (pin_mode(_scl, Pin_Mode::OUTPUT_OD, Pin_Status::PULLUP) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_mode fail\n");
            return false;
        }

        _freq_hz = freq_hz;
        _transmit_delay_us = buffer_transmit_delay_us;
        _address = address;

        return true;
    }

    bool Software_Iic::start_transmit(void)
    {
        if (pin_write(_scl, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        if (pin_write(_sda, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);
        if (pin_write(_sda, 0) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);
        if (pin_write(_scl, 0) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);

        return true;
    }

    bool Software_Iic::read(uint8_t *data, size_t length)
    {
        if (start_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
            return false;
        }

        // 读操作发送地址最后一位为1
        if (write_byte((_address << 1) | 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
            return false;
        }
        if (wait_ack() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
            return false;
        }

        uint8_t *buffer_ptr = data;
        for (size_t i = 0; i < (length - 1); i++)
        {
            if (read_byte(buffer_ptr++) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "read_byte fail\n");
                return false;
            }

            if (write_ack(Ack_Bit::ACK) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
                return false;
            }
        }

        // 读取最后一位数据
        if (read_byte(buffer_ptr) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read_byte fail\n");
            return false;
        }

        if (write_ack(Ack_Bit::NACK) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
            return false;
        }

        if (stop_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::write(const uint8_t *data, size_t length)
    {
        if (start_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
            return false;
        }

        // 写操作发送地址最后一位为0
        if (write_byte(_address << 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
            return false;
        }
        if (wait_ack() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
            return false;
        }

        for (size_t i = 0; i < length; i++)
        {
            if (write_byte(data[i]) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
                return false;
            }
            if (wait_ack() == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
                return false;
            }
        }

        if (stop_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {
        if (start_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
            return false;
        }

        // 写操作发送地址最后一位为0
        if (write_byte(_address << 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
            return false;
        }
        if (wait_ack() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
            return false;
        }

        for (size_t i = 0; i < write_length; i++)
        {
            if (write_byte(write_data[i]) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
                return false;
            }
            if (wait_ack() == false)
            {
                // 如果不为最后一位数据，则报错
                if (i != (write_length - 1))
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
                    return false;
                }
            }
        }

        if (read(read_data, read_length) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::stop_transmit(void)
    {
        if (pin_write(_sda, 0) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        if (pin_write(_scl, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);
        if (pin_write(_sda, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::probe(const uint16_t address)
    {
        if (start_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail\n");
            return false;
        }

        // 写操作发送地址最后一位为0
        if (write_byte(address << 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "write_byte fail\n");
            return false;
        }
        if (wait_ack() == false)
        {
            // assert_log(Log_Level::BUS, __FILE__, __LINE__, "wait_ack fail\n");
            return false;
        }

        if (stop_transmit() == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "stop_transmit fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::write_byte(uint8_t data)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (pin_write(_sda, data & 0x80) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }

            delay_us(_transmit_delay_us);
            if (pin_write(_scl, 1) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }
            delay_us(_transmit_delay_us);
            if (pin_write(_scl, 0) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }

            data <<= 1;
        }

        // 释放sda
        if (pin_write(_sda, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }

        return true;
    }

    bool Software_Iic::read_byte(uint8_t *data)
    {
        if (pin_write(_sda, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }

        uint8_t buffer_data = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            buffer_data <<= 1;

            if (pin_write(_scl, 1) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }
            delay_us(_transmit_delay_us);

            if (pin_read(_sda) == 1)
            {
                buffer_data |= 0x01;
            }

            if (pin_write(_scl, 0) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }
            delay_us(_transmit_delay_us);
        }

        *data = buffer_data;

        return true;
    }

    bool Software_Iic::wait_ack(void)
    {
        delay_us(_transmit_delay_us);
        if (pin_write(_scl, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);

        // sda应该保持低电平作为应答(ack)
        bool buffer_ack = !pin_read(_sda);

        if (pin_write(_scl, 0) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);

        return buffer_ack;
    }

    bool Software_Iic::write_ack(Ack_Bit ack)
    {
        if (pin_write(_sda, static_cast<bool>(ack)) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }

        if (pin_write(_scl, 1) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);
        if (pin_write(_scl, 0) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "pin_write fail\n");
            return false;
        }
        delay_us(_transmit_delay_us);

        return true;
    }

#endif
}