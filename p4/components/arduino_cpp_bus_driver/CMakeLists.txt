set(
        src_dirs 
        ./src 
    )
    
idf_component_register(
        SRC_DIRS 
        ${src_dirs} 
        INCLUDE_DIRS 
        ${src_dirs} 
        REQUIRES
        "cpp_bus_driver"
    )