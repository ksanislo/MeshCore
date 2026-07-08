/*
 * @Description: None
 * @Author: None
 * @Date: 2025-09-24 10:47:30
 * @LastEditTime: 2025-11-22 15:15:23
 * @License: GPL 3.0
 */
#include "aw21009xxx.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    constexpr const uint8_t Aw21009xxx::_init_list[];
#endif

    bool Aw21009xxx::begin(int32_t freq_hz)
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
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw21009xxx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get aw21009xxx id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    uint8_t Aw21009xxx::get_device_id(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Aw21009xxx::set_auto_power_save(bool enable)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GLOBAL_CONTROL_REGISTER), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B01111111) | (enable << 7);

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GLOBAL_CONTROL_REGISTER), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw21009xxx::set_chip_enable(bool enable)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_GLOBAL_CONTROL_REGISTER), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11111110) | enable;

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GLOBAL_CONTROL_REGISTER), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw21009xxx::set_brightness(Led_Channel channel, uint16_t value)
    {
        if (value > 4095)
        {
            value = 4095;
        }

        if (channel == Led_Channel::ALL)
        {
            for (uint8_t i = 0; i < static_cast<uint8_t>(Led_Channel::ALL); i++)
            {
                uint8_t buffer_address_lsb = static_cast<uint8_t>(Cmd::RW_BRIGHTNESS_CONTROL_REGISTER_START) + (i * 2);
                uint8_t buffer_address_msb = buffer_address_lsb + 1;

                if (_bus->write(buffer_address_lsb, value) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                    return false;
                }

                uint8_t buffer_msb_value = (value >> 4) & 0x0F;
                if (_bus->write(buffer_address_msb, buffer_msb_value) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                    return false;
                }
            }
        }
        else
        {
            uint8_t buffer_address_lsb = static_cast<uint8_t>(Cmd::RW_BRIGHTNESS_CONTROL_REGISTER_START) + (static_cast<uint8_t>(channel) * 2);
            uint8_t buffer_address_msb = buffer_address_lsb + 1;

            if (_bus->write(buffer_address_lsb, value) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            uint8_t buffer_msb_value = (value >> 4) & 0x0F;
            if (_bus->write(buffer_address_msb, buffer_msb_value) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_UPDATE_REGISTER), static_cast<uint8_t>(0)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw21009xxx::set_current_limit(Led_Channel channel, uint8_t value)
    {
        if (channel == Led_Channel::ALL)
        {
            for (uint8_t i = 0; i < static_cast<uint8_t>(Led_Channel::ALL); i++)
            {
                if (_bus->write(static_cast<uint8_t>(static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + i), value) == false)
                {
                    assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                    return false;
                }
            }
        }
        else
        {
            if (_bus->write(static_cast<uint8_t>(static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + static_cast<uint8_t>(channel)), value) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_UPDATE_REGISTER), static_cast<uint8_t>(0)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Aw21009xxx::set_global_current_limit(uint8_t value)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_GLOBAL_CURRENT_CONTROL_REGISTER), value) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
