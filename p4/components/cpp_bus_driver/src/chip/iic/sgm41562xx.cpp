/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-11-08 14:33:34
 * @License: GPL 3.0
 */
#include "sgm41562xx.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    constexpr const uint8_t Sgm41562xx::_init_list[];
#endif

    bool Sgm41562xx::begin(int32_t freq_hz)
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
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm41562xx id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sgm41562xx id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    bool Sgm41562xx::end(void)
    {
        if (Iic_Guide::end() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "end fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sgm41562xx::get_device_id(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    uint8_t Sgm41562xx::get_irq_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_FAULT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sgm41562xx::parse_irq_status(uint8_t irq_flag, Irq_Status &status)
    {
        if (irq_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error\n");
            return false;
        }

        status.Input_power_fault_flag = (irq_flag & 0B00100000) >> 5;
        status.thermal_shutdown_flag = (irq_flag & 0B00010000) >> 4;
        status.battery_over_voltage_fault_flag = (irq_flag & 0B00001000) >> 3;
        status.safety_timer_expiration_fault_flag = (irq_flag & 0B00000100) >> 2;
        status.ntc_exceeding_hot_flag = (irq_flag & 0B00000010) >> 1;
        status.ntc_exceeding_cold_flag = irq_flag & 0B00000001;

        return true;
    }

    bool Sgm41562xx::set_charge_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer &= 0B11110111;
        }
        else
        {
            buffer |= 0B00001000;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sgm41562xx::get_chip_status(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sgm41562xx::parse_chip_status(uint8_t chip_flag, Chip_Status &status)
    {
        if (chip_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error\n");
            return false;
        }

        status.watchdog_expiration_flag = (chip_flag & 0B00100000) >> 5;
        status.charge_status = static_cast<Charge_Status>((chip_flag & 0B00011000) >> 3);
        status.device_in_power_path_management_mode_flag = (chip_flag & 0B00000100) >> 2;
        status.input_power_status_flag = (chip_flag & 0B00000010) >> 1;
        status.thermal_regulation_status_flag = chip_flag & 0B00000001;

        return true;
    }

    bool Sgm41562xx::set_ship_mode_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable == true)
        {
            buffer |= 0B00100000;
        }
        else
        {
            buffer &= 0B11011111;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Sgm41562xx::set_enter_ship_time(Enter_Ship_Time time)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_FAULT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B00111111) | static_cast<uint8_t>(time);

        if (_bus->write(static_cast<uint8_t>(Cmd::RD_FAULT), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
