
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
target_include_directories(UHD_BOOST INTERFACE ${UHD_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(UHD_BOOST INTERFACE ${UHD_LIBRARIES} ${Boost_LIBRARIES})


########################################################################
# Add the sub-directories
########################################################################
add_subdirectory(src)
add_subdirectory(lib)