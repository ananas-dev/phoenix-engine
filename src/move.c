#include "move.h"
#include <stdio.h>
#include <string.h>

char from[3];
char to[3];

void move_print(Move move) {
    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("%s%s", from, to);
}

char* move_to_uci(Move move) {
    static char uci[150];

    char files[] = "abcdefgh";
    char ranks[] = "12345678";

    sprintf(uci, "%c%c%c%c",
            files[sq_file(move.from)], ranks[sq_rank(move.from)],
            files[sq_file(move.to)], ranks[sq_rank(move.to)]);


    if (!bb_is_empty(move.captures)) {
        char* ptr = uci + strlen(uci);
        *ptr++ = 'x';

        Bitboard captures_iter = move.captures & BB_USED;
        while (captures_iter) {
            Square square = bb_it_next(&captures_iter);

            *ptr++ = files[sq_file(square)];
            *ptr++ = ranks[sq_rank(square)];

            if (captures_iter) {
                *ptr++ = ',';
            }
        }

        *ptr = '\0';
    }

    return uci;
}

Move uci_to_move(const char* uci) {
    Move move = {0};

    if (strlen(uci) < 4) {
        return move;
    }

    int from_file = uci[0] - 'a';
    int from_rank = uci[1] - '1';
    move.from = sq_get(from_file, from_rank);

    int to_file = uci[2] - 'a';
    int to_rank = uci[3] - '1';
    move.to = sq_get(to_file, to_rank);

    const char* captures_start = strchr(uci + 4, 'x');
    if (captures_start) {
        // Skip the 'x'
        captures_start++;

        const char* ptr = captures_start;

        while (*ptr && *(ptr+1)) {
            char cap_file = *ptr++;
            char cap_rank = *ptr++;

            if (cap_file >= 'a' && cap_file <= 'h' &&
                cap_rank >= '1' && cap_rank <= '7') {
                int file_idx = cap_file - 'a';
                int rank_idx = cap_rank - '1';
                Square square = sq_get(file_idx, rank_idx);

                move.captures |= bb_from_sq(square);
            }

            if (*ptr == ',') ptr++;
        }
    }

    return move;
}