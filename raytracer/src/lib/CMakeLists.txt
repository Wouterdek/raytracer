file(GLOB_RECURSE raytracer_lib_SRC
    "*.h"
    "*.cpp"
        )

add_library(raytracer_lib ${raytracer_lib_SRC})

target_include_directories(raytracer_lib PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)
if(EMSCRIPTEN)
else()
    target_compile_definitions(raytracer_lib PRIVATE ENABLE_SIMD ENABLE_L2_BVH_PACK)
endif()

#Eigen3
if(EMSCRIPTEN)
    if(DEFINED ENV{EIGEN3_INCLUDE_DIR})
        target_include_directories(raytracer_lib PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
    else()
        message("Missing Eigen3, please set EIGEN3_INCLUDE_DIR")
    endif()
else()
    find_package(Eigen3 REQUIRED NO_MODULE)
    target_link_libraries(raytracer_lib PRIVATE Eigen3::Eigen)
endif()

#TBB
if(EMSCRIPTEN)
    target_compile_definitions(raytracer_lib PRIVATE NO_TBB=1)
else()
    find_package(TBB CONFIG REQUIRED)
    target_link_libraries(raytracer_lib PRIVATE TBB::tbb)
endif()

#UNIX stdlib
if(EMSCRIPTEN)
    target_link_libraries(raytracer_lib PRIVATE -lpthread -lm)
elseif(UNIX)
    target_link_libraries(raytracer_lib PRIVATE -lpthread -lm stdc++fs)
endif()

#PluckerTree
target_include_directories(raytracer_lib PRIVATE ${PLUCKERTREE_SOURCE_DIR}/include)
target_link_libraries (raytracer_lib PRIVATE pluckertree)