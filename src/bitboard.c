#include "bitboard.h"

#include <stdio.h>

bool bb_is_empty(Bitboard bb) {
    return (bb & BB_USED) == BB_EMPTY;
}

Bitboard bb_from_sq(Square sq) {
    return 1ULL << sq;
};

Square bb_it_next(Bitboard* b) {
    Square s = __builtin_ctzll(*b);
    *b &= *b - 1;
    return s;
}

int bb_popcnt(Bitboard x) {
    return __builtin_popcountll(x);
}

void bb_print(Bitboard bb) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (int rank = 6; rank >= 0; --rank) {
        for (int file = 0; file <= 7; file++) {
            if (bb & bb_from_sq(sq_get(file, rank))) {
                printf("| X ");
            } else {
                printf("|   ");
            }
        }
        printf("| %d\n", rank + 1);
        printf("+---+---+---+---+---+---+---+---+\n");
    }
    printf("  a   b   c   d   e   f   g   h\n");
}