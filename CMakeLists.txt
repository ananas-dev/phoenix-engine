cmake_minimum_required(VERSION 3.30)
project(ProjetIA C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(vendor/Unity-2.6.1)

add_library(phoenix SHARED
        src/phoenix.c
        src/bitboard.c
        src/move.c
        src/movegen.c
        src/position.c
        src/search.c
        src/tt.c
        src/eval.c
)

add_executable(phoenix_test src/main.c)
target_link_libraries(phoenix_test phoenix)

add_executable(perft_test test/perft.c)
target_link_libraries(perft_test unity phoenix)

add_test(NAME Perft COMMAND perft_test)
enable_testing()