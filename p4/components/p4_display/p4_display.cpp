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
            // DMA2D off -> draw_bitmap copies synchronously (blocks until the
            // pixels are in the frame buffer), so the LVGL flush can signal
            // flush-ready inline instead of relying on the on_color_trans_done
            // ISR (which wasn't firing -> LVGL busy-waited its buffer -> WDT).
            .use_dma2d = false,
        },
    };

    // 5) RM69A10 vendor panel (wraps the DPI panel + init table).
    rm69a10_vendor_config_t vendor_config = {};
    vendor_config.mipi_config.dsi_bus    = dsi_bus;
    vendor_config.mipi_config.dpi_config = &dpi_config;
    vendor_config.mipi_config.lane_num   = P4D_LANES;

    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = -1,                       // reset is via XL9535 (caller); software reset otherwise
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = P4D_BITS_PER_PIXEL,
        .vendor_config  = &vendor_config,
    };
    P4D_CHECK(esp_lcd_new_panel_rm69a10(dbi_io, &dev_config, &s_panel), "esp_lcd_new_panel_rm69a10 failed");

    // 6) Reset (software SWRESET), init (sends vendor table incl. sleep-out +
    //    display-on), then explicit display-on for good measure.
    P4D_CHECK(esp_lcd_panel_reset(s_panel), "panel reset failed");
    P4D_CHECK(esp_lcd_panel_init(s_panel), "panel init failed");
    P4D_CHECK(esp_lcd_panel_disp_on_off(s_panel, true), "panel disp_on failed");

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
    (void)area; (void)color_p;
    // LVGL renders straight into the scanned DPI frame buffer (full_refresh +
    // draw_buf == the panel FB), so there is nothing to copy — just release.
    // (Avoids esp_lcd_panel_draw_bitmap, which blocked on the DPI FB sync.)
    lv_disp_flush_ready(drv);
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

static void p4d_selftest_task(void *arg)
{
    (void)arg;

    // Panel already inited by main (with the reset callback); idempotent.
    if (!p4_display_init(NULL)) {
        ESP_LOGE(TAG, "selftest: panel init failed");
        vTaskDelete(NULL);
        return;
    }

    // Panel proof, EXACTLY the reference pattern (screen_lvgl standalone test):
    // push full-screen solid colors via esp_lcd_panel_draw_bitmap, which does
    // the DMA2D copy into the scanned frame buffer + cache write-back. A
    // cache-line-aligned PSRAM buffer is required. No LVGL yet — this isolates
    // the panel/DSI/brightness from the LVGL integration.
    const size_t px = (size_t)P4D_WIDTH * P4D_HEIGHT;
    uint16_t *cbuf = (uint16_t *)heap_caps_aligned_calloc(
        64, 1, px * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!cbuf) {
        ESP_LOGE(TAG, "selftest: buffer alloc failed");
        vTaskDelete(NULL);
        return;
    }

    p4_display_set_brightness(255);

    const uint16_t colors[] = {0xF800 /*red*/, 0x07E0 /*green*/,
                               0x001F /*blue*/, 0xFFFF /*white*/};
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE"};
    int c = 0;
    ESP_LOGI(TAG, "selftest: draw_bitmap color cycle");
    while (true) {
        for (size_t i = 0; i < px; i++) cbuf[i] = colors[c];
        esp_err_t e = esp_lcd_panel_draw_bitmap(p4_display_panel(), 0, 0,
                                                P4D_WIDTH, P4D_HEIGHT, cbuf);
        printf("[disp] draw_bitmap fill: %-5s (err=%s)  <-- LOOK AT SCREEN\n",
               names[c], esp_err_to_name(e));
        c = (c + 1) % 4;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void p4_display_selftest(void)
{
    // Generous stack: LVGL rendering + label draw.
    xTaskCreatePinnedToCore(p4d_selftest_task, "p4d_selftest", 8 * 1024, NULL, 4, NULL, 1);
}
