# companion — the shared MeshCore companion BACKEND (MyMesh + DataStore +
# RadioPresetStore + MeshProxy) AND the shared LVGL UI (UITask + asset TUs),
# compiled IN PLACE from examples/ via relative paths (no copies -> no drift).
# Connectivity (WiFi/BLE/MQTT) is compiled OFF for this milestone; MESH_PROXY is
# ON (dual-core snapshot/command/event model). The UI uses the P4 esp_lcd display
# backend (UI_DISPLAY_ESP_LCD seam) instead of LovyanGFX; SD / map / emoji-pack are
# deferred and satisfied by the p4_ui_stubs.cpp no-op TU.
idf_component_register(
    SRCS
        "../../../examples/companion_radio/MyMesh.cpp"
        "../../../examples/companion_radio/DataStore.cpp"
        "../../../examples/companion_radio/RadioPresetStore.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/MeshProxy.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/UITask.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/meshcore_emoji_14.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/meshcore_icons_16.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/meshcore_logo_alpha.cpp"
        "../../../examples/companion_radio_lvgl/ui-lvgl/meshcore_logo_img.cpp"
        "p4_target.cpp"
        "p4_ui_stubs.cpp"
        "p4_ui_smoke.cpp"
    INCLUDE_DIRS
        "."
        "../../../examples/companion_radio"
        "../../../examples/companion_radio_lvgl/ui-lvgl"
    REQUIRES
        meshcore
        fs_shim
        board_hw
        cpp_bus_driver
        p4_display
        lvgl
        nvs_flash
        esp_timer
        esp_system
        driver
        esp_hw_support
        espcoredump
)

# Connectivity OFF, dual-core proxy ON. ENV_INCLUDE_GPS pinned to 0 (GPS bring-up
# is a later milestone); the backend's GPS blocks are `#if ENV_INCLUDE_GPS == 1`.
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MESH_PROXY=1
    ENV_INCLUDE_GPS=0
    # Display seam: the UI drives our esp_lcd backend (p4_display) instead of
    # LovyanGFX. Gates every _lgfx / LGFX touch point in UITask.{h,cpp}.
    UI_DISPLAY_ESP_LCD=1
    # Radio TX-power ceiling for the UI's radio-settings validation (SX1262).
    LORA_TX_POWER=22
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
