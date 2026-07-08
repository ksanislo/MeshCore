/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:13:42
 * @LastEditTime: 2025-08-08 09:32:29
 * @License: GPL 3.0
 */
#include "co5300.h"

namespace Cpp_Bus_Driver
{
    bool Co5300::begin(int32_t freq_hz)
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

        if (Qspi_Guide::begin(freq_hz) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            // return false;
        }

        if (init_list(_init_list, sizeof(_init_list) / sizeof(uint32_t)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "init_list fail\n");
            return false;
        }

        if (_color_format != Color_Format::RGB565)
        {
            if (set_color_format(_color_format) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_color_format fail\n");
                return false;
            }
        }

        return true;
    }

    bool Co5300::set_render_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end)
    {
        x_start += _x_offset;
        x_end += _x_offset;
        y_start += _y_offset;
        y_end += _y_offset;

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_COLUMN_ADDRESS_SET) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_COLUMN_ADDRESS_SET) >> 8),
                static_cast<uint8_t>(Reg::WO_COLUMN_ADDRESS_SET),

                static_cast<uint8_t>(x_start >> 8),
                static_cast<uint8_t>(x_start),
                static_cast<uint8_t>(x_end >> 8),
                static_cast<uint8_t>(x_end),
            };
        uint8_t buffer_2[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_PAGE_ADDRESS_SET) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_PAGE_ADDRESS_SET) >> 8),
                static_cast<uint8_t>(Reg::WO_PAGE_ADDRESS_SET),

                static_cast<uint8_t>(y_start >> 8),
                static_cast<uint8_t>(y_start),
                static_cast<uint8_t>(y_end >> 8),
                static_cast<uint8_t>(y_end),
            };
        uint8_t buffer_3[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_WRITE_START) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_WRITE_START) >> 8),
                static_cast<uint8_t>(Reg::WO_MEMORY_WRITE_START),
            };

        if (_bus->write(buffer, 8) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(buffer_2, 8) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(buffer_3, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::send_color_stream(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
    {
        // 有效性检查
        if (data == nullptr)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid data (error data = nullptr)");
            return false;
        }
        else if (w == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error w = %d)", w);
            return false;
        }
        else if (h == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error h = %d)", h);
            return false;
        }
        else if (x >= _width)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid x (error (x = %d) >= (_width = %d))", x, _width);
            return false;
        }
        else if (y >= _height)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid y (error (y = %d) >= (_height = %d))", y, _height);
            return false;
        }
        else if (w > (_width - x))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid width (error (w = %d) > ((_width - x) = %d))", w, _width - x);
            return false;
        }
        else if (h > (_height - y))
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "invalid height (error (h = %d) > ((_height - y) = %d))", h, _height - y);
            return false;
        }

        // 硬件通常期望的是 [x_start, x_end] 和 [y_start, y_end] 的闭区间，即 x_end 和 y_end 是最后一个像素的坐标
        // 例如：
        // 如果 x=10, w=5，那么像素列是 10, 11, 12, 13, 14，所以 x_end 应该是 14（即 x + w - 1）
        // 如果不 -1，x_end 会是 15，可能超出实际范围或导致多写一个像素
        if (set_render_window(x, x + w - 1, y, y + h - 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_render_window fail\n");
            return false;
        }

        if (set_write_stream_mode(Write_Stream_Mode::CONTINUOUS_WRITE_4LANES) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_write_stream_mode fail\n");
            return false;
        }

        if (_color_format == Color_Format::RGB666)
        {
            if (_bus->write(data, w * h * 3, static_cast<uint32_t>(Spi_Trans::MODE_QIO)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_stream fail\n");
                return false;
            }
        }
        else
        {
            if (_bus->write(data, w * h * (static_cast<uint8_t>(_color_format) / 8), static_cast<uint32_t>(Spi_Trans::MODE_QIO)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_stream fail\n");
                return false;
            }
        }

        return true;
    }

    bool Co5300::set_write_stream_mode(Write_Stream_Mode mode)
    {
        uint8_t buffer[4] = {0};

        switch (mode)
        {
        case Write_Stream_Mode::WRITE_1LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_1LANES_CMD);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_START_WRITE);
            break;
        case Write_Stream_Mode::WRITE_4LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_START_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_START_WRITE);
            break;
        case Write_Stream_Mode::CONTINUOUS_WRITE_1LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_1LANES_CMD);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE);
            break;
        case Write_Stream_Mode::CONTINUOUS_WRITE_4LANES:
            buffer[0] = static_cast<uint8_t>(Cmd::WO_WRITE_COLOR_STREAM_4LANES_CMD_1);
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_MEMORY_CONTINUOUS_WRITE);
            break;

        default:
            break;
        }

        if (_bus->write(buffer, 4, static_cast<uint32_t>(NULL), true) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_brightness(uint8_t value)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_WRITE_DISPLAY_BRIGHTNESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_WRITE_DISPLAY_BRIGHTNESS) >> 8),
                static_cast<uint8_t>(Reg::WO_COLUMN_ADDRESS_SET),

                static_cast<uint8_t>(value)};

        if (_bus->write(buffer, 5) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_sleep(bool status)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_IN) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_IN) >> 8),
                static_cast<uint8_t>(Reg::WO_SLEEP_IN),
            };

        if (status == false)
        {
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_OUT) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SLEEP_OUT) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_SLEEP_OUT);
        }

        if (_bus->write(buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_screen_off(bool status)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_OFF) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_OFF) >> 8),
                static_cast<uint8_t>(Reg::WO_DISPLAY_OFF),
            };

        if (status == false)
        {
            buffer[1] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_ON) >> 16);
            buffer[2] = static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_DISPLAY_ON) >> 8);
            buffer[3] = static_cast<uint8_t>(Reg::WO_DISPLAY_ON);
        }

        if (_bus->write(buffer, 4) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_color_enhance(Color_Enhance mode)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SET_COLOR_ENHANCE) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_SET_COLOR_ENHANCE) >> 8),
                static_cast<uint8_t>(Reg::WO_SET_COLOR_ENHANCE),

                static_cast<uint8_t>(mode),
            };

        if (_bus->write(buffer, 5) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Co5300::set_color_format(Color_Format format)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(Cmd::WO_WRITE_REGISTER),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_INTERFACE_PIXEL_FORMAT) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Reg::WO_INTERFACE_PIXEL_FORMAT) >> 8),
                static_cast<uint8_t>(Reg::WO_INTERFACE_PIXEL_FORMAT),

                static_cast<uint8_t>(0x55),
            };

        switch (format)
        {
        case Color_Format::RGB565:
            break;
        case Color_Format::RGB666:
            buffer[4] = 0x66;
            break;
        case Color_Format::RGB888:
            buffer[4] = 0x77;
            break;

        default:
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "unknown format\n");
            return false;
        }

        if (_bus->write(buffer, 5) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
