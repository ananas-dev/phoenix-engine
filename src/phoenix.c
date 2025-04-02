#include "bitboard.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "movegen.h"
#include "position.h"
#include "time.h"

void init() {
    movegen_init();
    srand(time(NULL));
}

void test() {
    Position pos = position_from_fen("SS1G1G2/SSS1G2s/SSSS2ss/SSS3gs/SS2s1gs/S2sssss/2ss1gss 6 w 00");

    position_print(&pos);

    MoveList move_list = {0};
    legal_moves(&pos, &move_list);

    for (int i = 0; i < move_list.size; i++) {
        char from[3];
        char to[3];

        sq_to_string(move_list.moves[i].from, from);
        sq_to_string(move_list.moves[i].to, to);

        printf("Move:\n");
        printf("  from: %s\n", from);
        printf("  to: %s\n", to);
        printf("  captures:\n");
        bb_print(move_list.moves[i].captured);
    }
}



Move act(char *position, double time_remaining) {
    Position pos = position_from_fen(position);

    position_print(&pos);

    MoveList move_list = {0};
    legal_moves(&pos, &move_list);

    assert((move_list.size > 0) && "Empty move list !");

    return move_list.moves[rand() % move_list.size];
}
