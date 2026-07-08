/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-03-11 16:03:02
 * @LastEditTime: 2026-01-20 13:55:40
 * @License: GPL 3.0
 */
#include "hardware_iis.h"

namespace Cpp_Bus_Driver
{
#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
    bool Hardware_Iis::begin(i2s_mclk_multiple_t mclk_multiple, uint32_t sample_rate_hz, i2s_data_bit_width_t data_bit_width)
    {
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _port: %d\n", _port);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _ws_lrck: %d\n", _ws_lrck);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _bclk: %d\n", _bclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _mclk: %d\n", _mclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: %d\n", mclk_multiple);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config sample_rate_hz: %d hz\n", sample_rate_hz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_bit_width: %d\n", data_bit_width);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _slot_mode_in: %d\n", _slot_mode_in);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _slot_mode_out: %d\n", _slot_mode_out);

        i2s_chan_config_t chan_config = I2S_CHANNEL_DEFAULT_CONFIG(_port, I2S_ROLE_MASTER);
        // 自动清除DMA缓冲区中的旧数据
        chan_config.auto_clear = true;

        if (_data_mode == Data_Mode::INPUT_OUTPUT)
        {
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: input_output\n");
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_in: %d\n", _data_in);
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_out: %d\n", _data_out);

            esp_err_t assert = i2s_new_channel(&chan_config, &_chan_tx_handle, &_chan_rx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                return false;
            }

            switch (_iis_mode)
            {
            case Iis_Mode::STD:
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config iis_mode: std\n");

                i2s_std_config_t config =
                    {
                        .clk_cfg =
                            {
                                .sample_rate_hz = sample_rate_hz,
                                .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
                                .ext_clk_freq_hz = 0,
#endif
                                .mclk_multiple = mclk_multiple,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
                                .bclk_div = 8,
#endif
                            },
                        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_out),
                        .gpio_cfg =
                            {
                                .mclk = static_cast<gpio_num_t>(_mclk),
                                .bclk = static_cast<gpio_num_t>(_bclk),
                                .ws = static_cast<gpio_num_t>(_ws_lrck),
                                .dout = static_cast<gpio_num_t>(_data_out),
                                .din = static_cast<gpio_num_t>(_data_in),
                                .invert_flags =
                                    {
                                        .mclk_inv = 0,
                                        .bclk_inv = 0,
                                        .ws_inv = 0,
                                    },
                            },
                    };

                assert = i2s_channel_init_std_mode(_chan_tx_handle, &config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                config.slot_cfg.slot_mode = _slot_mode_in;

                assert = i2s_channel_init_std_mode(_chan_rx_handle, &config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                break;
            }
            case Iis_Mode::PDM:
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config iis_mode: pdm\n");

                i2s_pdm_rx_config_t rx_config = {
                    .clk_cfg =
                        {
                            .sample_rate_hz = sample_rate_hz,
                            .clk_src = I2S_CLK_SRC_DEFAULT,
                            .mclk_multiple = mclk_multiple,
                            .dn_sample_mode = I2S_PDM_DSR_8S,
                            .bclk_div = 8,
                        },
                    .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_in),
                    .gpio_cfg =
                        {
                            .clk = static_cast<gpio_num_t>(_ws_lrck),
                            .din = static_cast<gpio_num_t>(_data_in),
                            .invert_flags =
                                {
                                    .clk_inv = false,
                                },
                        },
                };

                i2s_pdm_tx_config_t tx_config = {
                    .clk_cfg =
                        {
                            .sample_rate_hz = sample_rate_hz,
                            .clk_src = I2S_CLK_SRC_DEFAULT,
                            .mclk_multiple = mclk_multiple,
                            .up_sample_fp = 960,
                            .up_sample_fs = 480,
                            .bclk_div = 8,
                        },
                    .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_out),
                    .gpio_cfg =
                        {
                            .clk = static_cast<gpio_num_t>(_ws_lrck),
                            .dout = static_cast<gpio_num_t>(_data_out),
#if SOC_I2S_PDM_MAX_TX_LINES > 1
                            .dout2 = GPIO_NUM_NC,
#endif
                            .invert_flags =
                                {
                                    .clk_inv = false,
                                },
                        },
                };

                assert = i2s_channel_init_pdm_rx_mode(_chan_rx_handle, &rx_config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_pdm_rx_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                assert = i2s_channel_init_pdm_tx_mode(_chan_tx_handle, &tx_config);
                if (assert != ESP_OK)
                {
                    assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_pdm_tx_mode fail (error code: %#X)\n", assert);
                    return false;
                }

                break;
            }
            default:
                break;
            }

            assert = i2s_channel_enable(_chan_tx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                return false;
            }

            assert = i2s_channel_enable(_chan_rx_handle);
            if (assert != ESP_OK)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                return false;
            }
        }
        else
        {
            switch (_iis_mode)
            {
            case Iis_Mode::STD:
            {
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config iis_mode: std\n");
                i2s_std_config_t config =
                    {
                        .clk_cfg =
                            {
                                .sample_rate_hz = sample_rate_hz,
                                .clk_src = I2S_CLK_SRC_DEFAULT,
#if SOC_I2S_HW_VERSION_2
                                .ext_clk_freq_hz = 0,
#endif
                                .mclk_multiple = mclk_multiple,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
                                .bclk_div = 8,
#endif
                            },
                        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_out),
                        .gpio_cfg =
                            {
                                .mclk = static_cast<gpio_num_t>(_mclk),
                                .bclk = static_cast<gpio_num_t>(_bclk),
                                .ws = static_cast<gpio_num_t>(_ws_lrck),
                                .dout = I2S_GPIO_UNUSED,
                                .din = I2S_GPIO_UNUSED,
                                .invert_flags =
                                    {
                                        .mclk_inv = 0,
                                        .bclk_inv = 0,
                                        .ws_inv = 0,
                                    },
                            },
                    };

                switch (_data_mode)
                {
                case Data_Mode::INPUT:
                {
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: input\n");

                    config.gpio_cfg.din = static_cast<gpio_num_t>(_data_in);
                    config.slot_cfg.slot_mode = _slot_mode_in;

                    esp_err_t assert = i2s_new_channel(&chan_config, NULL, &_chan_rx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_init_std_mode(_chan_rx_handle, &config);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_enable(_chan_rx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                        return false;
                    }
                }
                break;
                case Data_Mode::OUTPUT:
                {
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: output\n");

                    config.gpio_cfg.dout = static_cast<gpio_num_t>(_data_out);

                    esp_err_t assert = i2s_new_channel(&chan_config, &_chan_tx_handle, NULL);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_init_std_mode(_chan_tx_handle, &config);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_std_mode fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_enable(_chan_tx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                        return false;
                    }
                }
                break;

                default:
                    break;
                }

                break;
            }
            case Iis_Mode::PDM:
                assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config iis_mode: pdm\n");

                switch (_data_mode)
                {
                case Data_Mode::INPUT:
                {
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: input\n");
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_in: %d\n", _data_in);

                    i2s_pdm_rx_config_t rx_config = {
                        .clk_cfg =
                            {
                                .sample_rate_hz = sample_rate_hz,
                                .clk_src = I2S_CLK_SRC_DEFAULT,
                                .mclk_multiple = mclk_multiple,
                                .dn_sample_mode = I2S_PDM_DSR_8S,
                                .bclk_div = 8,
                            },
                        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_in),
                        .gpio_cfg =
                            {
                                .clk = static_cast<gpio_num_t>(_ws_lrck),
                                .din = static_cast<gpio_num_t>(_data_in),
                                .invert_flags =
                                    {
                                        .clk_inv = false,
                                    },
                            },
                    };

                    esp_err_t assert = i2s_new_channel(&chan_config, NULL, &_chan_rx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_init_pdm_rx_mode(_chan_rx_handle, &rx_config);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_pdm_rx_mode fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_enable(_chan_rx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                        return false;
                    }

                    break;
                }
                case Data_Mode::OUTPUT:
                {
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_mode: output\n");
                    assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _data_out: %d\n", _data_out);

                    i2s_pdm_tx_config_t tx_config = {
                        .clk_cfg =
                            {
                                .sample_rate_hz = sample_rate_hz,
                                .clk_src = I2S_CLK_SRC_DEFAULT,
                                .mclk_multiple = mclk_multiple,
                                .up_sample_fp = 960,
                                .up_sample_fs = 480,
                                .bclk_div = 8,
                            },
                        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(data_bit_width, _slot_mode_out),
                        .gpio_cfg =
                            {
                                .clk = static_cast<gpio_num_t>(_ws_lrck),
                                .dout = static_cast<gpio_num_t>(_data_out),
#if SOC_I2S_PDM_MAX_TX_LINES > 1
                                .dout2 = GPIO_NUM_NC,
#endif
                                .invert_flags =
                                    {
                                        .clk_inv = false,
                                    },
                            },
                    };

                    esp_err_t assert = i2s_new_channel(&chan_config, &_chan_tx_handle, NULL);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_new_channel fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_init_pdm_tx_mode(_chan_tx_handle, &tx_config);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_init_pdm_tx_mode fail (error code: %#X)\n", assert);
                        return false;
                    }

                    assert = i2s_channel_enable(_chan_tx_handle);
                    if (assert != ESP_OK)
                    {
                        assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_enable fail (error code: %#X)\n", assert);
                        return false;
                    }

                    break;
                }
                default:
                    break;
                }
                break;

            default:
                break;
            }
        }

        _mclk_multiple = mclk_multiple;
        _sample_rate_hz = sample_rate_hz;
        _data_bit_width = data_bit_width;

        return true;
    }

    size_t Hardware_Iis::read(void *data, size_t byte)
    {
        if (_chan_rx_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "_chan_rx_handle is nullptr \n");
            return false;
        }

        size_t buffer = 0;
        esp_err_t assert = i2s_channel_read(_chan_rx_handle, data, byte, &buffer, DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_read fail (error code: %#X)\n", assert);
            return false;
        }

        return buffer;
    }

    size_t Hardware_Iis::write(const void *data, size_t byte)
    {
        if (_chan_tx_handle == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "_chan_tx_handle is nullptr \n");
            return false;
        }

        size_t buffer = 0;
        esp_err_t assert = i2s_channel_write(_chan_tx_handle, data, byte, &buffer, DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS);
        if (assert != ESP_OK)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "i2s_channel_write fail (error code: %#X)\n", assert);
            return false;
        }

        return buffer;
    }

