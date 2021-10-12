#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.10.2)

project(COGRF VERSION 1.0
        DESCRIPTION "Cognitive RF Host Libaries and Examples"
        LANGUAGES CXX)
enable_testing()

########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")

########################################################################
# Find Packages and add to the project as a dependency
########################################################################

find_package(UHD 4.0.0 REQUIRED)

# Pull in all boost dependencies in one place.
find_package(Boost 1.50 REQUIRED COMPONENTS
    chrono
    date_time
    filesystem
    program_options
    system
    serialization
    thread
    unit_test_framework
)
# Interface library will automatically add UHD and Boost
# include directories with "target_link_libraries"
add_library(UHD_BOOST INTERFACE)
target_include_directories(UHD_BOOST INTERFACE ${UHD_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_link_libraries(UHD_BOOST INTERFACE ${UHD_LIBRARIES} ${Boost_LIBRARIES})


########################################################################
# Add the Libraries
########################################################################

add_subdirectory(lib)

########################################################################
# Make the executable
########################################################################

add_executable(rfnoc_txrx_loopback examples/rfnoc_txrx_loopback.cpp)
message(STATUS "Linking rfnoc_txrx_loopback.")
target_link_libraries(rfnoc_txrx_loopback PRIVATE UHD_BOOST cogrf_lib)

add_executable(iterative_loopback_singlethread examples/iterative_loopback_singlethread.cpp)
message(STATUS "Linking iterative_loopback_singlethread.")
target_link_libraries(iterative_loopback_singlethread PRIVATE UHD_BOOST cogrf_lib)

add_executable(multifreq_loopback examples/multifreq_loopback.cpp)
message(STATUS "Linking multifreq_loopback.")
target_link_libraries(multifreq_loopback PRIVATE UHD_BOOST cogrf_lib)

add_executable(rfnoc_txrx_loopback_mem examples/rfnoc_txrx_loopback_mem.cpp)
message(STATUS "Linking rfnoc_txrx_loopback_mem.")
target_link_libraries(rfnoc_txrx_loopback_mem PRIVATE UHD_BOOST cogrf_lib)


########################################################################
# Post Build Include Configuration files
########################################################################

#TODO: Make this more robust and not error out the cmake if the file is missing.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/250e6_a1_500khz_250e6tx_16000_0701_2.dat 
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)


