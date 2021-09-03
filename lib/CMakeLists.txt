### Make the library ###
add_library(cogrf_lib STATIC 
    graphassembly.cpp 
    graphassembly.hpp
    structures.hpp
    sync.hpp
    sync.cpp
    blocksettings.hpp
    blocksettings.cpp
    replaycontrol.hpp
    replaycontrol.cpp
    receivefunctions.hpp
    receivefunctions.cpp
    recvtofile.hpp
    
    
    
    )
target_link_libraries(cogrf_lib PRIVATE UHD_BOOST)

# The following line is will automatically add the correct 
# include directories with "target_link_libraries"
target_include_directories(cogrf_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



