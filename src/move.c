#include "move.h"
#include <stdio.h>

char from[3];
char to[3];

void move_print(Move move) {
    sq_to_string(move.from, from);
    sq_to_string(move.to, to);

    printf("%s%s", from, to);
}
