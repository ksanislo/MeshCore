/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:47:28
 * @LastEditTime: 2025-09-04 10:17:10
 * @License: GPL 3.0
 */
#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    class Software_Iic : public Bus_Iic_Guide
    {
    public:
        // iic通信中，应答(ack)是低电平(0)，非应答(nack)是高电平(1)
        enum class Ack_Bit
        {
            ACK = 0,
            NACK,
        };

        int32_t _sda, _scl;
        uint16_t _address = DEFAULT_CPP_BUS_DRIVER_VALUE;
        uint32_t _freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE;
        uint32_t _transmit_delay_us = 0;

        Software_Iic(int32_t sda, int32_t scl)
            : _sda(sda), _scl(scl)
        {
        }

        bool begin(uint32_t freq_hz = DEFAULT_CPP_BUS_DRIVER_VALUE, uint16_t address = DEFAULT_CPP_BUS_DRIVER_VALUE) override;

        bool start_transmit(void) override;
        bool read(uint8_t *data, size_t length) override;
        bool write(const uint8_t *data, size_t length) override;
        bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) override;
        bool stop_transmit(void) override;

        bool probe(const uint16_t address) override;

        bool write_byte(uint8_t data);
        bool read_byte(uint8_t *data);
        bool wait_ack(void);

        /**
         * @brief 写应答
         * @param ack 使用Ack_Bit::进行配置
         * @return
         * @Date 2025-08-22 15:44:49
         */
        bool write_ack(Ack_Bit ack);
    };
#endif
}
