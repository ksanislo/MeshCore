/*
  TwoWire.cpp - TWI/I2C library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified December 2014 by Ivan Grokhotkov (ivan@esp8266.com) - esp8266 support
  Modified April 2015 by Hrsto Gochkov (ficeto@ficeto.com) - alternative esp8266 support
  Modified Nov 2017 by Chuck Todd (ctodd@cableone.net) - ESP32 ISR Support
  Modified Nov 2021 by Hristo Gochkov <Me-No-Dev> to support ESP-IDF API
 */

#include "Wire.h"

// bool TwoWire::initPins(int sdaPin, int sclPin)
// {
//     //     if (sdaPin < 0)
//     //     { // default param passed
//     //         if (num == 0)
//     //         {
//     //             if (sda == -1)
//     //             {
//     //                 sdaPin = SDA; // use Default Pin
//     //             }
//     //             else
//     //             {
//     //                 sdaPin = sda; // reuse prior pin
//     //             }
//     //         }
//     //         else
//     //         {
//     //             if (sda == -1)
//     //             {
//     // #ifdef WIRE1_PIN_DEFINED
//     //                 sdaPin = SDA1;
//     // #else
//     //                 log_e("no Default SDA Pin for Second Peripheral");
//     //                 return false; // no Default pin for Second Peripheral
//     // #endif
//     //             }
//     //             else
//     //             {
//     //                 sdaPin = sda; // reuse prior pin
//     //             }
//     //         }
//     //     }

//     //     if (sclPin < 0)
//     //     { // default param passed
//     //         if (num == 0)
//     //         {
//     //             if (scl == -1)
//     //             {
//     //                 sclPin = SCL; // use Default pin
//     //             }
//     //             else
//     //             {
//     //                 sclPin = scl; // reuse prior pin
//     //             }
//     //         }
//     //         else
//     //         {
//     //             if (scl == -1)
//     //             {
//     // #ifdef WIRE1_PIN_DEFINED
//     //                 sclPin = SCL1;
//     // #else
//     //                 log_e("no Default SCL Pin for Second Peripheral");
//     //                 return false; // no Default pin for Second Peripheral
//     // #endif
//     //             }
//     //             else
//     //             {
//     //                 sclPin = scl; // reuse prior pin
//     //             }
//     //         }
//     //     }

//     //     sda = sdaPin;
//     //     scl = sclPin;
//     return true;
// }

bool TwoWire::setPins(int sdaPin, int sclPin)
{
    _bus->_sda = sdaPin;
    _bus->_scl = sclPin;

    return true;
}

// bool TwoWire::allocateWireBuffer(void)
// {
//     // // or both buffer can be allocated or none will be
//     // if (_rx_buffer == NULL)
//     // {
//     //     _rx_buffer = (uint8_t *)malloc(bufferSize);
//     //     if (_rx_buffer == NULL)
//     //     {
//     //         log_e("Can't allocate memory for I2C_%d _rx_buffer", num);
//     //         return false;
//     //     }
//     // }
//     // if (_tx_buffer == NULL)
//     // {
//     //     _tx_buffer = (uint8_t *)malloc(bufferSize);
//     //     if (_tx_buffer == NULL)
//     //     {
//     //         log_e("Can't allocate memory for I2C_%d _tx_buffer", num);
//     //         freeWireBuffer(); // free _rx_buffer for safety!
//     //         return false;
//     //     }
//     // }
//     // // in case both were allocated before, they must have the same size. All good.
//     return true;
// }

void TwoWire::freeWireBuffer(void)
{
    if (_rx_buffer.get() != NULL)
    {
        _rx_buffer.reset();
    }

    _tx_buffer.clear();
}

// size_t TwoWire::setBufferSize(size_t bSize)
// {
//     //     // Maximum size .... HEAP limited ;-)
//     //     if (bSize < 32)
//     //     { // 32 bytes is the I2C FIFO Len for ESP32/S2/S3/C3
//     //         log_e("Minimum Wire Buffer size is 32 bytes");
//     //         return 0;
//     //     }

//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     if (lock == NULL)
//     //     {
//     //         lock = xSemaphoreCreateMutex();
//     //         if (lock == NULL)
//     //         {
//     //             log_e("xSemaphoreCreateMutex failed");
//     //             return 0;
//     //         }
//     //     }
//     //     // acquire lock
//     //     if (xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
//     //     {
//     //         log_e("could not acquire lock");
//     //         return 0;
//     //     }
//     // #endif
//     //     // allocateWireBuffer allocates memory for both pointers or just free them
//     //     if (_rx_buffer != NULL || _tx_buffer != NULL)
//     //     {
//     //         // if begin() has been already executed, memory size changes... data may be lost. We don't care! :^)
//     //         if (bSize != bufferSize)
//     //         {
//     //             // we want a new buffer size ... just reset buffer pointers and allocate new ones
//     //             freeWireBuffer();
//     //             bufferSize = bSize;
//     //             if (!allocateWireBuffer())
//     //             {
//     //                 // failed! Error message already issued
//     //                 bSize = 0; // returns error
//     //                 log_e("Buffer allocation failed");
//     //             }
//     //         } // else nothing changes, all set!
//     //     }
//     //     else
//     //     {
//     //         // no memory allocated yet, just change the size value - allocation in begin()
//     //         bufferSize = bSize;
//     //     }
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     // release lock
//     //     xSemaphoreGive(lock);

