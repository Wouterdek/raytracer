file(GLOB_RECURSE raytracer_lib_SRC
    "*.h"
    "*.cpp"
        )

add_library(raytracer_lib ${raytracer_lib_SRC})

target_include_directories(raytracer_lib PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)

#Eigen3
if(EMSCRIPTEN)
    target_include_directories(raytracer_lib PRIVATE "/home/wouter/vcpkg/packages/eigen3_x86-wasm/include")
else()
    find_package(Eigen3 REQUIRED NO_MODULE)
    target_link_libraries(raytracer_lib PRIVATE Eigen3::Eigen)
endif()

#TBB
if(TRUE)
    target_compile_definitions(raytracer_lib PRIVATE NO_TBB=1)
else()
    find_package(TBB CONFIG REQUIRED)
    target_link_libraries(raytracer_lib PRIVATE TBB::tbb)
endif()

#UNIX stdlib
if(UNIX)
    target_link_libraries(raytracer_lib PRIVATE -lpthread -lm stdc++fs)
endif()