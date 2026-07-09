# Arduino <FS.h> (fs::FS / fs::File) shim over ESP-IDF VFS + POSIX stdio.
# Requires meshcore for its Stream.h (File's base class) and arduino_cpp_bus_driver
# for Arduino.h that Stream.h pulls in. meshcore <-> fs_shim is a mutual (circular)
# component dependency, which ESP-IDF resolves via a link group.
idf_component_register(
    SRCS "fs_shim.cpp"
    INCLUDE_DIRS "."
    REQUIRES spiffs vfs esp_partition meshcore arduino_cpp_bus_driver
)
