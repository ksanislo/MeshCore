/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date 2025-07-09 09:15:31
 * @LastEditTime: 2025-07-11 10:15:01
 * @License: GPL 3.0
 */

#pragma once

#include "../chip_guide.h"

namespace Cpp_Bus_Driver
{
#define GT9895_TOUCH_DEVICE_DEFAULT_ADDRESS_1 0x5D

    class Gt9895 : public Iic_Guide
    {
    private:
        static constexpr uint8_t MAX_TOUCH_FINGER_COUNT = 10;

        static constexpr uint8_t TOUCH_POINT_ADDRESS_OFFSET = 8;
        static constexpr uint8_t SINGLE_TOUCH_POINT_DATA_SIZE = 8;

        enum class Cmd
        {
            // 触摸信息开始地址
            RO_TOUCH_INFO_START_ADDRESS = 0x00010308,

            WO_SLEEP_START_ADDRESS = 0x00010174,
        };

        // 触摸xy坐标缩放处理比例
        float _x_scale_factor, _y_scale_factor;

        int32_t _rst;

    public:
        struct Touch_Info
        {
            uint8_t finger_id = -1;      // 触摸手指id
            uint16_t x = -1;             // x 坐标
            uint16_t y = -1;             // y 坐标
            uint8_t pressure_value = -1; // 触摸压力值
        };

        struct Touch_Point
        {
            uint8_t finger_count = -1;    // 触摸手指总数
            bool edge_touch_flag = false; // 边缘触摸标志

            std::vector<struct Touch_Info> info;
        };

        Gt9895(std::shared_ptr<Bus_Iic_Guide> bus, int16_t address, float x_scale_factor = 1.0, float y_scale_factor = 1.0,
               int32_t rst = DEFAULT_CPP_BUS_DRIVER_VALUE)
            : Iic_Guide(bus, address), _x_scale_factor(x_scale_factor), _y_scale_factor(y_scale_factor), _rst(rst)
        {
        }

        bool begin(int32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        /**
         * @brief 获取触摸总数
         * @return
         * @Date 2025-07-09 09:15:31
         */
        uint8_t get_finger_count(void);

        /**
         * @brief 获取单指触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @param finger_num 要获取的触摸点
         * @return [true]：获取的触摸点和finger_num相同 [false]：获取错误或者获取的触摸点和finger_num不相同
         * @Date 2025-07-09 09:15:31
         */
        bool get_single_touch_point(Touch_Point &tp, uint8_t finger_num = 1);

        /**
         * @brief 获取多个触控的触摸点信息
         * @param &tp 使用结构体Touch_Point::配置触摸点结构体
         * @return  [true]：获取的手指数大于0 [false]：获取错误或者获取的手指数为0
         * @Date 2025-07-09 09:15:31
         */
        bool get_multiple_touch_point(Touch_Point &tp);

        /**
         * @brief 获取边缘检测
         * @return  [true]：屏幕边缘检测触发 [false]：屏幕边缘检测未触发
         * @Date 2025-07-09 09:15:31
         */
        bool get_edge_touch(void);

        /**
         * @brief 设置睡眠
         * @return
         * @Date 2025-07-11 10:07:49
         */
        bool set_sleep();
    };
}