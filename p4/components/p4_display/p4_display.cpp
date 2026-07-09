/*
 * p4_display — RM69A10 MIPI-DSI AMOLED bring-up + LVGL 8.3 self-test.
 * See p4_display.h for the power/reset ordering contract (caller-owned).
 */
#include "p4_display.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_cache.h"

#include "t_display_p4_config.h" // RM69A10_* panel constants (board_hw)
#include "board_touch.h"         // board_touch_read()
#include "rm69a10_driver.h"

#include "lvgl.h"

static const char *TAG = "p4_display";

// ---- Panel geometry / DSI timings (from t_display_p4_config.h) ----------------
#define P4D_WIDTH            RM69A10_SCREEN_WIDTH               // 568
#define P4D_HEIGHT           RM69A10_SCREEN_HEIGHT              // 1232
#define P4D_DPI_CLK_MHZ      RM69A10_SCREEN_MIPI_DSI_DPI_CLK_MHZ // 60
#define P4D_HSYNC            RM69A10_SCREEN_MIPI_DSI_HSYNC       // 50
#define P4D_HBP              RM69A10_SCREEN_MIPI_DSI_HBP         // 150
#define P4D_HFP              RM69A10_SCREEN_MIPI_DSI_HFP         // 50
#define P4D_VSYNC            RM69A10_SCREEN_MIPI_DSI_VSYNC       // 40
#define P4D_VBP              RM69A10_SCREEN_MIPI_DSI_VBP         // 120
#define P4D_VFP              RM69A10_SCREEN_MIPI_DSI_VFP         // 80
#define P4D_LANES            RM69A10_SCREEN_DATA_LANE_NUM        // 2
#define P4D_LANE_MBPS        RM69A10_SCREEN_LANE_BIT_RATE_MBPS   // 1000
#define P4D_BITS_PER_PIXEL   16                                  // RGB565

#define P4D_LDO_CHAN         3      // MIPI DPHY LDO channel
#define P4D_LDO_MV           1830   // MIPI DPHY voltage (mV)

// ---- File-static handles ------------------------------------------------------
static esp_lcd_panel_handle_t   s_panel      = NULL;   // RM69A10 (wraps DPI panel)
static esp_ldo_channel_handle_t s_ldo        = NULL;   // kept alive => rail stays on
static bool                     s_inited     = false;

// Flush-ready hook (LVGL wires this in the self-test).
static bool (*s_flush_cb)(void *) = NULL;
static void  *s_flush_ctx         = NULL;

// -----------------------------------------------------------------------------
static bool p4d_ldo_power_on(void)
{
    if (s_ldo) {
        return true;
    }
    esp_ldo_channel_config_t cfg = {
        .chan_id    = P4D_LDO_CHAN,
        .voltage_mv = P4D_LDO_MV,
    };
    esp_err_t err = esp_ldo_acquire_channel(&cfg, &s_ldo);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ldo_acquire_channel(%d,%dmV) failed: %s", P4D_LDO_CHAN, P4D_LDO_MV, esp_err_to_name(err));
        return false;
    }
    return true;
}

// Check an esp_err_t; on failure log + return false from the enclosing (bool) fn.
#define P4D_CHECK(expr, msg)                                             \
    do {                                                                 \
        esp_err_t _e = (expr);                                           \
        if (_e != ESP_OK) {                                              \
            ESP_LOGE(TAG, "%s: %s", (msg), esp_err_to_name(_e));         \
            return false;                                                \
        }                                                                \
    } while (0)

