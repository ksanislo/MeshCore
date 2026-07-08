/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-09-24 15:35:00
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define GZ030PCC02_DEVICE_DEFAULT_ADDRESS 0x28

    class Gz030pcc0x : public Iic_Guide
    {
    private:
        enum class Cmd
        {
            RW_INTERNAL_TEST_MODE_INPUT_DATA_FORMAT = 0x0001,
            RW_HORIZONTAL_VERTICAL_MIRROR,

            RW_DISPLAY_BRIGHTNESS = 0x5800,

            RO_TEMPERATURE_READING = 0x3001,
        };

        static constexpr const uint16_t _init_list[] =
            {
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x08,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                // MIPI 总线的 lane 数量设置为 4 lane
                // static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x03,

                // MIPI 总线的 lane 数量设置为 2 lane
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x5F00, 0x22,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x9F00, 0x06,

                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x10,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x07,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x10,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x03,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x0F,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x14,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x03,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x02,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x08,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x11,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6900, 0x04,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6901, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6800, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6801, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6802, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6803, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x70,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6C00, 0x00,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7D02, 0xC0,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7E03, 0x01,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x6F00, 0x30,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x7402, 0x0D,
                static_cast<uint16_t>(Init_List_Cmd::WRITE_C16_D8), 0x9F01, 0x10};

        int32_t _rst;

    public:
        enum class Data_Format
        {
            RGB888 = 0B00000011,
            INTERNAL_TEST_MODE = 0B00000101, // 内部测试图模式
        };

        enum class Internal_Test_Mode
        {
            REGISTER_CONTROL_RGB = 0B00000000,
            PURE_WHITE_FIELD = 0B00100000,
            PURE_RED_FIELD = 0B01000000,
            PURE_GREEN_FIELD = 0B01100000,
            PURE_BLUE_FIELD = 0B10000000,
            GRAYSCALE_IMAGE = 0B10100000,
            COLOR_BAR = 0B11000000,
            CHECKERBOARD = 0B11100000,
        };

        enum class Show_Direction
        {
            NORMAL = 0B00000000,
            HORIZONTAL_MIRROR = 0B00000001,          // 水平镜像
            VERTICAL_MIRROR = 0B00000010,            // 垂直镜像
            HORIZONTAL_VERTICAL_MIRROR = 0B00000011, // 水平垂直镜像
        };

        Gz030pcc0x(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 获取温度
         * @return 以°C为单位
         * @Date 2025-08-15 11:41:42
         */
        float get_temperature_celsius(void);

        /**
         * @brief 设置数据模式
         * @param format 使用Data_Format::配置
         * @return
         * @Date 2025-08-15 13:50:37
         */
        bool set_data_format(Data_Format format);

        /**
         * @brief 内部测试模式
         * @param mode 使用Internal_Test_Mode::配置
         * @return
         * @Date 2025-08-15 14:05:28
         */
        bool set_internal_test_mode(Internal_Test_Mode mode);

        /**
         * @brief 设置显示方向
         * @param direction 使用Show_Direction::配置
         * @return
         * @Date 2025-09-18 10:52:45
         */
        bool set_show_direction(Show_Direction direction);

        /**
         * @brief 设置亮度
         * @param value 值范围：0~255
         * @return
         * @Date 2025-09-18 11:05:59
         */
        bool set_brightness(uint8_t value);
    };
}