//     // #endif
//     return bSize;
// }

// Slave Begin
// bool TwoWire::begin(uint8_t addr, int sdaPin, int sclPin, uint32_t frequency)
// {
//     //     bool started = false;
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     if (lock == NULL)
//     //     {
//     //         lock = xSemaphoreCreateMutex();
//     //         if (lock == NULL)
//     //         {
//     //             log_e("xSemaphoreCreateMutex failed");
//     //             return false;
//     //         }
//     //     }
//     //     // acquire lock
//     //     if (xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
//     //     {
//     //         log_e("could not acquire lock");
//     //         return false;
//     //     }
//     // #endif
//     //     if (is_slave)
//     //     {
//     //         log_w("Bus already started in Slave Mode.");
//     //         started = true;
//     //         goto end;
//     //     }
//     //     if (i2cIsInit(num))
//     //     {
//     //         log_e("Bus already started in Master Mode.");
//     //         goto end;
//     //     }
//     //     if (!allocateWireBuffer())
//     //     {
//     //         // failed! Error Message already issued
//     //         goto end;
//     //     }
//     //     if (!initPins(sdaPin, sclPin))
//     //     {
//     //         goto end;
//     //     }
//     //     i2cSlaveAttachCallbacks(num, onRequestService, onReceiveService, this);
//     //     if (i2cSlaveInit(num, sda, scl, addr, frequency, bufferSize, bufferSize) != ESP_OK)
//     //     {
//     //         log_e("Slave Init ERROR");
//     //         goto end;
//     //     }
//     //     is_slave = true;
//     //     started = true;
//     // end:
//     //     if (!started)
//     //         freeWireBuffer();
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     // release lock
//     //     xSemaphoreGive(lock);
//     // #endif
//     // return started;
//     return 0;
// }

// Master Begin
bool TwoWire::begin(int sdaPin, int sclPin, uint32_t frequency)
{
    _bus->_sda = sdaPin;
    _bus->_scl = sclPin;
    _bus->_freq_hz = frequency;

    init_flag = false;

    return true;
}

// bool TwoWire::end()
// {
//     //     esp_err_t err = ESP_OK;
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     if (lock != NULL)
//     //     {
//     //         // acquire lock
//     //         if (xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
//     //         {
//     //             log_e("could not acquire lock");
//     //             return false;
//     //         }
//     // #endif
//     //         if (is_slave)
//     //         {
//     //             err = i2cSlaveDeinit(num);
//     //             if (err == ESP_OK)
//     //             {
//     //                 is_slave = false;
//     //             }
//     //         }
//     //         else if (i2cIsInit(num))
//     //         {
//     //             err = i2cDeinit(num);
//     //         }
//     //         freeWireBuffer();
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //         // release lock
//     //         xSemaphoreGive(lock);
//     //     }
//     // #endif
//     //     return (err == ESP_OK);
//     return 0;
// }

// uint32_t TwoWire::getClock()
// {
//     //     uint32_t frequency = 0;
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     // acquire lock
//     //     if (lock == NULL || xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
//     //     {
//     //         log_e("could not acquire lock");
//     //     }
//     //     else
//     //     {
//     // #endif
//     //         if (is_slave)
//     //         {
//     //             log_e("Bus is in Slave Mode");
//     //         }
//     //         else
//     //         {
//     //             i2cGetClock(num, &frequency);
//     //         }
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //         // release lock
//     //         xSemaphoreGive(lock);
//     //     }
//     // #endif
//     //     return frequency;
//     return 0;
// }

// bool TwoWire::setClock(uint32_t frequency)
// {
//     //     esp_err_t err = ESP_OK;
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     // acquire lock
//     //     if (lock == NULL || xSemaphoreTake(lock, portMAX_DELAY) != pdTRUE)
//     //     {
//     //         log_e("could not acquire lock");
//     //         return false;
//     //     }
//     // #endif
//     //     if (is_slave)
//     //     {
//     //         log_e("Bus is in Slave Mode");
//     //         err = ESP_FAIL;
//     //     }
//     //     else
//     //     {
//     //         err = i2cSetClock(num, frequency);
//     //     }
//     // #if !CONFIG_DISABLE_HAL_LOCKS
//     //     // release lock
//     //     xSemaphoreGive(lock);
//     // #endif
//     //     return (err == ESP_OK);
//     return 0;
// }

// void TwoWire::setTimeOut(uint16_t timeOutMillis)
// {
//     _timeOutMillis = timeOutMillis;
// }

// uint16_t TwoWire::getTimeOut()
// {
//     return _timeOutMillis;
// }

void TwoWire::beginTransmission(uint16_t address)
{
    if (init_flag == true)
    {
        return;
    }

    if (_bus->begin(_bus->_freq_hz, address) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "begin fail\n");
        return;
    }

    init_flag = true;
}

