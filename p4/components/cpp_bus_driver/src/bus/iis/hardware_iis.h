/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-01-20 13:41:05
 * @License: GPL 3.0
 */

#pragma once

#include "../bus_guide.h"

namespace Cpp_Bus_Driver
{

    class Hardware_Iis : public Bus_Iis_Guide
    {
    private:
        int32_t _data_in, _data_out;
        int32_t _ws_lrck, _bclk, _mclk;
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        i2s_port_t _port;
        i2s_chan_handle_t _chan_tx_handle = nullptr;
        i2s_chan_handle_t _chan_rx_handle = nullptr;
#endif

        uint16_t _mclk_multiple = -1;
        uint32_t _sample_rate_hz = -1;
        uint8_t _data_bit_width = -1;

    public:
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
        enum class Data_Mode
        {
            INPUT,  // 输入模式
            OUTPUT, // 输出模式

            INPUT_OUTPUT, // 输入输出共有
        };

        enum class Iis_Mode
        {
            STD, // 标准模式
            PDM, // pdm模式
        };

        Data_Mode _data_mode;
        Iis_Mode _iis_mode;
        i2s_slot_mode_t _slot_mode_in;
        i2s_slot_mode_t _slot_mode_out;

        // 配置输入和输出设备
        Hardware_Iis(int32_t data_in, int32_t data_out, int32_t ws_lrck, int32_t bclk, int32_t mclk, i2s_port_t port = I2S_NUM_0,
                     Data_Mode data_mode = Data_Mode::INPUT_OUTPUT, Iis_Mode iis_mode = Iis_Mode::STD,
                     i2s_slot_mode_t slot_mode_in = i2s_slot_mode_t::I2S_SLOT_MODE_STEREO, i2s_slot_mode_t slot_mode_out = i2s_slot_mode_t::I2S_SLOT_MODE_STEREO)
            : _data_in(data_in), _data_out(data_out), _ws_lrck(ws_lrck), _bclk(bclk), _mclk(mclk), _port(port),
              _data_mode(data_mode), _iis_mode(iis_mode), _slot_mode_in(slot_mode_in), _slot_mode_out(slot_mode_out)
        {
        }

        bool begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width) override;

        size_t read(void *data, size_t byte) override;
        size_t write(const void *data, size_t byte) override;

#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

        // 配置输入和输出设备
        Hardware_Iis(int32_t data_in, int32_t data_out, int32_t ws_lrck, int32_t bclk, int32_t mclk)
            : _data_in(data_in), _data_out(data_out), _ws_lrck(ws_lrck), _bclk(bclk), _mclk(mclk)
        {
        }

        bool begin(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz, nrf_i2s_swidth_t data_bit_width,
                   nrf_i2s_channels_t channel = nrf_i2s_channels_t::NRF_I2S_CHANNELS_STEREO) override;

        bool start_transmit(uint32_t *write_data, uint32_t *read_data, size_t max_data_length) override;
        void stop_transmit(void) override;
        bool set_next_read_data(uint32_t *data) override;
        bool set_next_write_data(uint32_t *data) override;
        bool get_read_event_flag(void) override;
        bool get_write_event_flag(void) override;

        void end(void) override;
#endif
    };
}