bool p4_display_init(void (*reset_pulse)(void))
{
    if (s_inited) {
        return true;
    }

    // 1) MIPI DPHY power MUST come up before any DSI init, or the DSI PHY
    //    bring-up hangs and trips the task WDT.
    if (!p4d_ldo_power_on()) {
        return false;
    }

    // 1b) Reference order: after the LDO, settle, then pulse the panel's
    //     hardware reset (XL9535 IO2, owned by the caller) before creating the
    //     DSI bus/panel.
    vTaskDelay(pdMS_TO_TICKS(100));
    if (reset_pulse) {
        reset_pulse();
    }

    // 2) Create the MIPI-DSI bus (also inits the DSI PHY).
    esp_lcd_dsi_bus_handle_t   dsi_bus = NULL;
    esp_lcd_panel_io_handle_t  dbi_io  = NULL;

    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id             = 0,
        .num_data_lanes     = P4D_LANES,
        .phy_clk_src        = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
        .lane_bit_rate_mbps = P4D_LANE_MBPS,
    };
    P4D_CHECK(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus), "esp_lcd_new_dsi_bus failed");

    // 3) DBI command IO (used to send the vendor init table).
    esp_lcd_dbi_io_config_t dbi_config = {
        .virtual_channel = 0,
        .lcd_cmd_bits    = 8,
        .lcd_param_bits  = 8,
    };
    P4D_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io), "esp_lcd_new_panel_io_dbi failed");

    // 4) DPI (video) panel config — RGB565, config timings, DMA2D copy.
    esp_lcd_dpi_panel_config_t dpi_config = {
        .virtual_channel   = 0,
        .dpi_clk_src       = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = P4D_DPI_CLK_MHZ,
        .pixel_format      = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .num_fbs           = 1,
        .video_timing = {
            .h_size            = P4D_WIDTH,
            .v_size            = P4D_HEIGHT,
            .hsync_pulse_width = P4D_HSYNC,
            .hsync_back_porch  = P4D_HBP,
            .hsync_front_porch = P4D_HFP,
            .vsync_pulse_width = P4D_VSYNC,
            .vsync_back_porch  = P4D_VBP,
            .vsync_front_porch = P4D_VFP,
        },
        .flags = {
            .use_dma2d = true,   // match the working LilyGo reference
        },
    };

    // 5) RM69A10 vendor panel (wraps the DPI panel + init table).
    // Match the reference vendor_config exactly: only dsi_bus + dpi_config.
    // (Setting mipi_config.lane_num here is not done by the reference.)
    rm69a10_vendor_config_t vendor_config = {};
    vendor_config.mipi_config.dsi_bus    = dsi_bus;
    vendor_config.mipi_config.dpi_config = &dpi_config;

    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,                       // reset is via XL9535 (caller); software reset otherwise
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = P4D_BITS_PER_PIXEL,
        .vendor_config  = &vendor_config,
    };
    P4D_CHECK(esp_lcd_new_panel_rm69a10(dbi_io, &dev_config, &s_panel), "esp_lcd_new_panel_rm69a10 failed");

    // 6) Init only — sends the vendor table (incl. sleep-out 0x11 + display-on
    //    0x29). The reference does NOT call esp_lcd_panel_reset() (a DBI SWRESET
    //    without the required post-reset delay can corrupt the init table); the
    //    hardware reset pulse (done above) is the panel reset.
    P4D_CHECK(esp_lcd_panel_init(s_panel), "panel init failed");

    s_inited = true;
    ESP_LOGI(TAG, "RM69A10 %dx%d up (DPI %dMHz, %d lanes @ %d Mbps)",
             P4D_WIDTH, P4D_HEIGHT, P4D_DPI_CLK_MHZ, P4D_LANES, P4D_LANE_MBPS);
    return true;
}

void p4_display_set_brightness(uint8_t v)
{
    if (s_panel) {
        set_rm69a10_brightness(s_panel, v);
    }
}

esp_lcd_panel_handle_t p4_display_panel(void)
{
    return s_panel;
}

// DPI ISR trampoline -> user flush-ready cb.
static bool IRAM_ATTR p4d_on_color_trans_done(esp_lcd_panel_handle_t panel,
                                              esp_lcd_dpi_panel_event_data_t *edata,
                                              void *user_ctx)
{
    if (s_flush_cb) {
        return s_flush_cb(s_flush_ctx);
    }
    return false;
}

void p4_display_register_flush_ready_cb(bool (*cb)(void *), void *ctx)
{
    s_flush_cb  = cb;
    s_flush_ctx = ctx;
    if (!s_panel) {
        ESP_LOGE(TAG, "register_flush_ready_cb before panel init");
        return;
    }
    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    cbs.on_color_trans_done = p4d_on_color_trans_done;
    esp_err_t err = esp_lcd_dpi_panel_register_event_callbacks(s_panel, &cbs, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register DPI event cbs failed: %s", esp_err_to_name(err));
    }
}

// ============================ LVGL 8.3 self-test =============================
#define P4D_LVGL_TICK_MS   1
#define P4D_DRAW_LINES     120   // partial-render buffer height (~1/10 screen)

static lv_disp_drv_t      s_disp_drv;
static lv_disp_draw_buf_t s_draw_buf;

static void p4d_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    // DMA2D copies the LVGL buffer into the scanned frame buffer (handles the
    // source cache write-back). flush_ready is signalled from the DPI
    // on_color_trans_done ISR when the copy completes (reference pattern).
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}

