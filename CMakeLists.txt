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
)

add_subdirectory(vendor/Unity-2.6.1)

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

target_link_libraries(clippy m)