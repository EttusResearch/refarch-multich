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

project(RefArch VERSION 1.0
        DESCRIPTION "Open Architecture for Radar and EW Research Libaries and Examples"
        LANGUAGES CXX)
enable_testing()

# make sure any repo local custome CMake Module scripts are found first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")

########################################################################
# Find Packages and add to the project as a dependency
########################################################################

find_package(UHD 4.1.0 REQUIRED)

# Check if we have std::filesystem
find_package(Filesystem COMPONENTS Final Experimental)

if(Filesystem_FOUND)
    add_definitions(-DHAS_STD_FILESYSTEM=1)
    if(${find_experimental})
        add_definitions(-DHAS_STD_FILESYSTEM_EXPERIMENTAL=1)
    endif()
endif()

set(BOOST_FILESYSTEM "")
if(NOT FILESYSTEM_FOUND)
        set(BOOST_FILESYSTEM "filesystem")
endif()

# Pull in all boost dependencies in one place.
find_package(Boost 1.50 REQUIRED COMPONENTS
    chrono
    date_time
    ${BOOST_FILESYSTEM}
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
add_subdirectory(docs)

########################################################################
# Make the executable
########################################################################

add_executable(Arch_rfnoc_txrx_loopback_mem examples/Arch_rfnoc_txrx_loopback_mem.cpp)
message(STATUS "Linking Arch_rfnoc_txrx_loopback_mem.")
target_link_libraries(Arch_rfnoc_txrx_loopback_mem PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_rfnoc_txrx_loopback examples/Arch_rfnoc_txrx_loopback.cpp)
message(STATUS "Linking Arch_rfnoc_txrx_loopback.")
target_link_libraries(Arch_rfnoc_txrx_loopback PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_iterative_loopback examples/Arch_iterative_loopback.cpp)
message(STATUS "Linking Arch_iterative_loopback.")
target_link_libraries(Arch_iterative_loopback PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_multifreq_loopback examples/Arch_multifreq_loopback.cpp)
message(STATUS "Linking Arch_multifreq_loopback.")
target_link_libraries(Arch_multifreq_loopback PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_rx_to_mem examples/Arch_rx_to_mem.cpp)
message(STATUS "Linking Arch_rx_to_mem.")
target_link_libraries(Arch_rx_to_mem PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_txrx_fullduplex examples/Arch_txrx_fullduplex.cpp)
message(STATUS "Linking Arch_txrx_fullduplex.")
target_link_libraries(Arch_txrx_fullduplex PRIVATE UHD_BOOST Arch_lib)

add_executable(Arch_dynamic_tx examples/Arch_dynamic_tx.cpp)
message(STATUS "Linking Arch_dynamic_tx.")
target_link_libraries(Arch_dynamic_tx PRIVATE UHD_BOOST Arch_lib)

########################################################################
# Post Build Include Configuration files
########################################################################

#TODO: Make this more robust and not error out the cmake if the file is missing.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/250e6_a1_500khz_250e6tx_16000_0701_2.dat 
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)


