cmake_minimum_required (VERSION 3.8)
project(RAYTRACER)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_subdirectory (src) 
add_subdirectory (test)
enable_testing()