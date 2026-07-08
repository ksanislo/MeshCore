/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:51:36
 * @LastEditTime: 2026-01-20 13:35:43
 * @License: GPL 3.0
 */

#pragma once

#include "../config.h"

namespace Cpp_Bus_Driver
{
    class Bus_Iic_Guide : public Tool
    {
    public:
        Bus_Iic_Guide()
        {
        }

        virtual bool begin(uint32_t freq_hz, uint16_t address) = 0;
        virtual bool read(uint8_t *data, size_t length) = 0;
        virtual bool write(const uint8_t *data, size_t length) = 0;
        virtual bool write_read(const uint8_t *write_data, size_t write_length, uint8_t *read_data, size_t read_length) = 0;
        virtual bool probe(const uint16_t address) = 0;

        virtual bool end(void);

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        virtual i2c_cmd_handle_t cmd_link_create(void);
        virtual bool start_transmit(i2c_cmd_handle_t cmd_handle, i2c_rw_t rw, bool ack_en = true);
        virtual bool write(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en = true);
        virtual bool write(i2c_cmd_handle_t cmd_handle, const uint8_t *data, size_t data_len, bool ack_en = true);
        virtual bool read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, i2c_ack_type_t ack = I2C_MASTER_LAST_NACK);
        virtual bool stop_transmit(i2c_cmd_handle_t cmd_handle);

        virtual bool start_transmit(void);
        virtual bool stop_transmit(void);
#endif

        bool read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length = 1);
        bool read(const uint16_t write_c16, uint8_t *read_data, size_t read_data_length = 1);
        bool write(const uint8_t write_c8, const uint8_t write_d8);
        bool write(const uint8_t write_c8, const uint16_t write_d16, Endian endian = Endian::BIG);
        bool write(const uint16_t write_c16, const uint8_t write_d8);
        bool write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length);

        bool scan_7bit_address(std::vector<uint8_t> *address);
    };

    class Bus_Iis_Guide : public Tool
    {
    public:
        Bus_Iis_Guide()
        {
        }
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        virtual bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) = 0;

        virtual size_t read(void *data, size_t byte) = 0;
        virtual size_t write(const void *data, size_t byte) = 0;
#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
        virtual bool begin(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz, nrf_i2s_swidth_t data_bit_width, nrf_i2s_channels_t channel) = 0;

        /**
         * @brief 数据流传输开始
         * @param *write_data 写数据流缓存指针，如果为nullptr表示不写入数据，*write_data需要使用ram分配的内存
         * @param *read_data 读数据流缓存指针，如果为nullptr表示不读取数据，*read_data需要使用ram分配的内存
         * @param max_data_length 数据流缓存最大长度
         * @return
         * @Date 2025-08-29 17:49:07
         */
        virtual bool start_transmit(uint32_t *write_data, uint32_t *read_data, size_t max_data_length) = 0;

        /**
         * @brief 停止数据流传输
         * @return
         * @Date 2025-08-29 17:51:03
         */
        virtual void stop_transmit(void) = 0;

        /**
         * @brief 设置下一个读取的指针
         * @param *data 数据指针
         * @return
         * @Date 2025-08-29 17:52:08
         */
        virtual bool set_next_read_data(uint32_t *data) = 0;

        /**
         * @brief 设置下一个写入的指针
         * @param *data 数据指针
         * @return
         * @Date 2025-08-29 17:52:08
         */
        virtual bool set_next_write_data(uint32_t *data) = 0;

        /**
         * @brief 获取读取事件标志
         * @return [true]：有数据可读，[false]：无数据可读
         * @Date 2025-08-29 17:52:43
         */
        virtual bool get_read_event_flag(void) = 0;

        /**
         * @brief 获取写入事件标志
         * @return [true]：可以继续写入数据，[false]：不能写入数据
         * @Date 2025-08-29 17:52:43
         */
        virtual bool get_write_event_flag(void) = 0;

        virtual void end(void) = 0;
