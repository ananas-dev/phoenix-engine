#include "nnue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline int32_t crelu(int16_t x) {
    int32_t xi = (int32_t)x;
    if (xi < 0) return 0;
    if (xi > QA) return QA;
    return xi;
}

// Cross-platform aligned allocation
void* platform_aligned_alloc(size_t alignment, size_t size) {
    void* ptr = NULL;
#if defined(_WIN32) || defined(_WIN64)
    // Windows implementation
    ptr = _aligned_malloc(size, alignment);
#else
    // C11 implementation
    ptr = aligned_alloc(alignment, size);
#endif
    return ptr;
}

// And similarly for freeing:
void platform_aligned_free(void* ptr) {
#if defined(_WIN32) || defined(_WIN64)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

void load_network_from_bytes(Context *ctx, const uint8_t* data, size_t len) {
    if (!ctx) {
        abort();
    }

    if (!data) {
        abort();
    }

    if (len != sizeof(Network)) {
        fprintf(stderr, "NNUE size mismatch\n");
        abort();
    }

    Network* net = platform_aligned_alloc(64, sizeof(Network));

    if (!net) {
        abort();
    }

    memcpy(net, data, sizeof(Network));
    ctx->net = net;
}

int32_t network_evaluate(const Network* net, const Accumulator* us, const Accumulator* them) {
    int32_t output = (int32_t)(net->output_bias);

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(us->vals[i]) * (int32_t)(net->output_weights[i]);
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        output += crelu(them->vals[i]) * (int32_t)(net->output_weights[HIDDEN_SIZE + i]);
    }

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