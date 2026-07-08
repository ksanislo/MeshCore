/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2025-08-05 11:23:01
 * @LastEditTime: 2025-09-08 16:56:52
 * @License: GPL 3.0
 */
#include "SPI.h"

void SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{
    _bus->_port = static_cast<spi_host_device_t>(_spi_num);
    _bus->_sclk = sck;
    _bus->_miso = miso;
    _bus->_mosi = mosi;
    _bus->_cs = ss;

    init_flag = false;
}

// void SPIClass::end()
// {
//     // if (!_spi)
//     // {
//     //     return;
//     // }
//     // spiDetachSCK(_spi, _sck);
//     // spiDetachMISO(_spi, _miso);
//     // spiDetachMOSI(_spi, _mosi);
//     // setHwCs(false);
//     // spiStopBus(_spi);
//     // _spi = NULL;
// }

// void SPIClass::setHwCs(bool use)
// {
//     // if (use && !_use_hw_ss)
//     // {
//     //     spiAttachSS(_spi, 0, _ss);
//     //     spiSSEnable(_spi);
//     // }
//     // else if (!use && _use_hw_ss)
//     // {
//     //     spiSSDisable(_spi);
//     //     spiDetachSS(_spi, _ss);
//     // }
//     // _use_hw_ss = use;
// }

void SPIClass::setFrequency(uint32_t freq)
{
    _bus->_freq_hz = freq;
}

// void SPIClass::setClockDivider(uint32_t clockDiv)
// {
//     // SPI_PARAM_LOCK();
//     // _div = clockDiv;
//     // spiSetClockDiv(_spi, _div);
//     // SPI_PARAM_UNLOCK();
// }

// uint32_t SPIClass::getClockDivider()
// {
//     // return spiGetClockDiv(_spi);
//     return 0;
// }

// void SPIClass::setDataMode(uint8_t dataMode)
// {
//     // spiSetDataMode(_spi, dataMode);
// }

// void SPIClass::setBitOrder(uint8_t bitOrder)
// {
//     // spiSetBitOrder(_spi, bitOrder);
// }

void SPIClass::beginTransaction(SPISettings settings)
{
    // SPI_PARAM_LOCK();
    // // check if last freq changed
    // uint32_t cdiv = spiGetClockDiv(_spi);
    // if (_freq != settings._clock || _div != cdiv)
    // {
    //     _freq = settings._clock;
    //     _div = spiFrequencyToClockDiv(_freq);
    // }
    // spiTransaction(_spi, _div, settings._dataMode, settings._bitOrder);
    // _inTransaction = true;

    if (init_flag == true)
    {
        return;
    }

    _bus->_mode = settings._dataMode;

    switch (settings._bitOrder)
    {
    case SPI_LSBFIRST:
        _bus->_flags = SPI_DEVICE_BIT_LSBFIRST;
        break;
    case SPI_MSBFIRST:
        break;

    default:
        break;
    }

    if (_bus->begin(settings._clock, _bus->_cs) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
    }

    init_flag = true;
}

void SPIClass::endTransaction()
{
    size_t buffer_tx_buffer_length = _tx_buffer.size();
    size_t buffer_rx_buffer_length = buffer_tx_buffer_length - _rx_length;

    if (buffer_tx_buffer_length == 0)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::INFO, __FILE__, __LINE__, "endTransaction fail (_tx_buffer length == 0)\n");
        return;
    }

    if (buffer_rx_buffer_length == 0)
    {
        if (_bus->write(_tx_buffer.data(), buffer_tx_buffer_length) == false)
        {
            _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
        }

        // for (size_t i = 0; i < buffer_tx_buffer_length; i++)
        // {
        //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "tx1[%d]: %#X\n", i, _tx_buffer[i]);
        // }
    }
    else
    {
        auto buffer_rx_data = std::make_unique<uint8_t[]>(buffer_tx_buffer_length);

        if (_bus->write_read(_tx_buffer.data(), buffer_rx_data.get(), buffer_tx_buffer_length) == false)
        {
            _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "write_read fail\n");
        }

        // for (size_t i = 0; i < buffer_tx_buffer_length; i++)
        // {
        //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "tx2[%d]: %#X\n", i, _tx_buffer[i]);
        // }

        // for (size_t i = 0; i < buffer_tx_buffer_length; i++)
        // {
        //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "rx[%d]: %#X\n", i, buffer_rx_data[i]);
        // }

        std::memcpy(_rx_buffer, buffer_rx_data.get() + buffer_rx_buffer_length, _rx_length);

        // for (size_t i = 0; i < _rx_length; i++)
        // {
        //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "_rx_buffer[%d]: %#X\n", i, _rx_buffer[i]);
        // }
    }

    _tx_buffer.clear();
    _rx_buffer = nullptr;
    _rx_length = 0;
}

