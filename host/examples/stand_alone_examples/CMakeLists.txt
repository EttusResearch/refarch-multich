cmake_minimum_required(VERSION 3.10.2)

### Make the executable ###
add_executable(init_usrp init_usrp.cpp)
message(STATUS "Linking init_usrp.")
target_link_libraries(init_usrp PRIVATE UHD_BOOST)
