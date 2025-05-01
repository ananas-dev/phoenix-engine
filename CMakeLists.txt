cmake_minimum_required(VERSION 3.30)
project(Clippy C)

set(CMAKE_C_STANDARD 11)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE
            "-O3 -march=native -mtune=native -mavx -mfma -flto -funroll-loops -DNDEBUG"
    )
endif()

add_compile_options(
        -Wall          # Enable most warnings
        -Wextra        # Enable extra warnings
        -pthread
)

add_executable(clippy
        src/main.c
        src/bitboard.c
        src/move.c
        src/movegen.c
        src/position.c
        src/search.c
        src/tt.c
        src/nnue.c
        src/ugi.c
)

if (EMSCRIPTEN)
    set_target_properties(clippy PROPERTIES
            OUTPUT_NAME "clippy"
            SUFFIX ".js"
    )

    set_target_properties(clippy PROPERTIES
            LINK_FLAGS "-sINITIAL_MEMORY=268435456 -sENVIRONMENT=web,worker -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=1 -sSTACK_SIZE=1048576 -sEXIT_RUNTIME=0 -sEXPORTED_FUNCTIONS=['_main'] -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8'] -s ASYNCIFY=1 -msimd128 -matomics"
    )
endif()

target_link_libraries(clippy m)