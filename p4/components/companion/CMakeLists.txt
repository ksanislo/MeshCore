# companion — the shared MeshCore companion BACKEND (MyMesh + DataStore +
# RadioPresetStore + MeshProxy) compiled IN PLACE from examples/ via relative
# paths (no copies -> no drift). Connectivity (WiFi/BLE/MQTT) is compiled OFF for
# this milestone; MESH_PROXY is ON (dual-core snapshot/command/event model). The
# UI (UITask) is NOT built here — that is a later stage.
idf_component_register(
    SRCS
        "../../../examples/companion_radio/MyMesh.cpp"
        "../../../examples/companion_radio/DataStore.cpp"
        "../../../examples/companion_radio/RadioPresetStore.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/MeshProxy.cpp"
        "p4_target.cpp"
    INCLUDE_DIRS
        "."
        "../../../examples/companion_radio"
        "../../../examples/companion_radio_lvgl/ui-lvgl"
    REQUIRES
        meshcore
        fs_shim
        board_hw
        cpp_bus_driver
        nvs_flash
        esp_timer
        esp_system
        driver
        esp_hw_support
)

# Connectivity OFF, dual-core proxy ON. ENV_INCLUDE_GPS pinned to 0 (GPS bring-up
# is a later milestone); the backend's GPS blocks are `#if ENV_INCLUDE_GPS == 1`.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MESH_PROXY=1
    ENV_INCLUDE_GPS=0
    # MAX_CONTACTS / MAX_GROUP_CHANNELS are defined GLOBALLY in the root
    # CMakeLists (they must match the meshcore component's BaseChatMesh).
)

# Same warning relaxations + forced Arduino-extras include the meshcore component
# uses (the vendored MeshCore sources reach Arduino via the LilyGo <Arduino.h>
# shim, which doesn't pull arduino_compat.h). arduino_compat.h is on the include
# path via the `meshcore` REQUIRES above.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=reorder -Wno-error=unused-variable -Wno-error=unused-function
    -Wno-error=class-memaccess -Wno-error=sign-compare -Wno-error=narrowing
    -Wno-error=format-truncation -Wno-error=format=
    -Wno-error=misleading-indentation
    -include arduino_compat.h
)
