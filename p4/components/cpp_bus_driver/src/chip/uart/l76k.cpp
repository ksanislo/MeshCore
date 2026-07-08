/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-01-14 14:12:32
 * @LastEditTime: 2025-11-22 15:19:28
 * @License: GPL 3.0
 */
#include "l76k.h"

namespace Cpp_Bus_Driver
{
    bool L76k::begin(int32_t baud_rate)
    {
        if (_rst != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            Uart_Guide::pin_mode(_rst, Pin_Mode::OUTPUT, Pin_Status::PULLUP);

            Uart_Guide::pin_write(_rst, 1);
            Uart_Guide::delay_ms(10);
            Uart_Guide::pin_write(_rst, 0);
            Uart_Guide::delay_ms(10);
            Uart_Guide::pin_write(_rst, 1);
            Uart_Guide::delay_ms(10);
        }

        if (_wake_up != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            if (Uart_Guide::pin_mode(_wake_up, Pin_Mode::OUTPUT, Pin_Status::PULLUP) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "pin_mode fail\n");
            }
        }

        if (sleep(false) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "sleep fail\n");
        }

        if (Uart_Guide::begin(baud_rate) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "begin fail\n");
            return false;
        }

        size_t buffer_index = 0;
        if (get_device_id(&buffer_index) == false)
        {
            Uart_Guide::assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id fail\n");
            return false;
        }
        else
        {
            Uart_Guide::assert_log(Log_Level::INFO, __FILE__, __LINE__, "get l76k id success (index: %d)\n", buffer_index);
        }

        return true;
    }

    bool L76k::sleep(bool enable)
    {
        if (_wake_up != DEFAULT_CPP_BUS_DRIVER_VALUE)
        {
            if (Uart_Guide::pin_write(_wake_up, !enable) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "pin_write fail\n");
                return false;
            }
        }
        else if (_wake_up_callback != nullptr)
        {
            if (_wake_up_callback(!enable) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "_wake_up_callback fail\n");
                return false;
            }
        }

        return true;
    }

    bool L76k::get_device_id(size_t *search_index)
    {
        std::unique_ptr<uint8_t[]> buffer;
        uint32_t buffer_lenght = 0;

        if (get_info_data(buffer, &buffer_lenght) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_info_data fail\n");
            return false;
        }

        const char *buffer_cmd = "\r\n$G";
        if (Uart_Guide::search(buffer.get(), buffer_lenght, buffer_cmd, std::strlen(buffer_cmd), search_index) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "search fail\n");
            return false;
        }

        return true;
    }

    uint32_t L76k::read_data(uint8_t *data, uint32_t length)
    {
        uint32_t length_buffer = _bus->get_rx_buffer_length();
        if (length_buffer == 0)
        {
            // Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "get_rx_buffer_length is empty\n");
            return false;
        }

        if ((length == 0) || (length >= length_buffer))
        {
            if (_bus->read(data, length_buffer) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }
        else if (length < length_buffer)
        {
            if (_bus->read(data, length) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                return false;
            }
        }

        return length_buffer;
    }

    size_t L76k::get_rx_buffer_length(void)
    {
        return _bus->get_rx_buffer_length();
    }

    bool L76k::clear_rx_buffer_data(void)
    {
        return _bus->clear_rx_buffer_data();
    }

    bool L76k::get_info_data(std::unique_ptr<uint8_t[]> &data, uint32_t *length, uint32_t max_length, uint8_t timeout_count)
    {
        uint8_t buffer_timeout_count = 0;

        while (1)
        {
            Uart_Guide::delay_ms(_update_freq);

            uint32_t buffer_lenght = get_rx_buffer_length();
            if (buffer_lenght > max_length)
            {
                buffer_lenght = max_length;
            }

            if (buffer_lenght > 0)
            {
                data = std::make_unique<uint8_t[]>(buffer_lenght);
                if (data == nullptr)
                {
                    Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "data std::make_unique fail\n");
                    data = nullptr;
                    *length = 0;
                    return false;
                }

                buffer_lenght = _bus->read(data.get(), buffer_lenght);
                if (buffer_lenght == false)
                {
                    Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read fail\n");
                    data = nullptr;
                    *length = 0;
                    return false;
                }

                Uart_Guide::assert_log(Log_Level::DEBUG, __FILE__, __LINE__, "get_info_data lenght: %d\n", buffer_lenght);
                *length = buffer_lenght;
                break;
            }

            buffer_timeout_count++;
            if (buffer_timeout_count > timeout_count) // 超时
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "read timeout\n");
                data = nullptr;
                *length = 0;
                return false;
            }
        }

        return true;
    }

    bool L76k::set_update_frequency(Update_Freq freq)
    {
        const char *buffer = nullptr;

        switch (freq)
        {
        case Update_Freq::FREQ_1HZ:
            buffer = "$PCAS02,1000*2E\r\n";
            _update_freq = 1000;
            break;
        case Update_Freq::FREQ_2HZ:
            buffer = "$PCAS02,500*1A\r\n";
            _update_freq = 500;
            break;
        case Update_Freq::FREQ_5HZ:
            buffer = "$PCAS02,200*1D\r\n";
            _update_freq = 200;
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool L76k::set_baud_rate(Baud_Rate baud_rate)
    {
        const char *buffer = nullptr;

        switch (baud_rate)
        {
        case Baud_Rate::BR_4800_BPS:
            buffer = "$PCAS01,0*1C\r\n";
            break;
        case Baud_Rate::BR_9600_BPS:
            buffer = "$PCAS01,1*1D\r\n";
            break;
        case Baud_Rate::BR_19200_BPS:
            buffer = "$PCAS01,2*1E\r\n";
            break;
        case Baud_Rate::BR_38400_BPS:
            buffer = "$PCAS01,3*1F\r\n";
            break;
        case Baud_Rate::BR_57600_BPS:
            buffer = "$PCAS01,4*18\r\n";
            break;
        case Baud_Rate::BR_115200_BPS:
            buffer = "$PCAS01,5*19\r\n";
            break;

        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }
        // 只有设置波特率时候需要延时
        //  因为没有忙总线所以这里写入数据需要在模块未发送数据空闲的时候写，所以要延时，延时时间为更新频率的一半
        Uart_Guide::delay_ms(_update_freq / 2);
        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        switch (baud_rate)
        {
        case Baud_Rate::BR_4800_BPS:
            if (_bus->set_baud_rate(4800) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_9600_BPS:
            if (_bus->set_baud_rate(9600) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_19200_BPS:
            if (_bus->set_baud_rate(19200) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_38400_BPS:
            if (_bus->set_baud_rate(38400) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_57600_BPS:
            if (_bus->set_baud_rate(57600) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;
        case Baud_Rate::BR_115200_BPS:
            if (_bus->set_baud_rate(115200) == false)
            {
                Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "set_baud_rate fail\n");
                return false;
            }
            break;

        default:
            break;
        }

        return true;
    }

    uint32_t L76k::get_baud_rate(void)
    {
        return _bus->get_baud_rate();
    }

    bool L76k::set_restart_mode(Restart_Mode mode)
    {
        const char *buffer = nullptr;

        switch (mode)
        {
        case Restart_Mode::HOT_START:
            buffer = "$PCAS10,0*1C\r\n";
            break;
        case Restart_Mode::WARM_START:
            buffer = "$PCAS10,1*1D\r\n";
            break;
        case Restart_Mode::COLD_START:
            buffer = "$PCAS10,2*1E\r\n";
            break;
        case Restart_Mode::COLD_START_FACTORY_RESET:
            buffer = "$PCAS10,3*1F\r\n";
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

    bool L76k::set_gnss_constellation(Gnss_Constellation constellation)
    {
        const char *buffer = nullptr;

        switch (constellation)
        {
        case Gnss_Constellation::GPS:
            buffer = "$PCAS04,1*18\r\n";
            break;
        case Gnss_Constellation::BEIDOU:
            buffer = "$PCAS04,2*1B\r\n";
            break;
        case Gnss_Constellation::GPS_BEIDOU:
            buffer = "$PCAS04,3*1A\r\n";
            break;
        case Gnss_Constellation::GLONASS:
            buffer = "$PCAS04,4*1D\r\n";
            break;
        case Gnss_Constellation::GPS_GLONASS:
            buffer = "$PCAS04,5*1C\r\n";
            break;
        case Gnss_Constellation::BEIDOU_GLONASS:
            buffer = "$PCAS04,6*1F\r\n";
            break;
        case Gnss_Constellation::GPS_BEIDOU_GLONASS:
            buffer = "$PCAS04,7*1E\r\n";
            break;
        default:
            break;
        }

        if (_bus->write(buffer, strlen(buffer)) == false)
        {
            Uart_Guide::assert_log(Log_Level::CHIP, __FILE__, __LINE__, "write fail\n");
            return false;
        }

        return true;
    }

}
