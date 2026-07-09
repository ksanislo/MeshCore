# p4_display — RM69A10 MIPI-DSI AMOLED bring-up + LVGL 8.3 self-test.
#
# MIPI-DSI on the ESP32-P4 is built into the core `esp_lcd` component in
# IDF 5.4.1 (components/esp_lcd/dsi) — there is NO separate esp_lcd_mipi_dsi
# component to require. esp_lcd pulls driver/esp_driver_gpio/esp_mm itself.
# esp_hw_support provides esp_ldo_regulator.h (MIPI DPHY power).
# lvgl is the managed component (see main/idf_component.yml).
idf_component_register(
    SRCS
        "rm69a10_driver.cpp"
        "p4_display.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        esp_hw_support
        esp_mm
        driver
        board_hw
        cpp_bus_driver
        lvgl
)

# The vendored RM69A10 driver reuses esp_lcd designated-initializer patterns and
# a C++ compile; keep a couple of upstream-style warnings non-fatal.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=missing-field-initializers
    -Wno-error=unused-variable
)
