
file(GLOB_RECURSE tests_SRC
    "*.h"
    "*.cpp"
)

add_executable(tests ${tests_SRC})

find_package(GTest MODULE REQUIRED)
include(GoogleTest)
target_link_libraries (tests PRIVATE GTest::GTest GTest::Main raytracer_lib)

gtest_add_tests(TARGET tests tests_SRC)
#gtest_discover_tests(tests)