cmake_minimum_required(VERSION 3.30)
project(ProjetIA C)

set(CMAKE_C_STANDARD 11)

add_library(phoenix SHARED src/phoenix.c src/bitboard.c src/move.c src/move.c src/movegen.c src/position.c)
add_executable(phoenix_test src/main.c src/phoenix.c src/bitboard.c src/move.c src/movegen.c src/position.c)
