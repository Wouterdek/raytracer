file(GLOB_RECURSE raytracer_SRC
    "*.h"
    "*.cpp"
)

add_executable(raytracer_exe ${raytracer_SRC})

target_include_directories(raytracer_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_include_directories(raytracer_exe PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)

target_link_libraries (raytracer_exe raytracer_lib)