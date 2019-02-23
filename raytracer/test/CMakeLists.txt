
file(GLOB_RECURSE tests_SRC
    "*.h"
    "*.cpp"
)

add_executable(tests ${tests_SRC})

add_test(tests raytracer)

target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries (tests raytracer_lib)