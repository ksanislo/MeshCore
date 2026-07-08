/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-18 14:54:01
 * @LastEditTime: 2026-01-21 14:36:24
 * @License: GPL 3.0
 */
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>
#include <functional>
#include <string.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <stdarg.h>
#include <sstream>

#if defined CONFIG_IDF_INIT_VERSION
#define DEVELOPMENT_FRAMEWORK_ESPIDF

#include "driver/i2c_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_timer.h"

#elif defined ARDUINO
#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"

#if defined NRF52840_XXAA

#define DEVELOPMENT_FRAMEWORK_ARDUINO_NRF
#define DEVELOPMENT_FRAMEWORK_CPP11_SUPPORT
#define CUSTOM_TEMPLATE_MAKE_UNIQUE

#undef LOW
#undef HIGH

#undef INPUT
#undef OUTPUT

#undef RISING
#undef FALLING
#undef CHANGE

#include "nrfx_i2s.h"

#endif

#else
#error "development framework not selected"
#endif

#include "tool.h"

#define CPP_BUS_DRIVER_LOG_LEVEL_DEBUG
#define CPP_BUS_DRIVER_LOG_LEVEL_INFO
#define CPP_BUS_DRIVER_LOG_LEVEL_BUS
#define CPP_BUS_DRIVER_LOG_LEVEL_CHIP

#define DEFAULT_CPP_BUS_DRIVER_VALUE -1

#define DEFAULT_CPP_BUS_DRIVER_IIC_FREQ_HZ 100000
#define DEFAULT_CPP_BUS_DRIVER_IIC_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_SPI_FREQ_HZ 10000000

#define DEFAULT_CPP_BUS_DRIVER_QSPI_FREQ_HZ 10000000
#define DEFAULT_CPP_BUS_DRIVER_QSPI_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_UART_BAUD_RATE 115200
#define DEFAULT_CPP_BUS_DRIVER_UART_WAIT_TIMEOUT_MS 1000

#define DEFAULT_CPP_BUS_DRIVER_IIS_WAIT_TIMEOUT_MS 1000

#if defined DEVELOPMENT_FRAMEWORK_ESPIDF
#define DEFAULT_CPP_BUS_DRIVER_SDIO_FREQ_HZ SDMMC_FREQ_DEFAULT
#endif

#if defined CUSTOM_TEMPLATE_MAKE_UNIQUE
namespace std
{
#if defined DEVELOPMENT_FRAMEWORK_CPP11_SUPPORT
    // C++ 11
    //  通用模板（非数组类型）
    template <typename T, typename... Args, typename = typename std::enable_if<!std::is_array<T>::value>::type>
    std::unique_ptr<T> make_unique(Args &&...args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    // 特化模板（动态数组类型）
    template <typename T, typename = typename std::enable_if<std::is_array<T>::value>::type>
    std::unique_ptr<T> make_unique(size_t size)
    {
        using U = typename std::remove_extent<T>::type; // 获取数组元素类型
        return std::unique_ptr<T>(new U[size]());       // Value initialization
    }
#elif defined DEVELOPMENT_FRAMEWORK_CPP14_SUPPORT
    // C++ 14
    // 通用模板（非数组类型）
    template <typename T, typename... Args, typename = std::enable_if_t<!std::is_array<T>::value>>
    std::unique_ptr<T> make_unique(Args &&...args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    // 特化模板（动态数组类型）
    template <typename T, typename = std::enable_if_t<std::is_array<T>::value>>
    std::unique_ptr<T> make_unique(size_t size)
    {
        using U = typename std::remove_extent<T>::type; // 获取数组元素类型
        return std::unique_ptr<T>(new U[size]());       // Value initialization
    }

#else
#error "development framework not selected"
#endif
}
#endif
