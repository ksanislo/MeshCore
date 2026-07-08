# MeshCore mesh core + ed25519 crypto, vendored from our src/ (== upstream fork). Phase 1: get the
# protocol core + crypto compiling under IDF 5.4.1 with the arduino_compat shim. Radio/store/app next.
file(GLOB CORE_SRCS "meshcore_src/*.cpp")
list(FILTER CORE_SRCS EXCLUDE REGEX ".*IdentityStore\\.cpp$")   # Arduino-FS dependent
file(GLOB ED25519_SRCS "meshcore_src/ed25519/*.c")

idf_component_register(
    SRCS
        ${CORE_SRCS}
        ${ED25519_SRCS}
        "arduino_compat.cpp"
    INCLUDE_DIRS
        "."
        "meshcore_src"
        "meshcore_src/helpers"
        "meshcore_src/ed25519"
    REQUIRES
        arduino_cpp_bus_driver
        cpp_bus_driver
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
)
