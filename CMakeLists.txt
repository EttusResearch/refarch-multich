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
# Add the sub-directories
########################################################################
add_subdirectory(RefDesign)


