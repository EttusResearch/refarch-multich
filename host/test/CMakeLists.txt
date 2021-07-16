cmake_minimum_required(VERSION 3.10.2)

### copy pytests to build folder ###
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_sample.py 
                ${CMAKE_CURRENT_BINARY_DIR}/)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_sysexit.py 
                ${CMAKE_CURRENT_BINARY_DIR}/)

### Make the testcfgfile executable ###
add_executable(testCfgFile testCfgFile.cpp)
message(STATUS "Linking testCfgFile.")
target_link_libraries(testCfgFile PRIVATE 
    UHD_BOOST
    cogrf_lib)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testCfgFile.cfg 
                ${CMAKE_CURRENT_BINARY_DIR}/)