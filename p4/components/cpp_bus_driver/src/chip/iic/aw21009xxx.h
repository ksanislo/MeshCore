/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-09-24 14:04:37
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define AW21009QNR_DEVICE_DEFAULT_ADDRESS_1 0x20
#define AW21009QNR_DEVICE_DEFAULT_ADDRESS_2 0x21
#define AW21009QNR_DEVICE_DEFAULT_ADDRESS_3 0x24
#define AW21009QNR_DEVICE_DEFAULT_ADDRESS_4 0x25

    class Aw21009xxx : public Iic_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x12;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x70,

            RW_GLOBAL_CONTROL_REGISTER = 0x20,
            RW_BRIGHTNESS_CONTROL_REGISTER_START,

            WO_UPDATE_REGISTER = 0x45,
            RW_SCALING_REGISTER_START,

            RW_GLOBAL_CURRENT_CONTROL_REGISTER = 0x58,

            RW_RESET_REGISTER = 0x70,

        };

        static constexpr const uint8_t _init_list[] =
            {
                // 复位
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_RESET_REGISTER), 0,

                static_cast<uint8_t>(Init_List_Cmd::DELAY_MS), 10,

                // 启动自动省电模式，12位pwm消抖模式，使能芯片
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_GLOBAL_CONTROL_REGISTER), 0B10000111,

                // 设置全局最大电流为最大
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_GLOBAL_CURRENT_CONTROL_REGISTER), 0B11111111,

                // 设置所有通道的电流限制为最大
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START), 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 1, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 2, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 3, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 4, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 5, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 6, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 7, 0B11111111,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SCALING_REGISTER_START) + 8, 0B11111111,

                // 更新寄存器
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_UPDATE_REGISTER), 0};

        int32_t _rst;

    public:
        enum class Led_Channel
        {
            LED1 = 0,
            LED2,
            LED3,
            LED4,
            LED5,
            LED6,
            LED7,
            LED8,
            LED9,

            ALL,
        };

        Aw21009xxx(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        uint8_t get_device_id(void);

        /**
         * @brief 设置自动省电模式
         * @param enable [true]：开启自动省电模式  [false]：关闭自动省电模式
         * @return
         * @Date 2025-09-24 11:24:55
         */
        bool set_auto_power_save(bool enable);

        /**
         * @brief 设置芯片使能
         * @param enable [true]：开启芯片  [false]：关闭芯片
         * @return
         * @Date 2025-09-24 11:38:38
         */
        bool set_chip_enable(bool enable);

        /**
         * @brief 设置亮度
         * @param channel 使用Led_Channel::配置
         * @param value 值范围：0~4095
         * @return
         * @Date 2025-09-24 11:39:18
         */
        bool set_brightness(Led_Channel channel, uint16_t value);

        /**
         * @brief 设置亮度
         * @param channel 使用Led_Channel::配置
         * @param value 值范围：0~255
         * @return
         * @Date 2025-09-24 11:39:18
         */
        bool set_current_limit(Led_Channel channel, uint8_t value);

        /**
         * @brief 设置全局电流限制
         * @param value 值范围：0~255
         * @return
         * @Date 2025-09-24 13:37:12
         */
        bool set_global_current_limit(uint8_t value);
    };
}