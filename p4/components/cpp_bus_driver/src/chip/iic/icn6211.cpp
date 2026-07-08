/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-01-16 17:37:48
 * @License: GPL 3.0
 */
#include "icn6211.h"
#include <cmath>

namespace Cpp_Bus_Driver
{
    bool Icn6211::begin(int32_t freq_hz)
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

        auto buffer = get_device_id();
        if (buffer != DEVICE_ID)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get icn6211 id fail (error id: %#X)\n", buffer);
            return false;
        }
        else
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "get icn6211 id success (id: %#X)\n", buffer);
        }

        return true;
    }

    uint16_t Icn6211::get_device_id(void)
    {
        uint8_t buffer[2] = {0};

        if (_bus->read(static_cast<uint8_t>(Cmd::RO_DEVICE_ID_START), buffer, 2) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
            return -1;
        }

        return buffer[0] << 8 | buffer[1];
    }

    bool Icn6211::assert_interface_params_out_of_range(Interface_Params &params)
    {
        bool result = true;

        // 检查并限制rgb_width (H Active Pixel) - 最大值4095
        if (params.rgb_width > 4095)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_width out of range (setting to %d > 4095)\n", params.rgb_width);
            params.rgb_width = 4095;
            result = false;
        }
        else if (params.rgb_width == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_width out of range (cannot be 0)\n");
            params.rgb_width = 1;
            result = false;
        }

        // 检查并限制rgb_height (V Active Line) - 最大值4095
        if (params.rgb_height > 4095)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_height out of range (setting to %d > 4095)\n", params.rgb_height);
            params.rgb_height = 4095;
            result = false;
        }
        else if (params.rgb_height == 0)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_height out of range (cannot be 0)\n");
            params.rgb_height = 1;
            result = false;
        }

        // 检查并限制rgb_hfp (H Front Porch) - 最大值1023
        if (params.rgb_hfp > 1023)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_hfp out of range (setting to %d > 1023)\n", params.rgb_hfp);
            params.rgb_hfp = 1023;
            result = false;
        }

        // 检查并限制rgb_hsync (H Sync Width) - 最大值1023
        if (params.rgb_hsync > 1023)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_hsync out of range (setting to %d > 1023)\n", params.rgb_hsync);
            params.rgb_hsync = 1023;
            result = false;
        }

        // 检查并限制rgb_hbp (H Back Porch) - 最大值1023
        if (params.rgb_hbp > 1023)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_hbp out of range (setting to %d > 1023)\n", params.rgb_hbp);
            params.rgb_hbp = 1023;
            result = false;
        }

        // 检查并限制rgb_vfp (V Front Porch) - 最大值255
        if (params.rgb_vfp > 255)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_vfp out of range (setting to %d > 255)\n", params.rgb_vfp);
            params.rgb_vfp = 255;
            result = false;
        }

        // 检查并限制rgb_vsync (V Sync Width) - 最大值255
        if (params.rgb_vsync > 255)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_vsync out of range (setting to %d > 255)\n", params.rgb_vsync);
            params.rgb_vsync = 255;
            result = false;
        }

        // 检查并限制rgb_vbp (V Back Porch) - 最大值255
        if (params.rgb_vbp > 255)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "rgb_vbp out of range (setting to %d > 255)\n", params.rgb_vbp);
            params.rgb_vbp = 255;
            result = false;
        }

        return result;
    }

    bool Icn6211::config_interface_params(Interface_Params params)
    {
        if (assert_interface_params_out_of_range(params) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "assert_interface_params_out_of_range fail\n");
        }

        // 设置 H/V Active 低位
        if (_bus->write(static_cast<uint8_t>(Cmd::HACTIVE_L), static_cast<uint8_t>(params.rgb_width)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::VACTIVE_L), static_cast<uint8_t>(params.rgb_height)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置 H/V Active 高位
        uint8_t hv_h = ((params.rgb_height & 0x0F00) >> 4) | ((params.rgb_width & 0x0F00) >> 8);
        if (_bus->write(static_cast<uint8_t>(Cmd::HV_ACTIVE_H), hv_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置 HFP/HSYNC/HBP 低位
        if (_bus->write(static_cast<uint8_t>(Cmd::HFP_L), static_cast<uint8_t>(params.rgb_hfp)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::HSYNC_L), static_cast<uint8_t>(params.rgb_hsync)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::HBP_L), static_cast<uint8_t>(params.rgb_hbp)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置 Horizontal Porch 高位
        uint8_t h_porch_h = ((params.rgb_hfp & 0x0300) >> 4) | ((params.rgb_hsync & 0x0300) >> 6) | ((params.rgb_hbp & 0x0300) >> 8);
        if (_bus->write(static_cast<uint8_t>(Cmd::H_PORCH_H), h_porch_h) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置 Vertical Porches
        if (_bus->write(static_cast<uint8_t>(Cmd::VFP), static_cast<uint8_t>(params.rgb_vfp)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::VSYNC), static_cast<uint8_t>(params.rgb_vsync)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        if (_bus->write(static_cast<uint8_t>(Cmd::VBP), static_cast<uint8_t>(params.rgb_vbp)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 设置时钟相位
        if (_bus->write(static_cast<uint8_t>(Cmd::SYS_CTRL_1), static_cast<uint8_t>(params.rgb_clock_phase)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 根据参考时钟设置选择时钟源
        if (params.external_reference_clock_mhz > 0)
        {
            // 使用外部参考时钟
            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_REF_SEL), static_cast<uint8_t>(0x90)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            // 计算外部参考时钟的PLL配置
            double ratio = params.rgb_clock_mhz / params.external_reference_clock_mhz;
            uint8_t pll_ref_div = 0;

            if (params.rgb_clock_mhz >= 87.5)
            {
                pll_ref_div = 0x31; // 0b00110001
                ratio *= 8.0;
            }
            else if (params.rgb_clock_mhz >= 43.75)
            {
                pll_ref_div = 0x51; // 0b01010001
                ratio *= 16.0;
            }
            else
            {
                pll_ref_div = 0x71; // 0b01110001
                ratio *= 32.0;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_REF_DIV), pll_ref_div) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            // 向上取整
            uint8_t pll_int_value = static_cast<uint8_t>(ratio);
            if (ratio > static_cast<double>(pll_int_value))
            {
                pll_int_value++;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_INT), pll_int_value) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "using external reference clock: %f mhz, PLL_INT: %d\n", params.external_reference_clock_mhz, pll_int_value);
        }
        else
        {
            // 使用MIPI时钟作为参考
            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_REF_SEL), static_cast<uint8_t>(0x92)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            // 计算并设置 PLL 时钟（MIPI时钟）
            double ratio = params.rgb_clock_mhz / params.mipi_clock_mhz;
            uint8_t pll_ref_div = 0;

            if (params.rgb_clock_mhz >= 87.5)
            {
                pll_ref_div = 0x20;
                ratio *= 4;
            }
            else if (params.rgb_clock_mhz >= 43.75)
            {
                pll_ref_div = 0x40;
                ratio *= 8;
            }
            else
            {
                pll_ref_div = 0x60;
                ratio *= 16;
            }

            if (params.mipi_clock_mhz >= 320.0)
            {
                pll_ref_div |= 0x13;
                ratio *= 24.0;
            }
            else if (params.mipi_clock_mhz >= 160.0)
            {
                pll_ref_div |= 0x12;
                ratio *= 16.0;
            }
            else if (params.mipi_clock_mhz >= 80.0)
            {
                pll_ref_div |= 0x11;
                ratio *= 8.0;
            }
            else
            {
                pll_ref_div |= 0x01;
                ratio *= 4.0;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_REF_DIV), pll_ref_div) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            // 向上取整
            uint8_t pll_int_value = static_cast<uint8_t>(ratio);
            if (ratio > static_cast<double>(pll_int_value))
            {
                pll_int_value++;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::PLL_INT), pll_int_value) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "using mipi clock: %f mhz, PLL_INT: %d\n", params.mipi_clock_mhz, pll_int_value);
        }

        // 设置PLL相关寄存器
        if (_bus->write(static_cast<uint8_t>(Cmd::PLL_WT_LOCK), static_cast<uint8_t>(0xFF)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::PLL_CTRL_1), static_cast<uint8_t>(0x20)) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        // 计算帧率
        _fps = params.rgb_clock_mhz * 1000000.0 /
               ((params.rgb_width + params.rgb_hfp + params.rgb_hsync + params.rgb_hbp) *
                (params.rgb_height + params.rgb_vfp + params.rgb_vsync + params.rgb_vbp));

        assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "config_interface_params fps: %.03f\n", _fps);

        return true;
    }

    bool Icn6211::set_polarity_enable(bool de, bool vsync, bool hsync)
    {
        uint8_t buffer = 0;
        if (de)
        {
            buffer |= 0x01;
        }
        if (vsync)
        {
            buffer |= 0x02;
        }
        if (hsync)
        {
            buffer |= 0x04;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::SYNC_POLARITY_TEST_MODE), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    bool Icn6211::set_mipi_lane(uint8_t lane)
    {
        uint8_t buffer = 0x28 | ((lane - 1) & 0x03);

        if (_bus->write(static_cast<uint8_t>(Cmd::DSI_CTRL), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Icn6211::set_rgb_output_format(Rgb_Format format, Rgb_Order order, bool rfc_enable)
    {
        uint8_t buffer = static_cast<uint8_t>(format) | static_cast<uint8_t>(order);

        if (rfc_enable == true)
        {
            buffer |= 0x80;
        }

        if (_bus->write(static_cast<uint8_t>(Cmd::SYS_CTRL_0), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        return true;
    }

    bool Icn6211::set_test_mode(Test_Mode mode)
    {
        uint8_t buffer = 0;

        if (_bus->read(static_cast<uint8_t>(Cmd::SYNC_POLARITY_TEST_MODE), &buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        if (mode == Test_Mode::DISABLE)
        {
            // 关闭 BIST
            if (_bus->write(static_cast<uint8_t>(Cmd::BIST_MODE_EN), static_cast<uint8_t>(0x83)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }

            if (_bus->write(static_cast<uint8_t>(Cmd::SYNC_POLARITY_TEST_MODE), static_cast<uint8_t>(0x00)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }
        else
        {
            // 开启 BIST
            if (_bus->write(static_cast<uint8_t>(Cmd::BIST_MODE_EN), static_cast<uint8_t>(0x43)) == false)
            {
                assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
                return false;
            }
        }

        buffer = (buffer & 0B00000111) | static_cast<uint8_t>(mode);

        // 写入 BIST 模式
        if (_bus->write(static_cast<uint8_t>(Cmd::SYNC_POLARITY_TEST_MODE), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool Icn6211::set_chip_enable(bool enable)
    {
        uint8_t buffer = enable << 4;

        if (_bus->write(static_cast<uint8_t>(Cmd::CONFIG_FINISH_SOFT_RESET), buffer) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }
}