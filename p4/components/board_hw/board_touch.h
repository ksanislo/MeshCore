/*
 * board_touch.h — variant touch read hook for the T-Display P4.
 *
 * The GT9895 touch controller shares the IIC-1 bus with the XL9535 expander, so
 * it (and the read) live in main.cpp alongside the other hardware globals. UI
 * code (p4_display self-test, later UITask) calls board_touch_read() to feed an
 * LVGL pointer indev, without knowing the chip.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Construct/reset/begin the touch controller. Call after XL9535->begin().
void board_touch_init(void);

// Returns true if the panel is currently touched; *x/*y in panel pixels
// (already scaled to 568x1232 by the driver). Returns false when not touched.
bool board_touch_read(int16_t *x, int16_t *y);

#ifdef __cplusplus
}
#endif
