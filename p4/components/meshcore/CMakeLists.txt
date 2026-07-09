# MeshCore mesh core + ed25519 crypto, vendored from our src/ (== upstream fork). Phase 1: get the
# protocol core + crypto compiling under IDF 5.4.1 with the arduino_compat shim. Radio/store/app next.
file(GLOB CORE_SRCS "meshcore_src/*.cpp" "meshcore_src/helpers/*.cpp")
# Exclude platform/transport/UI/sensor helpers whose Arduino/BLE/WiFi/I2C-RTC deps
# are unavailable (or unwanted) on the P4 IDF build. The mesh node links only the
# protocol-core helpers: BaseChatMesh, StaticPoolPacketManager, Advert/Txt/Region
# helpers, ClientACL, TransportKeyStore, IdentityStore (against the fs_shim). The
# helper subdirs (esp32/nrf52/radiolib/ui/bridges/sensors/stm32) aren't globbed.
list(FILTER CORE_SRCS EXCLUDE REGEX
    "helpers/(ArduinoSerialInterface|ESP32Board|NRF52Board|CommonCLI|RTC_RX8130CE|AutoDiscoverRTCClock)\\.cpp$")
file(GLOB ED25519_SRCS "meshcore_src/ed25519/*.c")

idf_component_register(
    SRCS
        ${CORE_SRCS}
        ${ED25519_SRCS}
        "arduino_compat.cpp"
        "p4_radio.cpp"
        "p4_node.cpp"
    INCLUDE_DIRS
        "."
        "meshcore_src"
        "meshcore_src/helpers"
        "meshcore_src/ed25519"
    REQUIRES
        fs_shim
        arduino_cpp_bus_driver
        cpp_bus_driver
        board_hw
        mbedtls
        nvs_flash
        esp_timer
        esp_system
        esp_hw_support
        freertos
        driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=reorder -Wno-error=unused-variable -Wno-error=unused-function
    -Wno-error=class-memaccess -Wno-error=sign-compare -Wno-error=narrowing
    -Wno-error=format-truncation -Wno-error=format=
    # Make the Arduino extras (ltoa/micros/random/ps_malloc/ESP) visible to every
    # MeshCore TU — they reach Arduino via the LilyGo <Arduino.h> shim, which does
    # not pull in arduino_compat.h. Guarded by #pragma once + __cplusplus internally.
    -include arduino_compat.h
)
