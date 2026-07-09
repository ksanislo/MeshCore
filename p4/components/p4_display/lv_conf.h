/**
 * lv_conf.h — LVGL 8.3 configuration for the T-Display-P4 (RM69A10 DSI AMOLED).
 *
 * Vendored (not Kconfig): the build sets CONFIG_LV_CONF_SKIP=n and points
 * LVGL at this file via -DLV_CONF_PATH=<abs path> (see the top-level
 * CMakeLists.txt). LVGL's lv_conf_internal.h supplies sane defaults via
 * `#ifndef` for every option we don't set here, so this file only needs to
 * override what matters for bring-up. Expand it when the full UITask lands.
 */
#if 1 /* Enable this file */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* 16-bit RGB565 to match the DPI panel pixel format. */
#define LV_COLOR_DEPTH 16

/* MIPI-DSI DPI wants native-endian RGB565 (no byte swap). If colors look
 * wrong on hardware (e.g. red/blue inverted byte order), flip this to 1. */
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* LVGL's internal allocator. 48 KB is comfortable for the bring-up UI. */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)

/*====================
   HAL SETTINGS
 *====================*/

/* We drive lv_tick_inc() from an esp_timer, so no custom tick source. */
#define LV_TICK_CUSTOM 0

/* Default display refresh / input read periods (ms). */
#define LV_DISP_DEF_REFR_PERIOD 20
#define LV_INDEV_DEF_READ_PERIOD 20

/*=======================
   FEATURE CONFIGURATION
 *=======================*/

/* Logging: warnings+ to the console via printf. */
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
#endif

/*==================
   FONT USAGE
 *==================*/

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_28 1

/* Default text font. */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
   WIDGET USAGE
 *==================*/

#define LV_USE_LABEL 1

#endif /*LV_CONF_H*/

#endif /*End of "Enable this file"*/
