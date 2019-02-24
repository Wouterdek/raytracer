cmake_minimum_required (VERSION 3.8)
project(RAYTRACER)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory (src) 
add_subdirectory (test)

enable_testing()