cmake_minimum_required(VERSION 3.30)
project(Clippy C)

set(CMAKE_C_STANDARD 11)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE
            "-O3 -march=native -mtune=native -mavx -mfma -flto -funroll-loops -fno-omit-frame-pointer -DNDEBUG"
    )
endif()

add_subdirectory(vendor/Unity-2.6.1)

add_library(clippy SHARED
        src/clippy.c
        src/bitboard.c
        src/move.c
        src/movegen.c
        src/position.c
        src/search.c
        src/tt.c
        src/eval.c
)

add_executable(clippy_test src/main.c)
target_link_libraries(clippy_test clippy)

add_executable(perft_test test/perft.c)
target_link_libraries(perft_test unity clippy)

add_test(NAME Perft COMMAND perft_test)
enable_testing()