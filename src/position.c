#include "position.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct {
    uint64_t pieces[NUM_COLOR][NUM_PIECE][NUM_SQUARE];
    uint64_t side_to_move;
    uint64_t can_create_general;
    uint64_t can_create_king;
} Zobrists;

Zobrists zobrists;

static uint64_t random_u64(void) {
    static uint64_t next = 1;

    next = next * 1103515245 + 12345;
    return next;
}

void position_init() {
    for (Color color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
        for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
            for (Square square = SQ_A1; square <= SQ_H7; square++) {
                zobrists.pieces[color][piece][square] = random_u64();
            }
        }
    }

    zobrists.side_to_move = random_u64();
    zobrists.can_create_general = random_u64();
    zobrists.can_create_king = random_u64();
}

static inline void add_feature_symmetric(Position *pos, Network *net, Color color, Piece piece, Square square) {
    accumulator_add_feature(&pos->accumulators[COLOR_WHITE], net, get_feature_index_white(color, piece, square));
    accumulator_add_feature(&pos->accumulators[COLOR_BLACK], net, get_feature_index_black(color, piece, square));
}

static inline void remove_feature_symmetric(Position *pos, Network *net, Color color, Piece piece, Square square) {
    accumulator_remove_feature(&pos->accumulators[COLOR_WHITE], net, get_feature_index_white(color, piece, square));
    accumulator_remove_feature(&pos->accumulators[COLOR_BLACK], net, get_feature_index_black(color, piece, square));
}

uint64_t position_hash(const Position *position) {
    uint64_t hash = 0ULL;

    for (Color color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
        for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
            Bitboard pieces_it = position->pieces[color][piece] & BB_USED;
            while (pieces_it) {
                Square square = bb_it_next(&pieces_it);
                hash ^= zobrists.pieces[color][piece][square];
            }
        }
    }

    if (position->side_to_move == COLOR_BLACK) {
        hash ^= zobrists.side_to_move;
    }

    if (position->can_create_general) {
        hash ^= zobrists.can_create_general;
    }

    if (position->can_create_king) {
        hash ^= zobrists.can_create_king;
    }

    return hash;
}

void position_accumulators(Position *position, Network *net) {
    position->accumulators[COLOR_WHITE] = accumulator_new(net);
    position->accumulators[COLOR_BLACK] = accumulator_new(net);

    for (Color color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
        for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
            Bitboard pieces_it = position->pieces[color][piece] & BB_USED;
            while (pieces_it) {
                Square square = bb_it_next(&pieces_it);
                add_feature_symmetric(position, net, color, piece, square);
            }
        }
    }
}