#elif defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
    bool Hardware_Iis::begin(nrf_i2s_ratio_t mclk_multiple, uint32_t sample_rate_hz, nrf_i2s_swidth_t data_bit_width, nrf_i2s_channels_t channel)
    {
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _ws_lrck: %d\n", _ws_lrck);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _bclk: %d\n", _bclk);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config _mclk: %d\n", _mclk);

        nrf_i2s_mck_t buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_DISABLED;
        double buffer_mclk_freq_mhz = 0.0;

        switch (mclk_multiple)
        {
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_32X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 32.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 32\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_48X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 48.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 48\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_64X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 64.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 64\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_96X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 96.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 96\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_128X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 128.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 128\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_192X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 192.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 192\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_256X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 256.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 256\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_384X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 384.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 384\n");
            break;
        case nrf_i2s_ratio_t::NRF_I2S_RATIO_512X:
            buffer_mclk_freq_mhz = (static_cast<double>(sample_rate_hz) * 512.0) / 1000000.0;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_multiple: 512\n");
            break;

        default:
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "hardware_iis mclk_multiple check fail (unknown mclk_multiple)\n");
            return false;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config sample_rate_hz: %d hz\n", sample_rate_hz);
        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_freq_mhz: %.6f mhz\n", buffer_mclk_freq_mhz);

        // 定义每个分频对应的频率值
        constexpr const double buffer_division_freq_125 = 32.0 / 125.0;
        constexpr const double buffer_division_freq_63 = 32.0 / 63.0;
        constexpr const double buffer_division_freq_42 = 32.0 / 42.0;
        constexpr const double buffer_division_freq_32 = 32.0 / 32.0;
        constexpr const double buffer_division_freq_31 = 32.0 / 31.0;
        constexpr const double buffer_division_freq_30 = 32.0 / 30.0;
        constexpr const double buffer_division_freq_23 = 32.0 / 23.0;
        constexpr const double buffer_division_freq_21 = 32.0 / 21.0;
        constexpr const double buffer_division_freq_16 = 32.0 / 16.0;
        constexpr const double buffer_division_freq_15 = 32.0 / 15.0;
        constexpr const double buffer_division_freq_11 = 32.0 / 11.0;
        constexpr const double buffer_division_freq_10 = 32.0 / 10.0;
        constexpr const double buffer_division_freq_8 = 32.0 / 8.0 + 1; // 计算出来的值有可能大于32.0 / 8.0

        // 计算每个范围的中点
        constexpr const double buffer_division_freq_mid_125_63 = (buffer_division_freq_125 + buffer_division_freq_63) / 2.0;
        constexpr const double buffer_division_freq_mid_63_42 = (buffer_division_freq_63 + buffer_division_freq_42) / 2.0;
        constexpr const double buffer_division_freq_mid_42_32 = (buffer_division_freq_42 + buffer_division_freq_32) / 2.0;
        constexpr const double buffer_division_freq_mid_32_31 = (buffer_division_freq_32 + buffer_division_freq_31) / 2.0;
        constexpr const double buffer_division_freq_mid_31_30 = (buffer_division_freq_31 + buffer_division_freq_30) / 2.0;
        constexpr const double buffer_division_freq_mid_30_23 = (buffer_division_freq_30 + buffer_division_freq_23) / 2.0;
        constexpr const double buffer_division_freq_mid_23_21 = (buffer_division_freq_23 + buffer_division_freq_21) / 2.0;
        constexpr const double buffer_division_freq_mid_21_16 = (buffer_division_freq_21 + buffer_division_freq_16) / 2.0;
        constexpr const double buffer_division_freq_mid_16_15 = (buffer_division_freq_16 + buffer_division_freq_15) / 2.0;
        constexpr const double buffer_division_freq_mid_15_11 = (buffer_division_freq_15 + buffer_division_freq_11) / 2.0;
        constexpr const double buffer_division_freq_mid_11_10 = (buffer_division_freq_11 + buffer_division_freq_10) / 2.0;
        constexpr const double buffer_division_freq_mid_10_8 = (buffer_division_freq_10 + buffer_division_freq_8) / 2.0;

        if ((buffer_mclk_freq_mhz >= buffer_division_freq_125) && (buffer_mclk_freq_mhz < buffer_division_freq_mid_125_63))
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV125;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 125 (division_freq: %.6f mhz)\n", buffer_division_freq_125);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_63_42)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV63;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 63 (division_freq: %.6f mhz)\n", buffer_division_freq_63);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_42_32)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV42;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 42 (division_freq: %.6f mhz)\n", buffer_division_freq_42);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_32_31)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV32;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 32 (division_freq: %.6f mhz)\n", buffer_division_freq_32);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_31_30)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV31;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 31 (division_freq: %.6f mhz)\n", buffer_division_freq_31);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_30_23)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV30;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 30 (division_freq: %.6f mhz)\n", buffer_division_freq_30);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_23_21)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV23;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 23 (division_freq: %.6f mhz)\n", buffer_division_freq_23);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_21_16)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV21;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 21 (division_freq: %.6f mhz)\n", buffer_division_freq_21);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_16_15)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV16;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 16 (division_freq: %.6f mhz)\n", buffer_division_freq_16);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_15_11)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV15;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 15 (division_freq: %.6f mhz)\n", buffer_division_freq_15);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_11_10)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV11;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 11 (division_freq: %.6f mhz)\n", buffer_division_freq_11);
        }
        else if (buffer_mclk_freq_mhz < buffer_division_freq_mid_10_8)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV10;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 10 (division_freq: %.6f mhz)\n", buffer_division_freq_10);
        }
        else if (buffer_mclk_freq_mhz <= buffer_division_freq_8)
        {
            buffer_mclk_division = nrf_i2s_mck_t::NRF_I2S_MCK_32MDIV8;
            assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config mclk_division: 8 (division_freq: %.6f mhz)\n", buffer_division_freq_8);
        }
        else
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "hardware_iis mclk_division check fail (mclk_division out of bounds)\n");
            return false;
        }

        assert_log(Log_Level::INFO, __FILE__, __LINE__, "hardware_iis config data_bit_width: %d\n", (data_bit_width + 1) * 8);

        nrf_gpio_cfg_output(_bclk);
        nrf_gpio_cfg_output(_ws_lrck);
        nrf_gpio_cfg_output(_mclk);
        nrf_gpio_cfg_output(_data_out);
        nrf_gpio_cfg_input(_data_in, NRF_GPIO_PIN_NOPULL);
        nrf_i2s_pins_set(NRF_I2S, _bclk, _ws_lrck, _mclk, _data_out, _data_in);

        if (nrf_i2s_configure(NRF_I2S, nrf_i2s_mode_t::NRF_I2S_MODE_MASTER, nrf_i2s_format_t::NRF_I2S_FORMAT_I2S, nrf_i2s_align_t::NRF_I2S_ALIGN_LEFT,
                              data_bit_width, channel, buffer_mclk_division, mclk_multiple) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrf_i2s_configure fail\n");
            return false;
        }

        return true;
    }

    bool Hardware_Iis::start_transmit(uint32_t *write_data, uint32_t *read_data, size_t max_data_length)
    {
        if (write_data == nullptr && read_data == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail (write_data == nullptr && read_data == nullptr)\n");
            return false;
        }
        if (max_data_length == 0)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "start_transmit fail (max_data_length == 0)\n");
            return false;
        }

        if (write_data != nullptr)
        {
            if (nrfx_is_in_ram(write_data) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_in_ram fail (write_data is not located in the data ram region)\n");
                return false;
            }
            if (nrfx_is_word_aligned(write_data) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_word_aligned fail (write_data is not aligned to a 32-bit word)\n");
                return false;
            }

            nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
        }
        if (read_data != nullptr)
        {
            if (nrfx_is_in_ram(read_data) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_in_ram fail (write_data is not located in the data ram region)\n");
                return false;
            }
            if (nrfx_is_word_aligned(read_data) == false)
            {
                assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_word_aligned fail (write_data is not aligned to a 32-bit word)\n");
                return false;
            }

            nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
        }

        nrf_i2s_transfer_set(NRF_I2S, max_data_length, read_data, write_data);

        // 启动iis音频流传输任务
        nrf_i2s_enable(NRF_I2S);

        nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_START);

        return true;
    }

    void Hardware_Iis::stop_transmit(void)
    {
        nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);

        nrf_i2s_disable(NRF_I2S);
    }

    bool Hardware_Iis::set_next_read_data(uint32_t *data)
    {
        if (data == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "set_next_read_data fail (data == nullptr)\n");
            return false;
        }

        if (nrfx_is_in_ram(data) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_in_ram fail (data is not located in the data ram region)\n");
            return false;
        }
        if (nrfx_is_word_aligned(data) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_word_aligned fail (data is not aligned to a 32-bit word)\n");
            return false;
        }

        nrf_i2s_rx_buffer_set(NRF_I2S, data);

        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);

        return true;
    }

    bool Hardware_Iis::set_next_write_data(uint32_t *data)
    {
        if (data == nullptr)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "set_next_write_data fail (data == nullptr)\n");
            return false;
        }

        if (nrfx_is_in_ram(data) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_in_ram fail (data is not located in the data ram region)\n");
            return false;
        }
        if (nrfx_is_word_aligned(data) == false)
        {
            assert_log(Log_Level::BUS, __FILE__, __LINE__, "nrfx_is_word_aligned fail (data is not aligned to a 32-bit word)\n");
            return false;
        }

        nrf_i2s_tx_buffer_set(NRF_I2S, data);

        nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);

        return true;
    }

    bool Hardware_Iis::get_read_event_flag(void)
    {
        return nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
    }

    bool Hardware_Iis::get_write_event_flag(void)
    {
        return nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
    }

    void Hardware_Iis::end(void)
    {
        stop_transmit();

        nrf_i2s_pins_set(NRF_I2S, NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED,
                         NRF_I2S_PIN_NOT_CONNECTED, NRF_I2S_PIN_NOT_CONNECTED);

        nrf_gpio_cfg_default(_bclk);
        nrf_gpio_cfg_default(_ws_lrck);
        nrf_gpio_cfg_default(_mclk);
        nrf_gpio_cfg_default(_data_out);
        nrf_gpio_cfg_default(_data_in);
    }

#endif

}
