cmake_minimum_required(VERSION 3.13)

project(MyProject)

# Add subdirectories
add_subdirectory(buddy1)
add_subdirectory(buddy2)
add_subdirectory(buddy3)
add_subdirectory(buddy4)
add_subdirectory(buddy5)

# Add executable
add_executable(project main.c)

# Link libraries
target_link_libraries(project pico_stdlib buddy1 buddy2 buddy3 buddy4 buddy5)

# Enable USB output, disable UART output
pico_enable_stdio_usb(project 1)
pico_enable_stdio_uart(project 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(project)
