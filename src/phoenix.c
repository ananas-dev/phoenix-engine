#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"

void init() {
    movegen_init();
    position_init();
    srand(time(NULL));
}

void test() {
    Position pos = position_from_fen("K1GS4/GG1SS3/2SS3S/1SS5/SS6/S2s3g/5gs1 49 b 11");

    position_print(&pos);

    Move move = search(&pos, 5);

    char from[3];
    char to[3];

    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("Move:\n");
    printf("  from: %s\n", from);
    printf("  to: %s\n", to);
    printf("  captures:\n");
    bb_print(move.captured);
}



Move act(char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    Move move = search(&pos, 5);

    char from[3];
    char to[3];

    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("Move:\n");
    printf("  from: %s\n", from);
    printf("  to: %s\n", to);
    // printf("  captures:\n");
    // bb_print(move.captured);

    return move;
}
