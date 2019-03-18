file(GLOB_RECURSE raytracer_lib_SRC
    "*.h"
    "*.cpp"
)

add_library(raytracer_lib ${raytracer_lib_SRC})

find_package(Eigen3 REQUIRED NO_MODULE)
#find_package(Boost 1.69 REQUIRED COMPONENTS fiber)
find_package(TBB CONFIG REQUIRED)

target_include_directories(raytracer_lib PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)
target_link_libraries(raytracer_lib PRIVATE Eigen3::Eigen TBB::tbb)