static bool p4d_flush_ready_hook(void *ctx)
{
    lv_disp_drv_t *drv = (lv_disp_drv_t *)ctx;
    lv_disp_flush_ready(drv);
    return false;
}

static void p4d_lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(P4D_LVGL_TICK_MS);
}

// While a flush is in-flight and LVGL needs the buffer back, yield the CPU
// (block briefly) instead of busy-spinning — otherwise the LVGL task pins its
// core and starves the idle task -> task WDT.
static void p4d_wait_cb(lv_disp_drv_t *drv)
{
    (void)drv;
    vTaskDelay(1);
}

// ---- Touch indev (GT9895 via board_touch_read) ----
static lv_indev_drv_t s_indev_drv;
static lv_obj_t      *s_status_label = NULL;

static void p4d_touchpad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    int16_t x = 0, y = 0;
    if (board_touch_read(&x, &y)) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void p4d_btn_event_cb(lv_event_t *e)
{
    (void)e;
    static int n = 0;
    if (s_status_label) {
        lv_label_set_text_fmt(s_status_label, "TOUCH OK x%d", ++n);
    }
}

// Bring up LVGL 8.3 over the (proven) panel: double-buffered partial render in
// PSRAM (DMA2D flush + on_color_trans_done flush-ready), the GT9895 touch indev,
// and the tick timer. Returns the registered display (NULL on failure). The
// panel itself must already be up (p4_display_init done by main). Shared by the
// self-test AND by UITask on the P4 (the esp_lcd display backend).
extern "C" lv_disp_t *p4_display_lvgl_begin(void)
{
    if (!p4_display_init(NULL)) {
        ESP_LOGE(TAG, "lvgl_begin: panel not inited");
        return NULL;
    }
    lv_init();

    const int DRAW_LINES = 120;   // ~1/10 screen per partial buffer
    size_t buf_px = (size_t)P4D_WIDTH * DRAW_LINES;
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t),
                                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t),
                                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "lvgl_begin: draw buffer alloc failed");
        return NULL;
    }
    lv_disp_draw_buf_init(&s_draw_buf, buf1, buf2, buf_px);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res   = P4D_WIDTH;
    s_disp_drv.ver_res   = P4D_HEIGHT;
    s_disp_drv.flush_cb  = p4d_lvgl_flush_cb;
    s_disp_drv.draw_buf  = &s_draw_buf;
    s_disp_drv.wait_cb   = p4d_wait_cb;   // yield (not busy-spin) while flushing
    s_disp_drv.user_data = p4_display_panel();
    lv_disp_t *disp = lv_disp_drv_register(&s_disp_drv);

    // flush-ready via the DPI on_color_trans_done ISR (DMA2D copy complete).
    p4_display_register_flush_ready_cb(p4d_flush_ready_hook, &s_disp_drv);

    // Touch input device (GT9895 via board_touch_read).
    lv_indev_drv_init(&s_indev_drv);
    s_indev_drv.type    = LV_INDEV_TYPE_POINTER;
    s_indev_drv.read_cb = p4d_touchpad_read_cb;
    lv_indev_drv_register(&s_indev_drv);

    // LVGL tick from esp_timer.
    const esp_timer_create_args_t tick_args = {
        .callback = p4d_lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, P4D_LVGL_TICK_MS * 1000);

    p4_display_set_brightness(255);
    return disp;
}

static void p4d_selftest_task(void *arg)
{
    (void)arg;

    lv_disp_t *disp = p4_display_lvgl_begin();
    if (!disp) {
        ESP_LOGE(TAG, "selftest: lvgl_begin failed");
        vTaskDelete(NULL);
        return;
    }

    // Proof UI: blue background + centered label.
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0033AA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    s_status_label = lv_label_create(scr);
    lv_label_set_text(s_status_label, "T-Display-P4 LVGL OK");
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, -80);

    // Touch proof: a button that updates the label when tapped.
    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 240, 90);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn, p4d_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btnlbl = lv_label_create(btn);
    lv_label_set_text(btnlbl, "Touch me");
    lv_obj_set_style_text_font(btnlbl, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_center(btnlbl);

    p4_display_set_brightness(255);

    ESP_LOGI(TAG, "selftest: LVGL UI up; entering handler loop");
    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));   // 5 ticks @1000Hz FreeRTOS tick
    }
}

void p4_display_selftest(void)
{
    // Generous stack: LVGL rendering + label draw.
    xTaskCreatePinnedToCore(p4d_selftest_task, "p4d_selftest", 8 * 1024, NULL, 4, NULL, 1);
}