// void SPIClass::write(uint8_t data)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiWriteByteNL(_spi, data);
//     // }
//     // spiWriteByte(_spi, data);
// }

uint8_t SPIClass::transfer(uint8_t data)
{
    transfer(&data, 1);
    return 1;
}

// void SPIClass::write16(uint16_t data)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiWriteShortNL(_spi, data);
//     // }
//     // spiWriteWord(_spi, data);
// }

// uint16_t SPIClass::transfer16(uint16_t data)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiTransferShortNL(_spi, data);
//     // }
//     // return spiTransferWord(_spi, data);
//     return 0;
// }

// void SPIClass::write32(uint32_t data)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiWriteLongNL(_spi, data);
//     // }
//     // spiWriteLong(_spi, data);
// }

// uint32_t SPIClass::transfer32(uint32_t data)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiTransferLongNL(_spi, data);
//     // }
//     // return spiTransferLong(_spi, data);
//     return 0;
// }

// void SPIClass::transferBits(uint32_t data, uint32_t *out, uint8_t bits)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiTransferBitsNL(_spi, data, out, bits);
//     // }
//     // spiTransferBits(_spi, data, out, bits);
// }

// /**
//  * @param data uint8_t *
//  * @param size uint32_t
//  */
// void SPIClass::writeBytes(const uint8_t *data, uint32_t size)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiWriteNL(_spi, data, size);
//     // }
//     // spiSimpleTransaction(_spi);
//     // spiWriteNL(_spi, data, size);
//     // spiEndTransaction(_spi);
// }

void SPIClass::transfer(void *data, uint32_t size)
{
    transferBytes((const uint8_t *)data, (uint8_t *)data, size);
}

// /**
//  * @param data void *
//  * @param size uint32_t
//  */
// void SPIClass::writePixels(const void *data, uint32_t size)
// {
//     // if (_inTransaction)
//     // {
//     //     return spiWritePixelsNL(_spi, data, size);
//     // }
//     // spiSimpleTransaction(_spi);
//     // spiWritePixelsNL(_spi, data, size);
//     // spiEndTransaction(_spi);
// }

/**
 * @param data uint8_t * data buffer. can be NULL for Read Only operation
 * @param out  uint8_t * output buffer. can be NULL for Write Only operation
 * @param size uint32_t
 */
void SPIClass::transferBytes(const uint8_t *data, uint8_t *out, uint32_t size)
{
    _tx_buffer.insert(_tx_buffer.end(), data, data + size);

    _rx_buffer = out;
    _rx_length = size;
}

// /**
//  * @param data uint8_t *
//  * @param size uint8_t  max for size is 64Byte
//  * @param repeat uint32_t
//  */
// void SPIClass::writePattern(const uint8_t *data, uint8_t size, uint32_t repeat)
// {
//     // if (size > 64)
//     // {
//     //     return; // max Hardware FIFO
//     // }

//     // uint32_t byte = (size * repeat);
//     // uint8_t r = (64 / size);
//     // const uint8_t max_bytes_FIFO = r * size; // Max number of whole patterns (in bytes) that can fit into the hardware FIFO

//     // while (byte)
//     // {
//     //     if (byte > max_bytes_FIFO)
//     //     {
//     //         writePattern_(data, size, r);
//     //         byte -= max_bytes_FIFO;
//     //     }
//     //     else
//     //     {
//     //         writePattern_(data, size, (byte / size));
//     //         byte = 0;
//     //     }
//     // }
// }

// void SPIClass::writePattern_(const uint8_t *data, uint8_t size, uint8_t repeat)
// {
//     // uint8_t bytes = (size * repeat);
//     // uint8_t buffer[64];
//     // uint8_t *bufferPtr = &buffer[0];
//     // const uint8_t *dataPtr;
//     // uint8_t dataSize = bytes;
//     // for (uint8_t i = 0; i < repeat; i++)
//     // {
//     //     dataSize = size;
//     //     dataPtr = data;
//     //     while (dataSize--)
//     //     {
//     //         *bufferPtr = *dataPtr;
//     //         dataPtr++;
//     //         bufferPtr++;
//     //     }
//     // }

//     // writeBytes(&buffer[0], bytes);
// }

SPIClass SPI(SPI2_HOST);