uint8_t TwoWire::endTransmission(bool sendStop)
{
    size_t buffer = _tx_buffer.size();

    if (sendStop == true)
    {
        if (buffer == 0)
        {
            _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "endTransmission fail (_tx_buffer length == 0)\n");
            return -1;
        }

        if (_bus->write(_tx_buffer.data(), buffer) == false)
        {
            _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "write fail\n");
            return -1;
        }

        // for (size_t i = 0; i < buffer; i++)
        // {
        //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "tx1[%d]: %#X\n", i, _tx_buffer[i]);
        // }
        _tx_buffer.clear();
    }
    else
    {
    }

    return buffer;
}

size_t TwoWire::requestFrom(uint16_t address, size_t size, bool sendStop)
{
    // if (is_slave)
    // {
    //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "bus is in slave mode\n");
    //     return 0;
    // }

    size_t buffer = _tx_buffer.size();

    if (buffer == 0)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "requestFrom fail (_tx_buffer length == 0)\n");
        return 0;
    }

    _rx_index = 0;
    _rx_buffer = std::make_unique<uint8_t[]>(size);
    if (_bus->write_read(_tx_buffer.data(), buffer, _rx_buffer.get(), size) == false)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "write_read fail\n");
        return 0;
    }

    // for (size_t i = 0; i < buffer; i++)
    // {
    //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "tx2[%d]: %#X\n", i, _tx_buffer[i]);
    // }

    // for (size_t i = 0; i < size; i++)
    // {
    //     _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::DEBUG, __FILE__, __LINE__, "rx[%d]: %#X\n", i, _rx_buffer[i]);
    // }

    _tx_buffer.clear();
    _rx_length = size;

    return _rx_length;
}

size_t TwoWire::write(uint8_t data)
{
    write(&data, 1);
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    _tx_buffer.insert(_tx_buffer.end(), data, data + quantity);
    return quantity;
}

int TwoWire::available(void)
{
    int result = _rx_length - _rx_index;
    return result;
}

int TwoWire::read(void)
{
    int value = -1;
    if (_rx_buffer.get() == NULL)
    {
        _bus->assert_log(Cpp_Bus_Driver::Tool::Log_Level::BUS, __FILE__, __LINE__, "null rx buffer pointer\n");
        return value;
    }
    if (_rx_index < _rx_length)
    {
        value = _rx_buffer[_rx_index++];
    }
    return value;
}

// int TwoWire::peek(void)
// {
//     // int value = -1;
//     // if (_rx_buffer == NULL)
//     // {
//     //     log_e("NULL RX buffer pointer");
//     //     return value;
//     // }
//     // if (_rx_index < _rx_length)
//     // {
//     //     value = _rx_buffer[_rx_index];
//     // }
//     // return value;
//     return -1;
// }

void TwoWire::flush(void)
{
    _rx_index = 0;
    _rx_length = 0;
}

size_t TwoWire::requestFrom(uint8_t address, size_t len, bool sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(address, static_cast<size_t>(len), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, bool stopBit)
{
    return requestFrom((uint16_t)address, (size_t)len, stopBit);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len)
{
    return requestFrom(address, static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len, int sendStop)
{
    return static_cast<uint8_t>(requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop)));
}

void TwoWire::beginTransmission(int address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

void TwoWire::beginTransmission(uint8_t address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(true);
}

// size_t TwoWire::slaveWrite(const uint8_t *buffer, size_t len)
// {
//     // return i2cSlaveWrite(num, buffer, len, _timeOutMillis);
//     return -1;
// }

// void TwoWire::onReceiveService(uint8_t num, uint8_t *inBytes, size_t numBytes, bool stop, void *arg)
// {
//     // TwoWire *wire = (TwoWire *)arg;
//     // if (!wire->user_onReceive)
//     // {
//     //     return;
//     // }
//     // if (wire->_rx_buffer == NULL)
//     // {
//     //     log_e("NULL RX buffer pointer");
//     //     return;
//     // }
//     // for (uint8_t i = 0; i < numBytes; ++i)
//     // {
//     //     wire->_rx_buffer[i] = inBytes[i];
//     // }
//     // wire->_rx_index = 0;
//     // wire->_rx_length = numBytes;
//     // wire->user_onReceive(numBytes);
// }

// void TwoWire::onRequestService(uint8_t num, void *arg)
// {
//     // TwoWire *wire = (TwoWire *)arg;
//     // if (!wire->user_onRequest)
//     // {
//     //     return;
//     // }
//     // if (wire->_tx_buffer == NULL)
//     // {
//     //     log_e("NULL TX buffer pointer");
//     //     return;
//     // }
//     // wire->txLength = 0;
//     // wire->user_onRequest();
//     // if (wire->txLength)
//     // {
//     //     wire->slaveWrite((uint8_t *)wire->_tx_buffer, wire->txLength);
//     // }
// }

// void TwoWire::onReceive(void (*function)(int))
// {
//     user_onReceive = function;
// }

// // sets function called on slave read
// void TwoWire::onRequest(void (*function)(void))
// {
//     user_onRequest = function;
// }

TwoWire Wire = TwoWire(0);
TwoWire Wire1 = TwoWire(1);
