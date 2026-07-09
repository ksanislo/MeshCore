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

/* Route LVGL's heap to PSRAM. The full UITask UI (all screens/panes/fonts) needs
 * far more than the 48 KB bring-up pool did (the S3 build uses a 256 KB internal
 * pool); on the P4 we have 32 MB PSRAM, so back LV_MEM with heap_caps SPIRAM and
 * keep internal RAM free for stacks + the DMA draw buffers. */
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE        "esp_heap_caps.h"
#define LV_MEM_CUSTOM_ALLOC(sz)      heap_caps_malloc((sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_REALLOC(p, sz) heap_caps_realloc((p), (sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_FREE           heap_caps_free

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

/* Complex draw (arcs, rounded masks, letter-by-letter text selection). */
#define LV_DRAW_COMPLEX 1

/*==================
   FONT USAGE
 *==================*/

/* The shared UI's type ramp + structural metrics use these montserrat sizes;
 * keep in sync with FONT_RAMP in UITask (Small/Medium/Large tiers). */
#define LV_FONT_MONTSERRAT_8            0
#define LV_FONT_MONTSERRAT_10           1
#define LV_FONT_MONTSERRAT_12           1
#define LV_FONT_MONTSERRAT_14           1
#define LV_FONT_MONTSERRAT_16           1
#define LV_FONT_MONTSERRAT_18           1
#define LV_FONT_MONTSERRAT_20           1
#define LV_FONT_MONTSERRAT_24           1
#define LV_FONT_MONTSERRAT_28           1
#define LV_FONT_UNSCII_8                1
#define LV_FONT_DEFAULT                 &lv_font_montserrat_14

/*==================
   TEXT SETTINGS
 *==================*/

/* SOH control char as the recolor command (NOT '#') so a literal '#' hashtag can
 * be recolored; the UI generates all recolor markup itself and strips \x01 from
 * input. MUST match the S3 lv_conf.h or chat recoloring breaks. */
#define LV_TXT_COLOR_CMD                "\x01"
#define LV_TXT_LINE_BREAK_LONG_LEN      0

/*==================
   WIDGET USAGE
 *==================*/

#define LV_USE_LABEL 1
#define LV_LABEL_TEXT_SELECTION 1
#define LV_LABEL_LONG_TXT_HINT  1

/*==================
   EXTRA FEATURES the shared UI needs
 *==================*/

#define LV_USE_IMGFONT 1      // color-emoji imgfont fallback (withEmoji)
#define LV_USE_QRCODE  1      // share / contact QR codes
#define LV_USE_SPAN    1
#define LV_SPAN_SNIPPET_STACK_SIZE 64
// Decoded-image cache so scrolling emoji aren't re-read every frame.
#define LV_IMG_CACHE_DEF_SIZE 48

#endif /*LV_CONF_H*/

#endif /*End of "Enable this file"*/