#endif
    };

    class Bus_Spi_Guide : public Tool
    {
    public:
        Bus_Spi_Guide()
        {
        }

        virtual bool begin(int32_t freq_hz, int32_t cs) = 0;
        virtual bool write(const void *data, size_t byte) = 0;
        virtual bool read(void *data, size_t byte) = 0;
        virtual bool write_read(const void *write_data, void *read_data, size_t data_byte) = 0;

        bool read(const uint8_t write_c8, uint8_t *read_d8);
        bool read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length);
        bool write(const uint8_t write_c8);
        bool write(const uint8_t write_c8, const uint8_t write_d8);
        bool write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length);

        /**
         * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | 0xAA(等待码，有可能有有可能没有，一般为 0xAA) | read_data(8 bit)(要读出的数据) | read_data(8 bit)(要读出的数据) | ......]
         * @param write_c8 一般为命令位
         * @param write_c16 一般为寄存器地址位
         * @param *read_data 要读出数据的指针
         * @param read_data_length 要读出的数据长度
         * @return
         * @Date 2025-01-17 13:53:33
         */
        bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data, size_t read_data_length);

        bool read(const uint8_t write_c8_1, const uint8_t write_c8_2, uint8_t *read_data, size_t read_data_length);

        bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data);

        /**
         * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | write_data(8 bit)(要写入的数据) | write_data(8 bit)(要写入的数据) | ......]
         * @param write_c8 一般为命令位
         * @param write_c16 一般为寄存器地址位
         * @param *write_data 要写入数据的指针
         * @param write_data_length 要写入的数据长度
         * @return
         * @Date 2025-01-17 13:48:09
         */
        bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t *write_data, size_t write_data_length);

        bool write(const uint8_t write_c8_1, const uint8_t write_c8_2, const uint8_t *write_data, size_t write_data_length);

        bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t write_data);
    };

    class Bus_Qspi_Guide : public Tool
    {
    public:
        Bus_Qspi_Guide()
        {
        }

        virtual bool begin(int32_t freq_hz, int32_t cs) = 0;
        // virtual bool write(const uint16_t cmd, const uint64_t addr, const uint8_t *data = NULL, size_t byte = 0, uint32_t flags = static_cast<uint32_t>(NULL)) = 0;
        virtual bool write(const void *data, size_t byte, uint32_t flags = static_cast<uint32_t>(NULL), bool cs_keep_active = false) = 0;

        // bool read(const uint8_t write_c8, uint8_t *read_d8);
        // bool read(const uint8_t write_c8, uint8_t *read_data, size_t read_data_length);
        // bool write(const uint32_t write_c32);
        // bool write(const uint16_t cmd, const uint64_t addr, const uint8_t write_d8);
        // bool write(const uint8_t write_c8, const uint8_t *write_data, size_t write_data_length);

        // /**
        //  * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | 0xAA(等待码，有可能有有可能没有，一般为 0xAA) | read_data(8 bit)(要读出的数据) | read_data(8 bit)(要读出的数据) | ......]
        //  * @param write_c8 一般为命令位
        //  * @param write_c16 一般为寄存器地址位
        //  * @param *read_data 要读出数据的指针
        //  * @param read_data_length 要读出的数据长度
        //  * @return
        //  * @Date 2025-01-17 13:53:33
        //  */
        // bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data, size_t read_data_length);

        // bool read(const uint8_t write_c8_1, const uint8_t write_c8_2, uint8_t *read_data, size_t read_data_length);

        // bool read(const uint8_t write_c8, const uint16_t write_c16, uint8_t *read_data);

        // /**
        //  * @brief 传输数据结构 [CMD(8 bit) | REG(16 bit) | write_data(8 bit)(要写入的数据) | write_data(8 bit)(要写入的数据) | ......]
        //  * @param write_c8 一般为命令位
        //  * @param write_c16 一般为寄存器地址位
        //  * @param *write_data 要写入数据的指针
        //  * @param write_data_length 要写入的数据长度
        //  * @return
        //  * @Date 2025-01-17 13:48:09
        //  */
        // bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t *write_data, size_t write_data_length);

        // bool write(const uint8_t write_c8_1, const uint8_t write_c8_2, const uint8_t *write_data, size_t write_data_length);

        // bool write(const uint8_t write_c8, const uint16_t write_c16, const uint8_t write_data);
    };

    class Bus_Uart_Guide : public Tool
    {
    public:
        Bus_Uart_Guide()
        {
        }

        virtual bool begin(int32_t baud_rate = DEFAULT_CPP_BUS_DRIVER_VALUE) = 0;

        virtual int32_t read(void *data, uint32_t length) = 0;
        virtual int32_t write(const void *data, size_t length) = 0;

        virtual size_t get_rx_buffer_length(void) = 0;
        virtual bool clear_rx_buffer_data(void) = 0;
        virtual bool set_baud_rate(uint32_t baud_rate) = 0;
        virtual uint32_t get_baud_rate(void) = 0;
    };

    class Bus_Sdio_Guide : public Tool
    {
    public:
        Bus_Sdio_Guide()
        {
        }

        virtual bool begin(int32_t freq_hz) = 0;
        virtual bool wait_interrupt(uint32_t timeout_ms) = 0;
        virtual bool read(uint32_t function, uint32_t write_c32, void *data, size_t byte) = 0;
        virtual bool read(uint32_t function, uint32_t write_c32, uint8_t *data) = 0;
        virtual bool read_block(uint32_t function, uint32_t write_c32, void *data, size_t byte) = 0;
        virtual bool write(uint32_t function, uint32_t write_c32, const void *data, size_t byte) = 0;
        virtual bool write(uint32_t function, uint32_t write_c32, uint8_t data, uint8_t *read_d8_verify = NULL) = 0;
        virtual bool write_block(uint32_t function, uint32_t write_c32, const void *data, size_t byte) = 0;
    };
}