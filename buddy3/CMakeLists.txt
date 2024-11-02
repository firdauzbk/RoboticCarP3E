# Create a library for buddy3
add_library(buddy3 buddy3.c buddy3.h)

# Optionally specify include directories
target_include_directories(buddy3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies
target_link_libraries(buddy3 pico_stdlib hardware_adc buddy2)