Position make_move(Position *pos, Network *net, Move move) {
    Color color = pos->side_to_move;
    Position new_pos = *pos;

    new_pos.accumulators[COLOR_WHITE] = pos->accumulators[COLOR_WHITE];
    new_pos.accumulators[COLOR_BLACK] = pos->accumulators[COLOR_BLACK];

    if (new_pos.can_create_king) {
        new_pos.can_create_king = false;
        new_pos.hash ^= zobrists.can_create_king;
    }
    if (new_pos.can_create_general) {
        new_pos.can_create_general = false;
        new_pos.hash ^= zobrists.can_create_general;
    }

    Bitboard from_mask = bb_from_sq(move.from);
    Bitboard to_mask = bb_from_sq(move.to);
    Bitboard captured_mask = move.captures;

    if (new_pos.pieces[color][PIECE_SOLDIER] & from_mask) {
        // Remove soldier from old square
        new_pos.pieces[color][PIECE_SOLDIER] ^= from_mask;
        new_pos.hash ^= zobrists.pieces[color][PIECE_SOLDIER][move.from];

        remove_feature_symmetric(&new_pos, net, color, PIECE_SOLDIER, move.from);

        // Handle stacking
        if (new_pos.pieces[color][PIECE_SOLDIER] & to_mask) {
            new_pos.pieces[color][PIECE_SOLDIER] ^= to_mask;
            new_pos.pieces[color][PIECE_GENERAL] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_SOLDIER][move.to];
            new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];

            remove_feature_symmetric(&new_pos, net, color, PIECE_SOLDIER, move.to);
            add_feature_symmetric(&new_pos, net, color, PIECE_GENERAL, move.to);
        } else if (new_pos.pieces[color][PIECE_GENERAL] & to_mask) {
            new_pos.pieces[color][PIECE_GENERAL] ^= to_mask;
            new_pos.pieces[color][PIECE_KING] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];
            new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.to];

            remove_feature_symmetric(&new_pos, net, color, PIECE_GENERAL, move.to);
            add_feature_symmetric(&new_pos, net, color, PIECE_KING, move.to);
        } else {
            new_pos.pieces[color][PIECE_SOLDIER] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_SOLDIER][move.to];

            add_feature_symmetric(&new_pos, net, color, PIECE_SOLDIER, move.to);
        }
    } else if (new_pos.pieces[color][PIECE_GENERAL] & from_mask) {
        new_pos.pieces[color][PIECE_GENERAL] ^= from_mask;
        new_pos.pieces[color][PIECE_GENERAL] |= to_mask;
        new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.from];
        new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];

        remove_feature_symmetric(&new_pos, net, color, PIECE_GENERAL, move.from);
        add_feature_symmetric(&new_pos, net, color, PIECE_GENERAL, move.to);
    } else if (new_pos.pieces[color][PIECE_KING] & from_mask) {
        new_pos.pieces[color][PIECE_KING] ^= from_mask;
        new_pos.pieces[color][PIECE_KING] |= to_mask;
        new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.from];
        new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.to];

        remove_feature_symmetric(&new_pos, net, color, PIECE_KING, move.from);
        add_feature_symmetric(&new_pos, net, color, PIECE_KING, move.to);
    } else {
        assert(false && "Impossible move");
    }

    for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
        if (new_pos.pieces[1 - color][piece] & captured_mask) {
            Bitboard captured_it = new_pos.pieces[1 - color][piece] & captured_mask;
            while (captured_it) {
                Square square = bb_it_next(&captured_it);
                new_pos.hash ^= zobrists.pieces[1 - color][piece][square];
                remove_feature_symmetric(&new_pos, net, 1 - color, piece, square);
            }

            new_pos.pieces[1 - color][piece] &= ~captured_mask;

            if (piece == PIECE_GENERAL) {
                if (!new_pos.can_create_general) {
                    new_pos.can_create_general = true;
                    new_pos.hash ^= zobrists.can_create_general;
                }
            } else if (piece == PIECE_KING) {
                if (!new_pos.can_create_king) {
                    new_pos.can_create_king = true;
                    new_pos.hash ^= zobrists.can_create_king;
                }
            }
        }
    }

    new_pos.ply++;
    new_pos.side_to_move = 1 - new_pos.side_to_move;
    new_pos.hash ^= zobrists.side_to_move;

    if (new_pos.ply > 10) {
        if (bb_is_empty(move.captures)) {
            new_pos.half_move_clock++;
        } else {
            new_pos.half_move_clock = 0;
        }
    }

    return new_pos;
}


Position position_from_fen(const char *fen_str, Network *net) {
    Position position = position_from_fen_core(fen_str);
    position.hash = position_hash(&position);
    position_accumulators(&position, net);
    return position;
}

