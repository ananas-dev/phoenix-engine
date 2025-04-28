cmake_minimum_required(VERSION 3.30)
project(Clippy C)

set(CMAKE_C_STANDARD 11)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE
            "-O3 -march=native -mtune=native -mavx -mfma -flto -funroll-loops -fno-omit-frame-pointer -DNDEBUG"
    )
endif()

add_compile_options(
        -Wall          # Enable most warnings
        -Wextra        # Enable extra warnings
)

if (NOT EMSCRIPTEN)
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
            src/nnue.c
    )

    add_library(clippy_training_data_loader SHARED
            src/position.c
            src/batch.c
    )

    target_link_libraries(clippy m)

    add_executable(clippy_filter
            src/bitboard.c
            src/move.c
            src/movegen.c
            src/filter.c
            src/position.c
            src/eval.c
    )

    add_executable(perft_test test/perft.c)
    target_link_libraries(perft_test unity clippy)

    add_test(NAME Perft COMMAND perft_test)
    enable_testing()
endif()

if (EMSCRIPTEN)
    add_executable(clippy
            src/clippy.c
            src/bitboard.c
            src/move.c
            src/movegen.c
            src/position.c
            src/search.c
            src/tt.c
            src/eval.c
            src/nnue.c
    )

    target_link_libraries(clippy m)

    set_target_properties(clippy PROPERTIES
            SUFFIX ".js"
            LINK_FLAGS "-s WASM=1 \
               -s EXPORTED_FUNCTIONS='[\"_main\", \"_init_context\", \"_act\"]' \
               -s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\"]' \
               -s MODULARIZE=1 \
               -s EXPORT_ES6=1 \
               -s EXPORT_NAME='ClippyEngine'"
    )
endif()