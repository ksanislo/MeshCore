/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-11-22 15:16:10
 * @License: GPL 3.0
 */
#include "sy6970.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    constexpr const uint8_t Sy6970::_init_list[];
#endif

    bool Sy6970::begin(int32_t freq_hz)
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
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sy6970 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get sy6970 id success (id: %#X)\n", buffer);
        }

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    bool Sy6970::end(void)
    {
        if (Iic_Guide::end() == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "end fail\n");
            return false;
        }

        return true;
    }

    uint8_t Sy6970::get_device_id(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return (buffer & 0B00111000) >> 3;
    }

    bool Sy6970::set_adc_conversion_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable)
        {
            buffer |= 0B10000000;
        }
        else
        {
            buffer &= 0B01111111;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint16_t Sy6970::get_battery_voltage(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_BATTERY_VOLTAGE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return 0;
        }

        buffer &= 0B01111111;
        return 2304 + (buffer * 20);
    }

    uint16_t Sy6970::get_system_voltage(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_VOLTAGE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return 0;
        }

        buffer &= 0B01111111;
        return 2304 + (buffer * 20);
    }

    uint16_t Sy6970::get_bus_voltage(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_BUS_VOLTAGE_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return 0;
        }

        buffer &= 0B01111111;
        return 2600 + (buffer * 100);
    }

    bool Sy6970::set_ship_mode_enable(bool enable)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        if (enable)
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

    uint16_t Sy6970::get_charging_current(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_CHARGE_CURRENT), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return 0;
        }

        buffer &= 0B01111111;
        return buffer * 50;
    }

    uint8_t Sy6970::get_irq_flag(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_FAULT_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sy6970::parse_irq_status(uint8_t irq_flag, Irq_Status &status)
    {
        if (irq_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error\n");
            return false;
        }

        status.watchdog_expiration_flag = (irq_flag & 0B10000000) >> 7;
        status.boost_fault_flag = (irq_flag & 0B01000000) >> 6;
        status.charge_fault_status = static_cast<Charge_Fault>((irq_flag & 0B00110000) >> 4);
        status.battery_over_voltage_fault_flag = (irq_flag & 0B00001000) >> 3;
        status.ntc_fault_status = static_cast<NTC_Fault>(irq_flag & 0B00000111);

        return true;
    }

    uint8_t Sy6970::get_chip_status(void)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer;
    }

    bool Sy6970::parse_chip_status(uint8_t chip_flag, Chip_Status &status)
    {
        if (chip_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error\n");
            return false;
        }

        status.bus_status = static_cast<Bus_Status>((chip_flag & 0B11100000) >> 5);
        status.charge_status = static_cast<Charge_Status>((chip_flag & 0B00011000) >> 3);
        status.power_good_status = (chip_flag & 0B00000100) >> 2;
        status.usb_status = (chip_flag & 0B00000010) >> 1;
        status.system_voltage_regulation_status = chip_flag & 0B00000001;

        return true;
    }

    // bool Sy6970::set_charge_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00010000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11101111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_hiz_mode_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_ilim_pin_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B01000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_input_current_limit(uint16_t current_ma)
    // {
    //     if (current_ma < 100 || current_ma > 3250)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11000000;
    //     buffer |= ((current_ma - 100) / 50);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_INPUT_SOURCE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_boost_hot_threshold(uint8_t threshold)
    // {
    //     if (threshold > 3)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B00111111;
    //     buffer |= (threshold << 6);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_boost_cold_threshold(bool threshold)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (threshold)
    //     {
    //         buffer |= 0B00100000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11011111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_vindpm_offset(uint16_t offset_mv)
    // {
    //     if (offset_mv > 3100)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11100000;
    //     buffer |= (offset_mv / 100);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_TEMPERATURE_MONITOR_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_adc_conversion_rate(bool continuous)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (continuous)
    //     {
    //         buffer |= 0B01000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_boost_frequency(bool high_freq)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (high_freq)
    //     {
    //         buffer &= 0B11011111;
    //     }
    //     else
    //     {
    //         buffer |= 0B00100000;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_adaptive_current_limit_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00010000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11101111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_hvdcp_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00001000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11110111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_hvdcp_voltage_type(bool high_voltage)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (high_voltage)
    //     {
    //         buffer |= 0B00000100;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111011;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_force_dpdm_detection(bool force)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (force)
    //     {
    //         buffer |= 0B00000010;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111101;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_auto_dpdm_detection_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00000001;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_SYSTEM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_battery_load_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_watchdog_reset(bool reset)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (reset)
    //     {
    //         buffer |= 0B01000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_otg_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00100000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11011111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_min_system_voltage_limit(uint16_t voltage_mv)
    // {
    //     if (voltage_mv < 3000 || voltage_mv > 3700)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11110001;
    //     buffer |= (((voltage_mv - 3000) / 100) << 1);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_fast_charge_current_limit(uint16_t current_ma)
    // {
    //     if (current_ma > 5056)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_CURRENT_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B10000000;
    //     buffer |= (current_ma / 64);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_CURRENT_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_precharge_current_limit(uint16_t current_ma)
    // {
    //     if (current_ma < 64 || current_ma > 1024)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_PRECHRG_TERM_CURRENT_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B00001111;
    //     buffer |= (((current_ma - 64) / 64) << 4);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_PRECHRG_TERM_CURRENT_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_termination_current_limit(uint16_t current_ma)
    // {
    //     if (current_ma < 64 || current_ma > 1024)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_PRECHRG_TERM_CURRENT_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11110000;
    //     buffer |= ((current_ma - 64) / 64);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_PRECHRG_TERM_CURRENT_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_charge_voltage_limit(uint16_t voltage_mv)
    // {
    //     if (voltage_mv < 3840 || voltage_mv > 4608)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B00000011;
    //     buffer |= (((voltage_mv - 3840) / 16) << 2);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_battery_low_voltage_threshold(bool high_threshold)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (high_threshold)
    //     {
    //         buffer |= 0B00000010;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111101;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_battery_recharge_threshold(bool high_threshold)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (high_threshold)
    //     {
    //         buffer |= 0B00000001;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_VOLTAGE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_charge_termination_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_stat_pin_disable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B01000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_watchdog_timer(uint16_t timer_s)
    // {
    //     uint8_t value = 0;
    //     switch (timer_s)
    //     {
    //     case 0:
    //         value = 0;
    //         break;
    //     case 40:
    //         value = 1;
    //         break;
    //     case 80:
    //         value = 2;
    //         break;
    //     case 160:
    //         value = 3;
    //         break;
    //     default:
    //         return false;
    //     }

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11001111;
    //     buffer |= (value << 4);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_charge_safety_timer_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00001000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11110111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_fast_charge_timer(uint8_t timer_hr)
    // {
    //     uint8_t value = 0;
    //     switch (timer_hr)
    //     {
    //     case 5:
    //         value = 0;
    //         break;
    //     case 8:
    //         value = 1;
    //         break;
    //     case 12:
    //         value = 2;
    //         break;
    //     case 20:
    //         value = 3;
    //         break;
    //     default:
    //         return false;
    //     }

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11111001;
    //     buffer |= (value << 1);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_jeita_low_temp_current(bool low_current)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (low_current)
    //     {
    //         buffer |= 0B00000001;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_battery_compensation_resistance(uint8_t resistance_mohm)
    // {
    //     if (resistance_mohm > 140)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B00011111;
    //     buffer |= ((resistance_mohm / 20) << 5);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_ir_compensation_voltage_clamp(uint8_t voltage_mv)
    // {
    //     if (voltage_mv > 224)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11100011;
    //     buffer |= ((voltage_mv / 32) << 2);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_thermal_regulation_threshold(uint8_t temperature)
    // {
    //     uint8_t value = 0;
    //     switch (temperature)
    //     {
    //     case 60:
    //         value = 0;
    //         break;
    //     case 80:
    //         value = 1;
    //         break;
    //     case 100:
    //         value = 2;
    //         break;
    //     case 120:
    //         value = 3;
    //         break;
    //     default:
    //         return false;
    //     }

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11111100;
    //     buffer |= value;

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_IR_COMPENSATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_force_adaptive_current_limit(bool force)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (force)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_safety_timer_slowdown(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B01000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B10111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_jeita_high_temp_voltage(bool normal_voltage)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (normal_voltage)
    //     {
    //         buffer |= 0B00010000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11101111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_batfet_turnoff_delay(bool delay)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (delay)
    //     {
    //         buffer |= 0B00001000;
    //     }
    //     else
    //     {
    //         buffer &= 0B11110111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_batfet_reset_enable(bool enable)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (enable)
    //     {
    //         buffer |= 0B00000100;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111011;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_pump_control(bool up, bool down)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (up)
    //     {
    //         buffer |= 0B00000010;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111101;
    //     }

    //     if (down)
    //     {
    //         buffer |= 0B00000001;
    //     }
    //     else
    //     {
    //         buffer &= 0B11111110;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_boost_voltage(uint16_t voltage_mv)
    // {
    //     if (voltage_mv < 4550 || voltage_mv > 5510)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_BOOST_MODE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B00001111;
    //     buffer |= (((voltage_mv - 4550) / 64) << 4);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_BOOST_MODE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_boost_current_limit(uint16_t current_ma)
    // {
    //     uint8_t value = 0;
    //     switch (current_ma)
    //     {
    //     case 500:
    //         value = 0;
    //         break;
    //     case 750:
    //         value = 1;
    //         break;
    //     case 1200:
    //         value = 2;
    //         break;
    //     case 1400:
    //         value = 3;
    //         break;
    //     case 1650:
    //         value = 4;
    //         break;
    //     case 1875:
    //         value = 5;
    //         break;
    //     case 2150:
    //         value = 6;
    //         break;
    //     case 2450:
    //         value = 7;
    //         break;
    //     default:
    //         return false;
    //     }

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_BOOST_MODE_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B11111000;
    //     buffer |= value;

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_BOOST_MODE_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_vindpm_mode(bool absolute)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_VINDPM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (absolute)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_VINDPM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::set_absolute_vindpm_threshold(uint16_t voltage_mv)
    // {
    //     if (voltage_mv < 3900 || voltage_mv > 15300)
    //         return false;

    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RW_VINDPM_CONTROL), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     buffer &= 0B10000000;
    //     buffer |= ((voltage_mv - 2600) / 100);

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RW_VINDPM_CONTROL), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // Sy6970::Bus_Status Sy6970::read_bus_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return Bus_Status::NO_INPUT;
    //     }

    //     return static_cast<Bus_Status>((buffer & 0B11100000) >> 5);
    // }

    // Sy6970::Charge_Status Sy6970::read_charge_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return Charge_Status::NOT_CHARGING;
    //     }

    //     return static_cast<Charge_Status>((buffer & 0B00011000) >> 3);
    // }

    // bool Sy6970::read_power_good_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B00000100) >> 2;
    // }

    // bool Sy6970::read_usb_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B00000010) >> 1;
    // }

    // bool Sy6970::read_system_voltage_regulation_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return buffer & 0B00000001;
    // }

    // bool Sy6970::read_thermal_regulation_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_BATTERY_VOLTAGE), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B10000000) >> 7;
    // }

    // uint16_t Sy6970::read_ntc_voltage_percentage(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_NTC_VOLTAGE), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return 0;
    //     }

    //     buffer &= 0B01111111;
    //     return 21000 + (buffer * 465);
    // }

    // bool Sy6970::read_bus_connection_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_BUS_VOLTAGE_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B10000000) >> 7;
    // }

    // bool Sy6970::read_vindpm_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_INPUT_CURRENT_LIMIT_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B10000000) >> 7;
    // }

    // bool Sy6970::read_iindpm_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_INPUT_CURRENT_LIMIT_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B01000000) >> 6;
    // }

    // uint16_t Sy6970::read_input_current_limit_setting(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RD_INPUT_CURRENT_LIMIT_STATUS), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return 0;
    //     }

    //     buffer &= 0B00111111;
    //     return 100 + (buffer * 50);
    // }

    // bool Sy6970::set_register_reset(bool reset)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     if (reset)
    //     {
    //         buffer |= 0B10000000;
    //     }
    //     else
    //     {
    //         buffer &= 0B01111111;
    //     }

    //     if (_bus->write(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
    //         return false;
    //     }

    //     return true;
    // }

    // bool Sy6970::read_aicl_optimized_status(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B01000000) >> 6;
    // }

    // uint8_t Sy6970::read_device_configuration(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return 0;
    //     }

    //     return (buffer & 0B00111000) >> 3;
    // }

    // bool Sy6970::read_temperature_profile(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return false;
    //     }

    //     return (buffer & 0B00000100) >> 2;
    // }

    // uint8_t Sy6970::read_device_revision(void)
    // {
    //     uint8_t buffer = 0;
    //     if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID), &buffer) == false)
    //     {
    //         assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
    //         return 0;
    //     }

    //     return buffer & 0B00000011;
    // }
}