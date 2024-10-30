# Create a library for buddy1
add_library(buddy1 buddy1.c buddy1.h)

# Optionally specify include directories
target_include_directories(buddy1 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
