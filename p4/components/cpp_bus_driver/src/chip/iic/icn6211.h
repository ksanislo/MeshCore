/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2026-01-16 11:57:07
 * @LastEditTime: 2026-01-16 17:38:02
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define ICN6211_DEVICE_DEFAULT_ADDRESS 0x2C

    class Icn6211 : public Iic_Guide
    {
    private:
        static constexpr uint16_t DEVICE_ID = 0x6211;

        enum class Cmd
        {
            RO_DEVICE_ID_START = 0x01,

            CONFIG_FINISH_SOFT_RESET = 0x09,

            SYS_CTRL_0 = 0x10,
            SYS_CTRL_1,

            BIST_MODE_EN = 0x14,

            HACTIVE_L = 0x20,
            VACTIVE_L,
            HV_ACTIVE_H,
            HFP_L,
            HSYNC_L,
            HBP_L,
            H_PORCH_H,
            VFP,
            VSYNC,
            VBP,
            SYNC_POLARITY_TEST_MODE,

            PLL_CTRL_1 = 0x51,

            PLL_REF_SEL = 0x56,

            PLL_WT_LOCK = 0x5C,

            PLL_INT = 0x69,

            PLL_REF_DIV = 0x6B,

            MIPI_MODE = 0x7A,

            DSI_CTRL = 0x86,
            MIPI_PN_SWAP,
        };

    public:
        enum class Rgb_Phase
        {
            PHASE_0 = 0x00,
            PHASE_90 = 0x01,
            PHASE_180 = 0x02,
            PHASE_270 = 0x03
        };

        struct Interface_Params
        {
            uint16_t rgb_width;
            uint16_t rgb_height;
            uint16_t rgb_hfp;
            uint16_t rgb_hsync;
            uint16_t rgb_hbp;
            uint16_t rgb_vfp;
            uint16_t rgb_vsync;
            uint16_t rgb_vbp;
            double rgb_clock_mhz; // RGB 输出时钟
            Rgb_Phase rgb_clock_phase;

            double mipi_clock_mhz; // MIPI 输入时钟

            double external_reference_clock_mhz = 0; // 外部参考时钟，设置为0则代表使用mipi时钟作为rgb信号时钟，设置非0则使用外部参考时钟作为rgb信号时钟
        };

        enum class Rgb_Format
        {
            // RGB666 格式
            RGB666_50_50 = 0x00, // GroupX[5:0] = Color[5:0]
            RGB666_50_05 = 0x10, // GroupX[5:0] = Color[0:5]
            RGB666_72_50 = 0x20, // GroupX[7:2] = Color[5:0]
            RGB666_72_05 = 0x30, // GroupX[7:2] = Color[0:5]

            // RGB888 格式
            RGB888_70_70 = 0x40, // GroupX[7:0] = Color[7:0]
            RGB888_70_07 = 0x50, // GroupX[7:0] = Color[0:7]
        };

        enum class Rgb_Order
        {
            RGB = 0x00, // Red(0) - Green(1) - Blue(2)
            RBG = 0x01, // Red(0) - Blue(1) - Green(2)
            GRB = 0x02, // Green(0) - Red(1) - Blue(2)
            GBR = 0x03, // Green(0) - Blue(1) - Red(2)
            BRG = 0x04, // Blue(0) - Red(1) - Green(2)
            BGR = 0x05  // Blue(0) - Green(1) - Red(2)
        };

        enum class Test_Mode
        {
            DISABLE = 0x00,
            MONOCHROME = 0x18,
            BORDER = 0x28,
            CHESS_BOARD = 0x38,
            COLOR_BAR = 0x48,
            COLOR_SWITCHING = 0x58
        };

        int32_t _rst;

        double _fps;

    public:
        Icn6211(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst) {}

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint16_t get_device_id(void);

        /**
         * @brief 断言分析接口参数是否正确
         * @param &params 输入Interface_Params配置的接口参数，超出范围自动修改为极限值并返回false
         * @return
         * @Date 2026-01-16 17:34:46
         */
        bool assert_interface_params_out_of_range(Interface_Params &params);

        /**
         * @brief 配置接口参数
         * @param params 使用Interface_Params::配置
         * @return
         * @Date 2026-01-16 17:34:29
         */
        bool config_interface_params(Interface_Params params);

        /**
         * @brief 配置信号极性
         * @param de de 信号极性
         * @param vsync vsync 信号极性
         * @param hsync hsync 信号极性
         * @return
         * @Date 2026-01-16 13:57:13
         */
        bool set_polarity_enable(bool de, bool vsync, bool hsync);

        /**
         * @brief 设置mipi总线lane个数
         * @param lane 值范围：1~4
         * @return
         * @Date 2026-01-16 17:33:55
         */
        bool set_mipi_lane(uint8_t lane);

        /**
         * @brief 设置rgb输出格式
         * @param format 使用Rgb_Format::配置
         * @param order 使用Rgb_Order::配置
         * @param rfc_enable [true]：开启，[false]：关闭
         * @return
         * @Date 2026-01-16 17:33:14
         */
        bool set_rgb_output_format(Rgb_Format format, Rgb_Order order, bool rfc_enable = false);

        /**
         * @brief 设置测试模式
         * @param mode 使用Test_Mode::配置
         * @return
         * @Date 2026-01-16 17:32:50
         */
        bool set_test_mode(Test_Mode mode);

        /**
         * @brief 设置芯片使能
         * @param enable [true]：开启，[false]：关闭
         * @return
         * @Date 2026-01-16 16:09:57
         */
        bool set_chip_enable(bool enable);
    };
}