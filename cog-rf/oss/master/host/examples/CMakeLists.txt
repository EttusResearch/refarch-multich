cmake_minimum_required(VERSION 3.10.2)

add_subdirectory(python)

### Make the executable ###
add_executable(init_usrp init_usrp.cpp)
message(STATUS "Linking init_usrp.")
target_link_libraries(init_usrp PRIVATE UHD_BOOST)

### Make the executable ###
add_executable(synchronizedPlaybackRecord synchronizedPlaybackRecord.cpp)
message(STATUS "Linking synchronizedPlaybackRecord.")
target_link_libraries(synchronizedPlaybackRecord PRIVATE 
    UHD_BOOST
    cogrf_lib)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/synchronizedPlaybackRecord.cfg 
                ${CMAKE_CURRENT_BINARY_DIR}/)

### Make the executable ###
add_executable(rfnoc_rx_to_file rfnoc_rx_to_file.cpp)
message(STATUS "Linking rfnoc_rx_to_file.")
target_link_libraries(rfnoc_rx_to_file PRIVATE UHD_BOOST)


### Make the executable ###
add_executable(rfnoc_radio_loopback rfnoc_radio_loopback.cpp)
message(STATUS "Linking rfnoc_radio_loopback.")
target_link_libraries(rfnoc_radio_loopback PRIVATE UHD_BOOST)

### Make the executable ###
add_executable(rfnoc_rx_multichannel rfnoc_rx_multichannel.cpp)
message(STATUS "Linking rfnoc_rx_multichannel.")
target_link_libraries(rfnoc_rx_multichannel PRIVATE UHD_BOOST)

### Make the executable ###
add_executable(rfnoc_txrx_iterative_loopback rfnoc_txrx_iterative_loopback.cpp)
message(STATUS "Linking rfnoc_txrx_iterative_loopback.")
target_link_libraries(rfnoc_txrx_iterative_loopback PRIVATE UHD_BOOST)
