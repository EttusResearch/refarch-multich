cmake_minimum_required(VERSION 3.10.2)

### Make the library ###
add_library(cogrf_lib STATIC 
    applicationSettings.cpp
    applicationSettings.hpp
    checkClocks.cpp
    checkClocks.hpp
    config.hpp
    createSession.cpp
    createSession.hpp
    detectChannels.cpp
    detectChannels.hpp
    playbackRecord.cpp
    playbackRecord.hpp
    programOptions.cpp
    programOptions.hpp
    usrpSettings.cpp
    usrpSettings.hpp
    waveTable.hpp)
target_link_libraries(cogrf_lib PRIVATE UHD_BOOST)

# The following line is will automatically add the correct 
# include directories with "target_link_libraries"
target_include_directories(cogrf_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
