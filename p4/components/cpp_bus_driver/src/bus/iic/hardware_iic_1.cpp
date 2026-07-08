/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-02-13 15:04:49
 * @LastEditTime: 2025-09-26 10:39:30
 * @License: GPL 3.0
 */
#include "hardware_iic_1.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Hardware_Iic_1::begin(uint32_t freq_hz, uint16_t address)
    {
        if ((_bus_handle != nullptr) && (_device_handle != nullptr))
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "hardware_iic_1 has been initialized\n");
            return true;
        }

        if (freq_hz == static_cast<uint32_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            freq_hz = DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config address: %#X\n", address);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _sda: %d\n", _sda);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config _scl: %d\n", _scl);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iic config freq_hz: %d hz\n", freq_hz);

        if (_bus_handle == nullptr)
        {
            const i2c_master_bus_config_t bus_config =
                {
                    .i2c_port = _port, // 选择I2C端口号，I2C_NUM_0表示使用I2C0
                    .sda_io_num = static_cast<gpio_num_t>(_sda),
                    .scl_io_num = static_cast<gpio_num_t>(_scl),
                    .clk_source = I2C_CLK_SRC_DEFAULT, // 使用默认的I2C时钟源
                    .glitch_ignore_cnt = 7,            // 设置滤波器忽略的毛刺周期为7个I2C模块时钟周期
                    .intr_priority = 0,                // 设置中断优先级
                    .trans_queue_depth = 0,            // 设置传输队列深度
                    .flags =
                        {
                            .enable_internal_pullup = 1, // 启用内部上拉电阻
                            .allow_pd = 0                // 不允许在睡眠模式下备份/恢复I2C寄存器
                        },
                };

            esp_err_t assert = i2c_new_master_bus(&bus_config, &_bus_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_new_master_bus fail (error code: %#X)\n", assert);
                return false;
                // assert = i2c_master_get_bus_handle(_port, &_iic_bus);
                // if (assert != ESP_OK)
                // {
                //     assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_get_bus_handle fail (error code: %#X)\n", assert);
                //     return false;
                // }
            }
        }

        if (address == static_cast<uint16_t>(DEFAULT_CPP_BUS_DRIVER_VALUE))
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "address is null\n");
            return false;
        }

        if (_device_handle == nullptr)
        {
            const i2c_device_config_t device_config =
                {
                    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                    .device_address = address,
                    .scl_speed_hz = freq_hz,
                    .scl_wait_us = 0,
                    .flags =
                        {
                            .disable_ack_check = 1,
                        },
                };

            esp_err_t assert = i2c_master_bus_add_device(_bus_handle, &device_config, &_device_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_bus_add_device fail (error code: %#X)\n", assert);
                return false;
            }
        }

        _freq_hz = freq_hz;
        _address = address;

        return true;
    }

    bool Hardware_Iic_1::read(uint8_t *data, size_t length)
    {
        esp_err_t assert = i2c_master_receive(_device_handle, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_receive fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_1::write(const uint8_t *data, size_t length)
    {
        esp_err_t assert = i2c_master_transmit(_device_handle, data, length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }
    bool Hardware_Iic_1::write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length)
    {
        esp_err_t assert = i2c_master_transmit_receive(_device_handle, write_data, write_length, read_data, read_length, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2c_master_transmit_receive fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_1::probe(const uint16_t address)
    {
        esp_err_t assert = i2c_master_probe(_bus_handle, address, DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            // assert_log(Log_Level::INFO, __FILE__, __LINE__, "i2c_master_probe fail (error code: %#X)\n", assert);
            return false;
        }

        return true;
    }

    bool Hardware_Iic_1::set_bus_handle(i2c_master_bus_handle_t bus_handle)
    {
        if (bus_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "bus_handle fail (bus_handle == nullptr)\n");
            return false;
        }

        _bus_handle = bus_handle;

        return true;
    }

    i2c_master_bus_handle_t Hardware_Iic_1::get_bus_handle(void)
    {
        return _bus_handle;
    }
#endif
}