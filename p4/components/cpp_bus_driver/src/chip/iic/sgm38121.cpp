/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-12-05 10:22:26
 * @License: GPL 3.0
 */
#include "sgm38121.h"

namespace Cpp_Bus_Driver
{
    bool Sgm38121::begin(int32_t freq_hz)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            pin_write(_rst, 1);
            delay_ms(10);
            pin_write(_rst, 0);
            delay_ms(10);
            pin_write(_rst, 1);
            delay_ms(10);
        }

        if (Iic_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        uint8_t buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm38121 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm38121 id success (id: %#X)\n", buffer);
        }

        return true;
    }

    uint8_t Sgm38121::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sgm38121::set_output_voltage(Channel channel, uint16_t voltage)
    {
        uint8_t buffer = 0;

        switch (channel)
        {
        case Channel::DVDD_1:
            if (voltage < 528)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "DVDD_1 voltage setting failed (voltage < 528)\n");
                voltage = 528;
            }
            else if (voltage > 1504)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "DVDD_1 voltage setting failed (voltage > 1504)\n");
                voltage = 1504;
            }
            buffer = (voltage - 504) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_DVDD1_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::DVDD_2:
            if (voltage < 528)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "DVDD_2 voltage setting failed (voltage < 528)\n");
                voltage = 528;
            }
            else if (voltage > 1504)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "DVDD_2 voltage setting failed (voltage > 1504)\n");
                voltage = 1504;
            }
            buffer = (voltage - 504) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_DVDD2_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::AVDD_1:
            if (voltage < 1504)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "AVDD_1 voltage setting failed (voltage < 1504)\n");
                voltage = 1504;
            }
            else if (voltage > 3424)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "AVDD_1 voltage setting failed (voltage > 3424)\n");
                voltage = 3424;
            }
            buffer = (voltage - 1384) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_AVDD1_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;
        case Channel::AVDD_2:
            if (voltage < 1504)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "AVDD_2 voltage setting failed (voltage < 1504)\n");
                voltage = 1504;
            }
            else if (voltage > 3424)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "AVDD_2 voltage setting failed (voltage > 3424)\n");
                voltage = 3424;
            }
            buffer = (voltage - 1384) / 8;
            if (_bus->write(static_cast<uint8_t>(Cmd::RW_AVDD2_OUTPUT_VOLTAGE_LEVEL), buffer) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    bool Sgm38121::set_channel_status(Channel channel, Status status)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_ENABLE_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
        switch (channel)
        {
        case Channel::DVDD_1:
            buffer = (buffer & 0B11111110) | static_cast<uint8_t>(status);
            break;
        case Channel::DVDD_2:
            buffer = (buffer & 0B11111101) | (static_cast<uint8_t>(status) << 1);
            break;
        case Channel::AVDD_1:
            buffer = (buffer & 0B11111011) | (static_cast<uint8_t>(status) << 2);
            break;
        case Channel::AVDD_2:
            buffer = (buffer & 0B11110111) | (static_cast<uint8_t>(status) << 3);
            break;

        default:
            break;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_ENABLE_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}
