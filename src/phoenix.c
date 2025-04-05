#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "movegen.h"
#include "position.h"
#include "time.h"
#include "search.h"
#include "tt.h"

void init() {
    movegen_init();
    position_init();
    tt_init(65536); // 2 ^ 16
    srand(time(NULL));
}

void test() {
    Position pos = position_from_fen("1SSSSS2/G1SSS3/1KSS3g/G1S3k1/G4ssg/3sgsss/2Ssssss 13 b 00");

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


// void test() {
//     // Position pos = position_from_fen("SSSSSS2/SSSGS2s/SSS3ss/SSS2sss/SS2ssss/S2ssssg/2sssss1 2 w 00");
//     Position pos = position_from_fen("G2SSS2/1SKSS2s/GSS4s/1SSS2k1/GS2s1gg/3sssg1/2ssssss 13 b 00");
//
//     position_print(&pos);
//
//     uint64_t num_nodes = perft(&pos, 3);
//     printf("Perft result %lu:\n", num_nodes);
// }



Move act(char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    Move move = search(&pos, 10.0);

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
