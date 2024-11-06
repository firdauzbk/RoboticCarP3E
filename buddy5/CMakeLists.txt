# Create a library for buddy5
add_library(buddy5 buddy5.c buddy5.h)

# Optionally specify include directories
target_include_directories(buddy5 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies
target_link_libraries(buddy5 pico_stdlib hardware_pwm)
