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

add_executable(selfplay
        src/selfplay.c
        src/bitboard.c
        src/move.c
        src/movegen.c
        src/position.c
        src/search.c
        src/tt.c
        src/nnue.c
)

if (EMSCRIPTEN)
    string(APPEND EMSCRIPTEN_LINK_FLAGS
            " -sINITIAL_MEMORY=268435456"
            " -sENVIRONMENT=worker"
            " -sUSE_PTHREADS=1"
            " -sPTHREAD_POOL_SIZE=1"
            " -sSTACK_SIZE=1048576"
            " -sEXIT_RUNTIME=0"
            " -sEXPORTED_FUNCTIONS=['_main']"
            " -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8']"
            " -sASYNCIFY=1"
    )

    set_target_properties(clippy PROPERTIES
            OUTPUT_NAME "clippy"
            SUFFIX ".js"
            LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}"
    )

    target_compile_options(clippy PRIVATE
            -msimd128
            -matomics
            -mbulk-memory
            -O3
            -flto
    )
endif()

target_link_libraries(clippy m)