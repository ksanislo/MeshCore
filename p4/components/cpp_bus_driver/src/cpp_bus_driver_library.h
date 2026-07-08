/*
 * @Description: None
 * @Author: LILYGO_L
 * @Date: 2024-12-16 17:14:18
 * @LastEditTime: 2026-01-16 16:18:21
 * @License: GPL 3.0
 */

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <numeric>

// #include "assert.h"

#include "./bus/iic/hardware_iic_1.h"
#include "./bus/iic/hardware_iic_2.h"
#include "./bus/iic/software_iic.h"
#include "./bus/spi/hardware_spi.h"
#include "./bus/spi/hardware_qspi.h"
#include "./bus/uart/hardware_uart.h"
#include "./bus/iis/hardware_iis.h"
#include "./bus/sdio/hardware_sdio.h"

#include "./chip/iic/xl95x5.h"
#include "./chip/iic/aw862xx.h"
#include "./chip/iic/sgm38121.h"
#include "./chip/iic/pcf8563x.h"
#include "./chip/iic/bq27220xxxx.h"
#include "./chip/iic/hi8561_touch.h"
#include "./chip/iic/cst2xxse.h"
#include "./chip/iic_iis/es8311.h"
#include "./chip/iic/ft3x68.h"
#include "./chip/iic/gt9895.h"
#include "./chip/iic/sgm41562xx.h"
#include "./chip/iic/tca8418.h"
#include "./chip/iic/gz030pcc0x.h"
#include "./chip/iic/aw21009xxx.h"
#include "./chip/iic/sy6970.h"
#include "./chip/iic/icn6211.h"
#include "./chip/spi/sx126x.h"
#include "./chip/spi/ecx336cn.h"
#include "./chip/spi/co5300.h"
#include "./chip/spi/sh8601.h"
#include "./chip/uart/l76k.h"
#include "./chip/sdio/esp_at.h"

#if defined DEVELOPMENT_FRAMEWORK_ARDUINO_NRF

#define LOW (0x0)
#define HIGH (0x1)

#define INPUT (0x0)
#define OUTPUT (0x1)

#define CHANGE 2
#define FALLING 3
#define RISING 4

#endif
