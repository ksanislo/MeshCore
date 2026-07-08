/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 17:17:22
 * @LastEditTime: 2025-09-24 15:36:01
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define TCA8418_DEVICE_DEFAULT_ADDRESS 0x34

    class Tca8418 : public Iic_Guide
    {
    private:
        static constexpr uint8_t MAX_WIDTH_SIZE = 10;
        static constexpr uint8_t MAX_HEIGHT_SIZE = 8;

        enum class Cmd
        {
            RW_CONFIGURATION = 0x01,
            RW_INTERRUPT_STATUS,
            RW_KEY_LOCK_AND_EVENT_COUNTER,
            RO_KEY_EVENT,

            RO_GPIO_INTERRUPT_STATUS_START = 0x11,

            WO_GPIO_INT_EN1 = 0x1A, // GPIO中断使能1
            WO_GPIO_INT_EN2,        // GPIO中断使能2
            WO_GPIO_INT_EN3,        // GPIO中断使能3
            WO_KP_GPIO1,            // 按键或GPIO模式选择1
            WO_KP_GPIO2,            // 按键或GPIO模式选择2
            WO_KP_GPIO3,            // 按键或GPIO模式选择3

            WO_GPI_EM1 = 0x20, // GPI事件模式1
            WO_GPI_EM2,        // GPI事件模式2
            WO_GPI_EM3,        // GPI事件模式3
            WO_GPIO_DIR1,      // GPIO数据方向1
            WO_GPIO_DIR2,      // GPIO数据方向2
            WO_GPIO_DIR3,      // GPIO数据方向3
            WO_GPIO_INT_LVL1,  // GPIO边沿/电平检测1
            WO_GPIO_INT_LVL2,  // GPIO边沿/电平检测2
            WO_GPIO_INT_LVL3   // GPIO边沿/电平检测3

        };

        static constexpr const uint8_t _init_list[] =
            {
                // 开启寄存器读写自动递增
                // 开启fifo溢出自动栈队列循环
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::RW_CONFIGURATION), 0B10100000,

                // 设置默认全部引脚为输入
                //  static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR1), 0x00,
                //  static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR2), 0x00,
                //  static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_DIR3), 0x00,

                // 添加全部gpio事件到fifo
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPI_EM3), 0xFF,

                //  设置全部引脚中断为下降沿中断
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL1), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL2), 0x00,
                // static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_LVL3), 0x00,

                // 添加全部引脚到中断
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN1), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN2), 0xFF,
                static_cast<uint8_t>(Init_List_Cmd::WRITE_C8_D8), static_cast<uint8_t>(Cmd::WO_GPIO_INT_EN3), 0xFF};

        int32_t _rst;

    public:
        enum class Irq_Flag // 需要清除的中断请求参数，设置1代表清除
        {
            ALL = 0B00011111, // 全部中断
            CTRL_ALT_DEL_KEY_SEQUENCE = 0B00010000,
            FIFO_OVERFLOW = 0B00001000,
            KEYPAD_LOCK = 0B00000100,
            GPIO_INTERRUPT = 0B00000010,
            KEY_EVENTS = 0B00000001,
        };

        enum class Irq_Mask // 中断掩码，设置中断开关
        {
            ALL = 0B00001111, // 全部中断
            FIFO_OVERFLOW = 0B00001000,
            KEYPAD_LOCK = 0B00000100,
            GPIO_INTERRUPT = 0B00000010,
            KEY_EVENTS = 0B00000001,
        };

        enum class Event_Type // 事件类型
        {
            KEYPAD, // 0~80为触发键盘事件
            GPIO,   // 97~114为触发GPIO事件
        };

        struct Touch_Info
        {
            uint8_t num = -1; // 序号
            Event_Type event_type = Event_Type::KEYPAD;
            bool press_flag = false; // 按压标志
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1; // 触摸手指总数

            std::vector<struct Touch_Info> info;
        };

        struct Irq_Status // 中断状态
        {
            bool ctrl_alt_del_key_sequence_flag = false; // ctrl+alt+del 这一系统级组合键的同时按下动作标志
            bool fifo_overflow_flag = false;             // fifo溢出中断标志
            bool keypad_lock_flag = false;               // 键盘锁定中断标志
            bool gpio_interrupt_flag = false;            // gpio中断标志
            bool key_events_flag = false;                // 按键事件标志
        };

        struct Touch_Position
        {
            uint8_t x = -1; // x 坐标
            uint8_t y = -1; // y 坐标
        };

        Tca8418(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        /**
         * @brief 设置键盘扫描模式开窗大小
         * @param x 开窗点x坐标，值范围（0~9）
         * @param y 开窗点y坐标，值范围（0~7）
         * @param w 开窗长度，值范围（0~9）
         * @param h 开窗高度，值范围（0~7）
         * @return
         * @Date 2025-07-30 13:42:08
         */
        bool set_keypad_scan_window(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-07-30 15:23:03
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-07-30 16:16:10
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取中断标志
         * @return
         * @Date 2025-07-30 16:39:43
         */
        uint8_t get_irq_flag(void);

        /**
         * @brief 中断解析，详细请参考TCA8418手册 8.6.2.2 Interrupt Status Register, INT_STAT (Address 0x02)
         * @param irq_flag 解析状态语句，由get_irq_flag()函数获取
         * @param &status 使用Irq_Status结构体配置，相应位自动置位
         * @return
         * @Date 2025-07-30 16:39:56
         */
        bool parse_irq_status(uint8_t irq_flag, Irq_Status &status);

        /**
         * @brief 清除中断标志位
         * @param flag 使用Irq_Flag::配置，需要清除的标志，设置1为清除标志
         * @return
         * @Date 2025-07-30 16:55:59
         */
        bool clear_irq_flag(Irq_Flag flag);

        /**
         * @brief 获取gpio中断同时清除gpio中断标志
         * @return
         * @Date 2025-07-31 09:05:05
         */
        uint32_t get_clear_gpio_irq_flag(void);

        /**
         * @brief 设置中断引脚模式
         * @param mode 由Irq_Mask::配置，选择需要开启的中断引脚位
         * @return
         * @Date 2025-07-31 10:16:03
         */
        bool set_irq_pin_mode(Irq_Mask mode);

        /**
         * @brief 用于解码触摸号数为x、y坐标
         * @param num 结构体Touch_Info中的num值，解码前需要先获取该值
         * @param &position 使用Touch_Position::配置，解码后的坐标信息
         * @return
         * @Date 2025-07-31 13:39:08
         */
        bool parse_touch_num(uint8_t num, Touch_Position &position);
    };
}