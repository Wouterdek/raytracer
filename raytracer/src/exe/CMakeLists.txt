file(GLOB_RECURSE raytracer_SRC
    "*.h"
    "*.cpp"
)

add_executable(raytracer_exe ${raytracer_SRC})

find_package(Eigen3 REQUIRED NO_MODULE)
#find_package(Boost 1.69 REQUIRED COMPONENTS fiber)
find_package(TBB CONFIG REQUIRED)

target_include_directories(raytracer_exe PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)
target_link_libraries(raytracer_exe PRIVATE Eigen3::Eigen raytracer_lib TBB::tbb)
