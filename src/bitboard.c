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
    x = x - ((x >> 1)  & 0x5555555555555555ULL); /* put count of each 2 bits into those 2 bits */
    x = (x & 0x3333333333333333ULL) + ((x >> 2)  & 0x3333333333333333ULL); /* put count of each 4 bits into those 4 bits */
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL; /* put count of each 8 bits into those 8 bits */
    x = (x * 0x0101010101010101ULL) >> 56; /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
    return (int)x;
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