/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2023-11-16 15:42:22
 * @LastEditTime: 2025-09-24 16:48:37
 * @License: GPL 3.0
 */
#include "tca8418.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    constexpr const uint8_t Tca8418::_init_list[];
#endif

    bool Tca8418::begin(int32_t freq_hz)
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

        if (init_list(_init_list, sizeof(_init_list)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        return true;
    }

    bool Tca8418::set_keypad_scan_window(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
    {
        // 有效性检查
        if (w == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error w = %d)", w);
            return false;
        }
        else if (h == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error h = %d)", h);
            return false;
        }
        else if (x >= MAX_WIDTH_SIZE)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid x (error (x = %d) >= (MAX_WIDTH_SIZE = %d))", x, MAX_WIDTH_SIZE);
            return false;
        }
        else if (y >= MAX_HEIGHT_SIZE)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid y (error (y = %d) >= (MAX_HEIGHT_SIZE = %d))", y, MAX_HEIGHT_SIZE);
            return false;
        }
        else if (w > (MAX_WIDTH_SIZE - x))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error (w = %d) > ((MAX_WIDTH_SIZE - x) = %d))", w, MAX_WIDTH_SIZE - x);
            return false;
        }
        else if (h > (MAX_HEIGHT_SIZE - y))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error (h = %d) > ((MAX_HEIGHT_SIZE - y) = %d))", h, MAX_HEIGHT_SIZE - y);
            return false;
        }

        // 配置行选择寄存器
        uint8_t buffer_row_mask = 0;
        for (uint8_t i = y; i < (h + y); i++)
        {
            buffer_row_mask |= (1 << i); // 设置对应的行位为1，键盘扫描
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO1), buffer_row_mask) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 配置列选择寄存器
        uint8_t buffer_col_mask_low = 0;
        uint8_t buffer_col_mask_high = 0;
        for (uint8_t i = x; i < (w + x); i++)
        {
            if (i < 8)
            {
                buffer_col_mask_low |= (1 << i); // 0~7列
            }
            else
            {
                buffer_col_mask_high |= (1 << (i - 8)); // 8，9列
            }
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO2), buffer_col_mask_low) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::WO_KP_GPIO3), buffer_col_mask_high) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint8_t Tca8418::get_finger_count(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_KEY_LOCK_AND_EVENT_COUNTER), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer & 0x0F;
    }

    bool Tca8418::get_multiple_touch_point(Touch_Point &tp)
    {
        uint8_t buffer_finger_count = get_finger_count();
        if ((buffer_finger_count == static_cast<uint8_t>(-1)) || (buffer_finger_count == 0))
        {
            return false;
        }

        uint8_t buffer[buffer_finger_count] = {0};

#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        // 地址不能自动偏移
        for (size_t i = 0; i < buffer_finger_count; i++)
        {
            if (_bus->read(static_cast<uint8_t>(Cmd::RO_KEY_EVENT), &buffer[i]) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }
#else
        // 地址自动偏移
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_KEY_EVENT), buffer, buffer_finger_count) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }
#endif

        tp.finger_count = buffer_finger_count;

        for (uint8_t i = 0; i < tp.finger_count; i++)
        {
            Touch_Info buffer_ti;
            buffer_ti.press_flag = buffer[i] >> 7;
            buffer_ti.num = buffer[i] & 0B01111111;
            if (buffer_ti.num > 96)
            {
                buffer_ti.event_type = Event_Type::GPIO;
            }

            tp.info.push_back(buffer_ti);
        }

        return true;
    }

    uint8_t Tca8418::get_irq_flag(void)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::RW_INTERRUPT_STATUS), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer & 0B00011111;
    }

    bool Tca8418::parse_irq_status(uint8_t irq_flag, Irq_Status &status)
    {
        if (irq_flag == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error\n");
            return false;
        }

        status.ctrl_alt_del_key_sequence_flag = (irq_flag & 0B00010000) >> 4;
        status.fifo_overflow_flag = (irq_flag & 0B00001000) >> 3;
        status.keypad_lock_flag = (irq_flag & 0B00000100) >> 2;
        status.gpio_interrupt_flag = (irq_flag & 0B00000010) >> 1;
        status.key_events_flag = irq_flag & 0B00000001;

        return true;
    }

    bool Tca8418::clear_irq_flag(Irq_Flag flag)
    {
        if (_bus->write(static_cast<uint8_t>(Cmd::RW_INTERRUPT_STATUS), static_cast<uint8_t>(flag)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    uint32_t Tca8418::get_clear_gpio_irq_flag(void)
    {
        uint8_t buffer[3] = {0};
        if (_bus->read(static_cast<uint8_t>(Cmd::RO_GPIO_INTERRUPT_STATUS_START), buffer, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        return (static_cast<uint32_t>(buffer[0]) << 16) | (static_cast<uint32_t>(buffer[1]) << 8) | static_cast<uint32_t>(buffer[2]);
    }

    bool Tca8418::set_irq_pin_mode(Irq_Mask mode)
    {
        uint8_t buffer = 0;
        if (_bus->read(static_cast<uint8_t>(Cmd::RW_CONFIGURATION), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return false;
        }

        buffer = (buffer & 0B11110000) | static_cast<uint8_t>(mode);

        if (_bus->write(static_cast<uint8_t>(Cmd::RW_CONFIGURATION), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Tca8418::parse_touch_num(uint8_t num, Touch_Position &position)
    {
        if (num == static_cast<uint8_t>(-1))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error (num == -1)\n");
            return false;
        }

        if ((num == static_cast<uint8_t>(-1)) || (num == 0))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "parse error (num == 0)\n");
            return false;
        }

        position.x = (num - 1) % 10;
        position.y = (num - 1) / 10;

        return true;
    }

}
