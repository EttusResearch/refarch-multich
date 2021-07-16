cmake_minimum_required(VERSION 3.10.2)

project(COGRF VERSION 1.0
        DESCRIPTION "Cognitive RF Host Libaries and Examples"
        LANGUAGES CXX)

# Configure Compiler #
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")

# Add the sub-directories for CMake to scan
add_subdirectory(examples)
add_subdirectory(test)

# Add UHD as a dependency to this project
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
target_include_directories(UHD_BOOST INTERFACE ${UHD_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(UHD_BOOST INTERFACE ${UHD_LIBRARIES} ${Boost_LIBRARIES})



