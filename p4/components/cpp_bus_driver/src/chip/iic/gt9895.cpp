/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date 2025-07-09 09:15:31
 * @LastEditTime: 2025-11-22 15:15:47
 * @License: GPL 3.0
 */
#include "gt9895.h"

namespace Cpp_Bus_Driver
{
    bool Gt9895::begin(int32_t freq_hz)
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
            // return false;
        }

        return true;
    }

    uint8_t Gt9895::get_finger_count(void)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
            };

        uint8_t buffer_2[3] = {0};

        if (_bus->write_read(buffer, 4, buffer_2, 3) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return -1;
        }

        return buffer_2[2];
    }

    bool Gt9895::get_single_touch_point(Touch_Point &tp, uint8_t finger_num)
    {
        if ((finger_num == 0) || (finger_num > MAX_TOUCH_FINGER_COUNT))
        {
            return false;
        }

        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
            };

        const uint8_t buffer_touch_point_size = TOUCH_POINT_ADDRESS_OFFSET + finger_num * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[buffer_touch_point_size] = {0};

        if (_bus->write_read(buffer, 4, buffer_2, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数小于需要读取的手指数就读取失败
        if (buffer_2[2] < finger_num)
        {
            if (buffer_2[0] == 0x84)
            {
                tp.edge_touch_flag = true;

                tp.finger_count = 1;

                Touch_Info buffer_ti;
                buffer_ti.finger_id = 0;

                tp.info.push_back(buffer_ti);

                return true;
            }
            else
            {
                return false;
            }
        }
        tp.finger_count = buffer_2[2];

        const uint8_t buffer_touch_point_offset = TOUCH_POINT_ADDRESS_OFFSET + finger_num * SINGLE_TOUCH_POINT_DATA_SIZE - SINGLE_TOUCH_POINT_DATA_SIZE;

        Touch_Info buffer_ti;
        buffer_ti.finger_id = (buffer_2[buffer_touch_point_offset] >> 4) + 1;
        buffer_ti.x = (buffer_2[buffer_touch_point_offset + 2] | static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 3]) << 8) * _x_scale_factor;
        buffer_ti.y = (buffer_2[buffer_touch_point_offset + 4] | static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 5]) << 8) * _y_scale_factor;
        buffer_ti.pressure_value = buffer_2[buffer_touch_point_offset + 6];

        tp.info.push_back(buffer_ti);

        if (buffer_2[0] == 0x84)
        {
            tp.edge_touch_flag = true;
        }
        else
        {
            tp.edge_touch_flag = false;
        }

        return true;
    }

    bool Gt9895::get_multiple_touch_point(Touch_Point &tp)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
            };

        const uint8_t buffer_touch_point_size = TOUCH_POINT_ADDRESS_OFFSET + MAX_TOUCH_FINGER_COUNT * SINGLE_TOUCH_POINT_DATA_SIZE;
        uint8_t buffer_2[buffer_touch_point_size] = {0};

        if (_bus->write_read(buffer, 4, buffer_2, buffer_touch_point_size) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        // 如果手指数为0或者大于最大触摸手指数
        if (buffer_2[2] == 0)
        {
            if (buffer_2[0] == 0x84)
            {
                tp.edge_touch_flag = true;

                tp.finger_count = 1;

                Touch_Info buffer_ti;
                buffer_ti.finger_id = 0;

                tp.info.push_back(buffer_ti);

                return true;
            }
            else
            {
                return false;
            }
        }
        else if (buffer_2[2] > MAX_TOUCH_FINGER_COUNT)
        {
            return false;
        }

        tp.finger_count = buffer_2[2];

        for (uint8_t i = 0; i < tp.finger_count; i++)
        {
            const uint8_t buffer_touch_point_offset = TOUCH_POINT_ADDRESS_OFFSET + i * SINGLE_TOUCH_POINT_DATA_SIZE;

            Touch_Info buffer_ti;
            buffer_ti.finger_id = (buffer_2[buffer_touch_point_offset] >> 4) + 1;
            buffer_ti.x = (buffer_2[buffer_touch_point_offset + 2] | static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 3]) << 8) * _x_scale_factor;
            buffer_ti.y = (buffer_2[buffer_touch_point_offset + 4] | static_cast<uint16_t>(buffer_2[buffer_touch_point_offset + 5]) << 8) * _y_scale_factor;
            buffer_ti.pressure_value = buffer_2[buffer_touch_point_offset + 6];

            tp.info.push_back(buffer_ti);
        }

        if (buffer_2[0] == 0x84)
        {
            tp.edge_touch_flag = true;
        }
        else
        {
            tp.edge_touch_flag = false;
        }

        return true;
    }

    bool Gt9895::get_edge_touch(void)
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::RO_TOUCH_INFO_START_ADDRESS)),
            };

        uint8_t buffer_2 = 0;

        if (_bus->write_read(buffer, 4, &buffer_2, 1) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write_read fail\n");
            return false;
        }

        if (buffer_2 != 0x84)
        {
            return false;
        }

        return true;
    }

    bool Gt9895::set_sleep()
    {
        uint8_t buffer[] =
            {
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::WO_SLEEP_START_ADDRESS) >> 24),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::WO_SLEEP_START_ADDRESS) >> 16),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::WO_SLEEP_START_ADDRESS) >> 8),
                static_cast<uint8_t>(static_cast<uint32_t>(Cmd::WO_SLEEP_START_ADDRESS)),

                0x00, 0x00, 0x04, 0x84, 0x88, 0x00};

        if (_bus->write(buffer, 10) == false)
        {
            assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
