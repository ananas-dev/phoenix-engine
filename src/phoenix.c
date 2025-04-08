#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"
#include "tt.h"

void init() {
    movegen_init();
    position_init();
    eval_init();
    tt_init(65536); // 2 ^ 16
    srand(time(NULL));
}

void test() {
    Position pos = position_from_fen("1S2K3/GSGSS2s/SSSS2s1/SSS2ssg/SS2sss1/S3gssg/2ssss1g 8 w 00");

    position_print(&pos);

    Move move = search(&pos, 10.0);

    char from[3];
    char to[3];

    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("Move:\n");
    printf("  from: %s\n", from);
    printf("  to: %s\n", to);
    printf("  captures:\n");
    bb_print(move.captures);
}

Move act(char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    double allocated_time = 10.0;

    // Basic time management
    if (time_remaining < 100.0) {
        allocated_time = 5.0;
    } if (time_remaining < 30.0) {
        allocated_time = 3.0;
    } else if (time_remaining < 10.0) {
        allocated_time = 1.0;
    } else if (time_remaining < 3.0) {
        allocated_time = 0.1;
    }

    return search(&pos, allocated_time);
}
