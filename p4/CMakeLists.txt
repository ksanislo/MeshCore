# ESP-IDF project root for the ESP32-P4 touch-panel firmware.
cmake_minimum_required(VERSION 3.16.0)

# Select the P4 ESP-IDF platform branch in vendored MeshCore headers
# (e.g. helpers/IdentityStore.h -> Arduino fs::FS via components/fs_shim).
# Must precede project() so it applies to every IDF component compile.
add_compile_definitions(P4_IDF_PLATFORM=1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tdisplay_p4)
