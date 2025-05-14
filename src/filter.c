#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "context.h"
#include "eval.h"
#include "movegen.h"

int main(int argc, char **argv) {
    Context *ctx = malloc(sizeof(Context));

    movegen_init(ctx);
    eval_init(ctx);

    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");

    if (argc != 3) return 1;

    if (in == NULL || out == NULL) {
        perror("Error opening file");
        return 1;
    }

    // First pass: count number of lines
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), in)) {
        buffer[strcspn(buffer, "\r\n")] = 0;

        char *fen_str = strtok(buffer, ",");
        char *score_str = strtok(NULL, ",");

        if (fen_str == NULL || score_str == NULL) {
            fprintf(stderr, "Malformed line, skipping: %s\n", buffer);
            continue;
        }

        double R_i;
        if (sscanf(score_str, "%lf", &R_i) != 1) {
            fprintf(stderr, "Failed to parse score: %s\n", score_str);
            continue;
        }

        Position pos = position_from_fen_no_hash(fen_str);

        // Skip positions with a missing king because they don't matter
        if (bb_popcnt(pos.pieces[COLOR_WHITE][PIECE_KING]) == 0 || bb_popcnt(pos.pieces[COLOR_BLACK][PIECE_KING]) == 0) {
            continue;
        }

        int score = (pos.side_to_move == COLOR_WHITE ? 1 : -1) * eval(ctx, &pos);

        if (score >= (10000 - 100) || score <= -(10000 - 100)) continue;

        fprintf(out, "%s|%d|%.1f\n", fen_str, score, R_i);
    }

    fclose(in);
    fclose(out);
    free(ctx);
}
