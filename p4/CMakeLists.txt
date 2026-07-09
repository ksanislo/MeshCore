# ESP-IDF project root for the ESP32-P4 touch-panel firmware.
cmake_minimum_required(VERSION 3.16.0)

# Select the P4 ESP-IDF platform branch in vendored MeshCore headers
# (e.g. helpers/IdentityStore.h -> Arduino fs::FS via components/fs_shim).
# Must precede project() so it applies to every IDF component compile.
add_compile_definitions(P4_IDF_PLATFORM=1)

# Point LVGL at our vendored lv_conf.h (CONFIG_LV_CONF_SKIP=n). lv_conf_internal.h
# resolves LV_CONF_PATH before LV_CONF_INCLUDE_SIMPLE, so this wins cleanly
# without polluting include paths. Applied globally so the LVGL managed
# component sees it. Must precede project().
add_compile_definitions(LV_CONF_PATH=${CMAKE_CURRENT_LIST_DIR}/components/p4_display/lv_conf.h)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tdisplay_p4)
