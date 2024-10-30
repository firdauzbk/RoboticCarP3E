# Create a library for buddy4
add_library(buddy4 buddy4.c buddy4.h)

# Optionally specify include directories
target_include_directories(buddy4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies and additional i2c hardware support
target_link_libraries(buddy4 pico_stdlib hardware_i2c)

