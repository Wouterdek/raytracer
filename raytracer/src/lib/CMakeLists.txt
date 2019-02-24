file(GLOB_RECURSE raytracer_lib_SRC
    "*.h"
    "*.cpp"
)

add_library(raytracer_lib ${raytracer_lib_SRC})

find_package(Eigen3 REQUIRED NO_MODULE)

target_link_libraries(raytracer_lib PRIVATE Eigen3::Eigen)