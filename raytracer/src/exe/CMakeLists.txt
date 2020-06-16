file(GLOB_RECURSE raytracer_SRC
    "*.h"
    "*.cpp"
	"*.c"
)

add_executable(raytracer_exe ${raytracer_SRC})

if(WIN32 AND PROFILE)
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
endif()

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Boost 1.65 REQUIRED COMPONENTS program_options)
find_package(TBB CONFIG REQUIRED)
find_package(glfw3 3.2 REQUIRED)

target_include_directories(raytracer_exe PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib ${RAYTRACER_SOURCE_DIR}/src/exe/preview/glad/include)
target_link_libraries(raytracer_exe PRIVATE Eigen3::Eigen raytracer_lib Boost::program_options TBB::tbb glfw dl)
#target_link_libraries(raytracer_exe PRIVATE Eigen3::Eigen raytracer_lib Boost::program_options TBB::tbb dl)

#PluckerTree
target_include_directories(raytracer_exe PRIVATE ${PLUCKERTREE_SOURCE_DIR}/include)
target_link_libraries (raytracer_exe PRIVATE pluckertree)