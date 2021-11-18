#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

### Make the library ###
add_library(Arch_lib STATIC 
    RefArch.hpp
    RefArch.cpp
    )
target_link_libraries(Arch_lib PRIVATE UHD_BOOST)

if(Filesystem_FOUND)
    target_link_libraries(Arch_lib PRIVATE std::filesystem)
endif()

# The following line is will automatically add the correct 
# include directories with "target_link_libraries"
target_include_directories(Arch_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
