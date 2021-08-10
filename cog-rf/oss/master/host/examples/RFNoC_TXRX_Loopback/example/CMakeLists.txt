cmake_minimum_required(VERSION 3.10.2)


### Make the executable ###
add_executable(rfnoc_txrx_loopback rfnoc_txrx_loopback.cpp)
message(STATUS "Linking rfnoc_txrx_loopback.")
target_link_libraries(rfnoc_txrx_loopback PRIVATE UHD_BOOST cogrf_lib)

### Make the executable ###
add_executable(spsc spsctest.cpp)
message(STATUS "Linking spsc.")
target_link_libraries(spsc PRIVATE UHD_BOOST cogrf_lib)




configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_txrx_loopback.cfg 
                ${CMAKE_CURRENT_BINARY_DIR}/)





