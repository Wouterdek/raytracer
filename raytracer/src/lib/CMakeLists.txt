file(GLOB_RECURSE raytracer_lib_SRC
    "*.h"
    "*.cpp"
)

add_library(raytracer_lib ${raytracer_lib_SRC})

target_include_directories(raytracer_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
