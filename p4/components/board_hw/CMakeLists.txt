# board_hw — header-only board pin/address map for the LilyGo T-Display P4.
# Owns t_display_p4_config.h so BOTH main/ (which constructs the hardware globals)
# and the meshcore component (radio adapter) can include it. The Pin:: enum
# references pull in cpp_bus_driver, so REQUIRE it.
idf_component_register(
    INCLUDE_DIRS "."
    REQUIRES cpp_bus_driver
)
