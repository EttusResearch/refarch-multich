cmake_minimum_required(VERSION 3.10.2)

# Add the sub-directories for CMake to scan
add_subdirectory(tx_rx_to_file_multiusrp)
add_subdirectory(python)
add_subdirectory(stand_alone_examples)

## Make the executable ###
add_executable(dat_to_buffer dat_to_buffer.cpp)
message(STATUS "Linking dat_to_buffer")
target_link_libraries(dat_to_buffer PRIVATE UHD_BOOST)

