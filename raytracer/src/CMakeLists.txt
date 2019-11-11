add_subdirectory (lib)

if(EMSCRIPTEN)
    add_subdirectory (web)
else()
    add_subdirectory (exe)
endif()