file(GLOB_RECURSE raytracer_SRC
    "*.h"
    "*.cpp"
)

add_executable(raytracer_exe ${raytracer_SRC})

if(WIN32 AND PROFILE)
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
endif()

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Boost 1.69 REQUIRED COMPONENTS program_options)
find_package(TBB CONFIG REQUIRED)

target_include_directories(raytracer_exe PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)
target_link_libraries(raytracer_exe PRIVATE Eigen3::Eigen raytracer_lib TBB::tbb Boost::program_options)
