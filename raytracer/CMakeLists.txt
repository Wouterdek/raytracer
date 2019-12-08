cmake_minimum_required (VERSION 3.10)
project(RAYTRACER)

set(CMAKE_CXX_STANDARD 17)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
  # Update submodules as needed
  option(GIT_SUBMODULE "Check submodules during build" ON)
  if(GIT_SUBMODULE)
    message(STATUS "Submodule update")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

if(WIN32)
	set (CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /fp:fast")
	set (CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} /GS- /fp:fast")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

if(UNIX)
  set (CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -ftree-vectorize -march=native -mtune=native -mavx -mavx2")
  set (CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -ftree-vectorize -march=native -mtune=native -mavx -mavx2")
endif()

if(EMSCRIPTEN)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s ALLOW_MEMORY_GROWTH=1 -s USE_WEBGL2=1 -s USE_PTHREADS=1 -s WASM_MEM_MAX=2GB -s OFFSCREEN_FRAMEBUFFER=1")
endif()

add_subdirectory (src)

#if(DEFINED ENV{ENABLE_TESTS})
  add_subdirectory (test)
#endif()
