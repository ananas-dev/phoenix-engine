#include "bitboard.h"

#include <stdio.h>

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