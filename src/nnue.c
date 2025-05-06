#include "nnue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int32_t crelu(int16_t x) {
    if (x < 0) return 0;
    if (x > QA) return QA;
    return x;
}

void load_network_from_bytes(Network *net, const uint8_t* data, size_t len) {
    if (!data) {
        abort();
    }

    if (len != sizeof(Network)) {
        fprintf(stderr, "Error: NNUE size mismatch\n");
        abort();
    }

    if (((uintptr_t)net % 64) != 0) {
        fprintf(stderr, "Error: Alignment mismatch\n");
        abort();
    }

    memcpy(net, data, sizeof(Network));
}

int32_t network_evaluate(const Network* net, const Accumulator* us, const Accumulator* them) {
    int32_t output = (int32_t)(net->output_bias);

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(us->vals[i]) * (int32_t)(net->output_weights[HIDDEN_SIZE * 2 + i]);
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(them->vals[i]) * (int32_t)(net->output_weights[HIDDEN_SIZE * 3 + i]);
    }

    if (output > INT32_MAX / SCALE) output = INT32_MAX / SCALE;
    if (output < INT32_MIN / SCALE) output = INT32_MIN / SCALE;

    output *= SCALE;
    output /= QA * QB;

    return output;
}

int32_t network_evaluate_setup(const Network* net, const Accumulator* us, const Accumulator* them) {
    int32_t output = (int32_t)(net->output_bias);

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(us->vals[i]) * (int32_t)(net->output_weights[i]);
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(them->vals[i]) * (int32_t)(net->output_weights[HIDDEN_SIZE + i]);
    }

    if (output > INT32_MAX / SCALE) output = INT32_MAX / SCALE;
    if (output < INT32_MIN / SCALE) output = INT32_MIN / SCALE;

    output *= SCALE;
    output /= QA * QB;

    return output;
}

Accumulator accumulator_new(const Network* net) {
    Accumulator acc;
    memcpy(&acc, &net->feature_bias, sizeof(Accumulator));
    return acc;
}

void accumulator_add_feature(Accumulator* acc, const Network* net, size_t feature_idx) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        acc->vals[i] += net->feature_weights[feature_idx].vals[i];
    }
}

void accumulator_remove_feature(Accumulator* acc, const Network* net, size_t feature_idx) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        acc->vals[i] -= net->feature_weights[feature_idx].vals[i];
    }
}