cmake_minimum_required(VERSION 3.10.2)

# Add the sub-directories for CMake to scan
add_subdirectory(lib)

### Make the executable ###
add_executable(synchronized_txrx_to_file_multiusrp main.cpp)
message(STATUS "Linking synchronized_txrx_to_file_multiusrp.")
target_link_libraries(synchronized_txrx_to_file_multiusrp PRIVATE 
    UHD_BOOST
    cogrf_lib)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/4dev_txrx.cfg 
                ${CMAKE_CURRENT_BINARY_DIR}/)

