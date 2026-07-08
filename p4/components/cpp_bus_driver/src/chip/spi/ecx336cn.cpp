/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2026-01-16 10:09:07
 * @License: GPL 3.0
 */
#include "ecx336cn.h"

namespace Cpp_Bus_Driver
{
    bool Ecx336cn::begin(int32_t freq_hz)
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

        if (Spi_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (init_list(_init_list_640x400_60hz, sizeof(_init_list_640x400_60hz)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        if (set_power_save_mode(false) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_power_save_mode fail\n");
            return false;
        }

        return true;
    }

    bool Ecx336cn::set_power_save_mode(bool enable)
    {
        if (enable == true)
        {
            if (_bus->write(static_cast<uint8_t>(Cmd::WO_POWER_SAVE_MODE), 0x0E) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->write(static_cast<uint8_t>(Cmd::WO_POWER_SAVE_MODE), 0x0F) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        return true;
    }

}
