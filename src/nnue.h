#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "core.h"

#define HIDDEN_SIZE 128
#define SCALE 400
#define QA 255
#define QB 64

struct Context;

typedef struct __attribute__((aligned(64))) {
    int16_t vals[HIDDEN_SIZE];
} Accumulator;

typedef struct __attribute__((aligned(64))) {
    Accumulator feature_weights[336];
    Accumulator feature_bias;
    int16_t output_weights[4 * HIDDEN_SIZE];
    int16_t output_bias;
} Network;

static inline int get_feature_index_white(Color color, Piece piece, Square square) {
    return NUM_PIECE * NUM_SQUARE * color + (piece * NUM_SQUARE) + square;
}

static inline int get_feature_index_black(Color color, Piece piece, Square square) {
    Square mirrored = sq_mirror(square);

    return get_feature_index_white(1-color, piece, mirrored);
}

void load_network_from_bytes(Network* net, const uint8_t* data, size_t len);
int32_t network_evaluate(const Network* net, const Accumulator* us, const Accumulator* them);
int32_t network_evaluate_setup(const Network* net, const Accumulator* us, const Accumulator* them);
Accumulator accumulator_new(const Network* net);
void accumulator_add_feature(Accumulator* acc, const Network* net, size_t feature_idx);
void accumulator_remove_feature(Accumulator* acc, const Network* net, size_t feature_idx);