Position position_from_fen_core(const char *fen_str) {
    Position position = {0};

    char fen_copy[256];
    strncpy(fen_copy, fen_str, sizeof(fen_copy) - 1);
    fen_copy[sizeof(fen_copy) - 1] = '\0';

    char *tokens[5];
    int token_count = 0;

    char *ptr = fen_copy;
    while (token_count < 5 && *ptr != '\0') {
        while (*ptr == ' ') ptr++;

        if (*ptr == '\0') break;

        tokens[token_count++] = ptr;

        while (*ptr != ' ' && *ptr != '\0') ptr++;
        if (*ptr != '\0') *ptr++ = '\0';
    }

    if (token_count != 5) {
        fprintf(stderr, "Invalid FEN: expected 5 fields, got %d\n", token_count);
        exit(1);
    }

    const char *board = tokens[0];
    int curr_file = FILE_A;
    int curr_rank = RANK_7;

    for (unsigned int i = 0; i < strlen(board); i++) {
        char x = board[i];

        if (x == '/') {
            curr_file = FILE_A;
            curr_rank--;
        } else if (x >= '1' && x <= '9') {
            curr_file += x - '0';
        } else {
            Color color;
            Piece piece;

            switch (x) {
                case 'S': piece = PIECE_SOLDIER; color = COLOR_WHITE; break;
                case 'G': piece = PIECE_GENERAL; color = COLOR_WHITE; break;
                case 'K': piece = PIECE_KING;    color = COLOR_WHITE; break;
                case 's': piece = PIECE_SOLDIER; color = COLOR_BLACK; break;
                case 'g': piece = PIECE_GENERAL; color = COLOR_BLACK; break;
                case 'k': piece = PIECE_KING;    color = COLOR_BLACK; break;
                default:
                    fprintf(stderr, "Invalid piece char '%c'\n", x);
                    exit(1);
            }

            position.pieces[color][piece] |= bb_from_sq(sq_get(curr_file, curr_rank));
            curr_file++;
        }
    }

    position.ply = atoi(tokens[1]);

    position.half_move_clock = atoi(tokens[2]);

    if (tokens[3][0] == 'w') {
        position.side_to_move = COLOR_WHITE;
    } else if (tokens[3][0] == 'b') {
        position.side_to_move = COLOR_BLACK;
    } else {
        fprintf(stderr, "Invalid side to move: %s\n", tokens[3]);
        exit(1);
    }

    const char *flags = tokens[4];
    if (flags[0] == '1') {
        position.can_create_general = true;
    } else if (flags[0] == '0') {
        position.can_create_general = false;
    } else {
        fprintf(stderr, "Invalid general creation flag: %c\n", flags[0]);
        exit(1);
    }

    if (flags[1] == '1') {
        position.can_create_king = true;
    } else if (flags[1] == '0') {
        position.can_create_king = false;
    } else {
        fprintf(stderr, "Invalid king creation flag: %c\n", flags[1]);
        exit(1);
    }

    return position;
}

void position_print(Position *pos) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (int rank = 6; rank >= 0; --rank) {
        for (int file = 0; file <= 7; file++) {
            Bitboard square_mask = bb_from_sq(sq_get(file, rank));

            Piece current_piece = INVALID_PIECE;
            Color current_color = -1;

            for (Color color = COLOR_WHITE; color <= COLOR_BLACK; color++) {
                for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
                    if (pos->pieces[color][piece] & square_mask) {
                        current_piece = piece;
                        current_color = color;
                        goto end;
                    }
                }
            }
        end:

            if (current_piece == INVALID_PIECE) {
                printf("|   ");
                continue;
            }

            if (current_color == COLOR_WHITE) {
                switch (current_piece) {
                    case PIECE_SOLDIER:
                        printf("| S ");
                        break;
                    case PIECE_GENERAL:
                        printf("| G ");
                        break;
                    case PIECE_KING:
                        printf("| K ");
                        break;
                    default:
                        assert(false && "Invalid piece");
                }
            } else {
                switch (current_piece) {
                    case PIECE_SOLDIER:
                        printf("| s ");
                        break;
                    case PIECE_GENERAL:
                        printf("| g ");
                        break;
                    case PIECE_KING:
                        printf("| k ");
                        break;
                    default:
                        assert(false && "Invalid piece");
                }
            }
        }
        printf("| %d\n", rank + 1);
        printf("+---+---+---+---+---+---+---+---+\n");
    }
    printf("  a   b   c   d   e   f   g   h\n");

    printf("Side to move  : %s\n", pos->side_to_move == COLOR_WHITE ? "white" : "black");
    printf("Ply           : %d\n", pos->ply);
    printf("Create general: %s\n", pos->can_create_general ? "true" : "false");
    printf("Create king   : %s\n", pos->can_create_king ? "true" : "false");
}
