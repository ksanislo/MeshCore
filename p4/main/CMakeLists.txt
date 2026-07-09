idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES meshcore board_hw cpp_bus_driver driver esp_hw_support fs_shim
)

# main.cpp includes MeshCore headers (Dispatcher.h) that trip these warnings;
# the meshcore component suppresses the same set on its own sources.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=reorder -Wno-error=unused-variable -Wno-error=unused-function
    -Wno-error=class-memaccess -Wno-error=sign-compare -Wno-error=narrowing
    -Wno-error=format-truncation -Wno-error=format=
)
