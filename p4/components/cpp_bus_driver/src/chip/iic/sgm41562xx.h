/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-11-08 14:33:10
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define SGM41562XX_DEVICE_DEFAULT_ADDRESS 0x03

    class Sgm41562xx : public Iic_Guide
    {
    private:
        static constexpr uint8_t DEVICE_ID = 0x04;

        enum class Cmd
        {
            RO_DEVICE_ID = 0x0B,

            RW_INPUT_SOURCE_CONTROL = 0x00,             // 输入源控制寄存器
            RW_POWER_ON_CONFIGURATION,                  // 上电配置寄存器
            RW_CHARGE_CURRENT_CONTROL,                  // 充电电流控制寄存器
            RW_DISCHARGE_TERMINATION_CURRENT,           // 放电/终止电流寄存器
            RW_CHARGE_VOLTAGE_CONTROL,                  // 充电电压控制寄存器
            RW_CHARGE_TERMINATION_TIMER_CONTROL,        // 充电终止/定时器控制寄存器
            RW_MISCELLANEOUS_OPERATION_CONTROL,         // 杂项操作控制寄存器
            RW_SYSTEM_VOLTAGE_REGULATION,               // 系统电压调节寄存器
            RD_SYSTEM_STATUS,                           // 系统状态寄存器
            RD_FAULT,                                   // 故障寄存器
            RW_IIC_ADDRESS_MISCELLANEOUS_CONFIGURATION, // IIC地址及杂项配置寄存器
        };

        static constexpr const uint8_t _init_list[] =
            {
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CHARGE_CURRENT_CONTROL), 0B11001111, // 重置寄存器
                // static_cast<uint8_t>(Init_List_Cmd::DELAY_MS), 120,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_SYSTEM_VOLTAGE_REGULATION), 0B10110111,       // 关闭PCB OTP
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), 0B01000000, // 关闭NTC

                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_MISCELLANEOUS_OPERATION_CONTROL), 0B11011111, // 屏蔽INT
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_IIC_ADDRESS_MISCELLANEOUS_CONFIGURATION), 0B01100001, // 充电电流权重限制

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CHARGE_TERMINATION_TIMER_CONTROL), 0B00011010, // 关闭看门狗功能
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), 0B10100100,           // 开启电池充电功能

                //  static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_POWER_ON_CONFIGURATION), 0B10101100,        // 关闭电池充电功能

                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), 0B01000000 // 关闭输入电流限制

                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RD_SYSTEM_STATUS), 0B00100000, // 添加200ma电流阈值到输入电流限制中（仅在电流限制模式有效）

            };

        int32_t _rst;

    public:
        enum class Charge_Status
        {
            NOT_CHARGING = 0,
            PRECHARGE,
            CHARGE,
            CHARGE_DONE,
        };

        enum class Enter_Ship_Time
        {
            WAIT_1S = 0B00000000,
            WAIT_2S = 0B01000000,
            WAIT_4S = 0B10000000,
            WAIT_8S = 0B11000000,
        };

        struct Irq_Status // 中断状态
        {
            bool Input_power_fault_flag = false;             // 输入电源故障标志
            bool thermal_shutdown_flag = false;              // 过热关断标志
            bool battery_over_voltage_fault_flag = false;    // 电池过压故障标志
            bool safety_timer_expiration_fault_flag = false; // 安全定时器到期故障标志
            bool ntc_exceeding_hot_flag = false;             // ntc过热标志
            bool ntc_exceeding_cold_flag = false;            // ntc过冷标志
        };

        struct Chip_Status // 芯片状态
        {
            bool watchdog_expiration_flag = false;                     // 看门狗超时标志
            Charge_Status charge_status = Charge_Status::NOT_CHARGING; // 充电状态标志
            bool device_in_power_path_management_mode_flag = false;    // 设备在电源路径管理模式标志
            bool input_power_status_flag = false;                      // 输入电源状态标志（[1] = 电源是好的 [0] = 电源是不好的）
            bool thermal_regulation_status_flag = false;               // 热调节状态
        };

        Sgm41562xx(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;
        bool end() override;

        uint8_t get_device_id(void);

        /**
         * @brief 获取中断标志
         * @return
         * @Date 2025-07-17 13:49:36
         */
        uint8_t get_irq_flag(void);

        /**
         * @brief 中断解析，详细请参考SGM41562手册 Table 13. REG09 Register Details
         * @param irq_flag 解析状态语句，由get_irq_flag()函数获取
         * @param &status 使用Irq_Status结构体配置，相应位自动置位
         * @return
         * @Date 2025-07-17 13:59:38
         */
        bool parse_irq_status(uint8_t irq_flag, Irq_Status &status);

        /**
         * @brief 设置充电使能
         * @param enable [true]：打开充电 [false]：关闭充电
         * @return
         * @Date 2025-07-17 14:49:29
         */
        bool set_charge_enable(bool enable);

        /**
         * @brief 获取芯片状态
         * @return
         * @Date 2025-07-17 15:05:29
         */
        uint8_t get_chip_status(void);

        /**
         * @brief 芯片状态解析，详细请参考SGM41562手册表格 Table 12. REG08 Register Details
         * @param chip_flag 解析状态语句，由get_chip_status()函数获取
         * @param &status 使用Chip_Status结构体配置，相应位自动置位
         * @return
         * @Date 2025-07-17 15:03:59
         */
        bool parse_chip_status(uint8_t chip_flag, Chip_Status &status);

        /**
         * @brief 设置开启运输模式
         * @param enable [true]：开启运输模式 [false]：关闭运输模式
         * @return
         * @Date 2025-07-19 16:14:57
         */
        bool set_ship_mode_enable(bool enable);

        /**
         * @brief 设置进入运输模式的时间
         * @param time 使用Enter_Ship_Time::进行配置
         * @return
         * @Date 2025-11-08 14:29:54
         */
        bool set_enter_ship_time(Enter_Ship_Time time);
    };
}