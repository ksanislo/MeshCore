/*
 * p4_display — RM69A10 MIPI-DSI AMOLED (568x1232) bring-up for the
 * LilyGo T-Display-P4.
 *
 * Owns the esp_lcd DSI/DPI init path (LDO PHY power, DSI bus, DBI IO, DPI
 * panel, RM69A10 vendor init) and an LVGL 8.3 self-test that proves the panel.
 *
 * IMPORTANT — power/reset ordering (done by the CALLER, not here):
 *   The XL9535 power rails AND the panel reset pulse (expander IO2 =
 *   XL9535_SCREEN_RST) must already be applied before p4_display_init() runs.
 *   p4_display_init() does NOT touch the XL9535 expander. The DPI panel is
 *   created with reset_gpio_num = -1, so its reset() is a DSI software reset
 *   only — it relies on the hardware reset having been released HIGH first.
 *   See notes in main.cpp's board_power_up().
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bring up the RM69A10 DSI panel.
 *
 * Acquires LDO channel 3 @ 1830 mV (MIPI DPHY power — MUST precede panel init),
 * creates the DSI bus (2 data lanes, 1000 Mbps), the DBI command IO, and the
 * DPI panel (RGB565, 60 MHz pixel clock, config timings, use_dma2d), then runs
 * the RM69A10 vendor init and turns the display on. Idempotent: repeated calls
 * after the first success are no-ops.
 *
 * @return true on success (or already-initialized), false on any esp_lcd error.
 */
// reset_pulse: called AFTER the MIPI DPHY LDO is up but BEFORE the DSI bus/panel
// are created, so the caller can pulse XL9535_SCREEN_RST in the reference order
// (LDO -> delay -> reset HIGH/LOW/HIGH -> panel). Pass NULL if already reset.
bool p4_display_init(void (*reset_pulse)(void));

/**
 * @brief Set panel brightness (DCS 0x51), 0..255.
 */
void p4_display_set_brightness(uint8_t v);

/**
 * @brief Get the esp_lcd panel handle (NULL until p4_display_init() succeeds).
 */
esp_lcd_panel_handle_t p4_display_panel(void);

/**
 * @brief Register a "DPI color transfer done" callback (flush-ready hook).
 *
 * Wraps esp_lcd_dpi_panel_register_event_callbacks(on_color_trans_done). The
 * supplied cb is invoked from the DPI ISR when a draw_bitmap transfer finishes;
 * return value is forwarded to esp_lcd (return true if a higher-prio task was
 * woken). Used by the LVGL flush pipeline to call lv_disp_flush_ready().
 *
 * @param cb   callback taking the user ctx; may run in ISR context.
 * @param ctx  opaque pointer passed back to cb.
 */
void p4_display_register_flush_ready_cb(bool (*cb)(void *), void *ctx);

/**
 * @brief On-device proof: init LVGL 8.3, wire flush/tick, draw a full-screen
 *        colored background + centered label, ramp brightness to max, and spin
 *        an LVGL task. Calls p4_display_init() first if not already done.
 *
 * Spawns its own FreeRTOS task and returns immediately.
 */
void p4_display_selftest(void);

#ifdef __cplusplus
}
#endif
