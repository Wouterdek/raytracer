if(EMSCRIPTEN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
endif()

file(GLOB_RECURSE raytracer_web_SRC
    "*.h"
    "*.cpp"
	"*.c"
)

set(CMAKE_EXECUTABLE_SUFFIX ".html")
add_executable(raytracer_web ${raytracer_web_SRC})
target_include_directories(raytracer_web PRIVATE ${RAYTRACER_SOURCE_DIR}/src/lib)
target_link_libraries(raytracer_web PRIVATE raytracer_lib dl)

#Eigen3
if(EMSCRIPTEN)
	if(DEFINED ENV{EIGEN3_INCLUDE_DIR})
		target_include_directories(raytracer_web PRIVATE $ENV{EIGEN3_INCLUDE_DIR})
	else()
		message("Missing Eigen3, please set EIGEN3_INCLUDE_DIR")
	endif()
else()
	find_package(Eigen3 REQUIRED NO_MODULE)
	target_link_libraries(raytracer_web PRIVATE Eigen3::Eigen)
endif()

#TBB
if(EMSCRIPTEN)
	target_compile_definitions(raytracer_web PRIVATE NO_TBB=1)
else()
	find_package(TBB CONFIG REQUIRED)
	target_link_libraries(raytracer_web PRIVATE TBB::tbb)
endif()
