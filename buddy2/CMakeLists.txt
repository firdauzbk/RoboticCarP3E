# Create a library for buddy2
add_library(buddy2 buddy2.c buddy2.h)

# Optionally specify include directories
target_include_directories(buddy2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies and additional pwm hardware support
target_link_libraries(buddy2 pico_stdlib hardware_pwm)