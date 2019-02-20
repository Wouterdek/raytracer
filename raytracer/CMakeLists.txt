cmake_minimum_required (VERSION 3.8)
project(raytracer)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE raytracer_SRC
    "*.h"
    "*.cpp"
)

add_executable(raytracer ${raytracer_SRC})

target_include_directories(raytracer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)