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

static uint64_t random_u64() {
    static uint64_t number = 0xFFAAB58C5833FE89;

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    return number;
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

Position make_null_move(Position *pos) {
    Position new_pos = *pos;

    new_pos.can_create_general = false;
    new_pos.can_create_king = false;

    new_pos.ply++;
    new_pos.side_to_move = 1 - new_pos.side_to_move;

    return new_pos;
}

Position make_move(Position *pos, Move move) {
    Color color = pos->side_to_move;
    Position new_pos = *pos;

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

        // Handle stacking
        if (new_pos.pieces[color][PIECE_SOLDIER] & to_mask) {
            new_pos.pieces[color][PIECE_SOLDIER] ^= to_mask;
            new_pos.pieces[color][PIECE_GENERAL] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_SOLDIER][move.to];
            new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];
        } else if (new_pos.pieces[color][PIECE_GENERAL] & to_mask) {
            new_pos.pieces[color][PIECE_GENERAL] ^= to_mask;
            new_pos.pieces[color][PIECE_KING] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];
            new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.to];
        } else {
            new_pos.pieces[color][PIECE_SOLDIER] |= to_mask;
            new_pos.hash ^= zobrists.pieces[color][PIECE_SOLDIER][move.to];
        }
    } else if (new_pos.pieces[color][PIECE_GENERAL] & from_mask) {
        new_pos.pieces[color][PIECE_GENERAL] ^= from_mask;
        new_pos.pieces[color][PIECE_GENERAL] |= to_mask;
        new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.from];
        new_pos.hash ^= zobrists.pieces[color][PIECE_GENERAL][move.to];
    } else if (new_pos.pieces[color][PIECE_KING] & from_mask) {
        new_pos.pieces[color][PIECE_KING] ^= from_mask;
        new_pos.pieces[color][PIECE_KING] |= to_mask;
        new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.from];
        new_pos.hash ^= zobrists.pieces[color][PIECE_KING][move.to];
    } else {
        assert(false && "Impossible move");
    }

    for (Piece piece = PIECE_SOLDIER; piece <= PIECE_KING; piece++) {
        if (new_pos.pieces[1 - color][piece] & captured_mask) {
            Bitboard captured_it = new_pos.pieces[1 - color][piece] & captured_mask;
            while (captured_it) {
                Square square = bb_it_next(&captured_it);
                new_pos.hash ^= zobrists.pieces[1 - color][piece][square];
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

    return new_pos;
}


Position position_from_fen(const char *fen_str) {
    char *fen = malloc((strlen(fen_str) + 1) * sizeof(char));
    strcpy(fen, fen_str);

    char *board = strtok(fen, " ");
    Position position = {0};

    int curr_file = FILE_A;
    int curr_rank = RANK_7;

    for (int i = 0; i < strlen(board); i++) {
        char x = board[i];

        if (x == '/') {
            curr_file = 0;
            curr_rank--;
        } else if (x >= '1' && x <= '9') {
            curr_file += x - '0';
        } else {
            Color color;
            Piece piece;

            switch (x) {
                case 'S':
                    piece = PIECE_SOLDIER;
                    color = COLOR_WHITE;
                    break;
                case 'G':
                    piece = PIECE_GENERAL;
                    color = COLOR_WHITE;
                    break;
                case 'K':
                    piece = PIECE_KING;
                    color = COLOR_WHITE;
                    break;
                case 's':
                    piece = PIECE_SOLDIER;
                    color = COLOR_BLACK;
                    break;
                case 'g':
                    piece = PIECE_GENERAL;
                    color = COLOR_BLACK;
                    break;
                case 'k':
                    piece = PIECE_KING;
                    color = COLOR_BLACK;
                    break;
                default:
                    assert(false && "Unknown piece in FEN");
            }

            position.pieces[color][piece] |= bb_from_sq(sq_get(curr_file, curr_rank));
            curr_file++;
        }
    }

    char *ply_str = strtok(NULL, " ");

    // FIXME: Use strtol to report erros
    position.ply = atoi(ply_str);

    char *side_to_move_str = strtok(NULL, " ");

    if (side_to_move_str[0] == 'w') {
        position.side_to_move = COLOR_WHITE;
    } else if (side_to_move_str[0] == 'b') {
        position.side_to_move = COLOR_BLACK;
    } else {
        assert(false && "Unknown side to move in FEN");
    }

    char *creation_flag = strtok(NULL, " ");

    if (creation_flag[0] == '1') {
        position.can_create_general = true;
    } else if (creation_flag[0] == '0') {
        position.can_create_general = false;
    } else {
        assert(false && "Invalid creation flag in FEN");
    }

    if (creation_flag[1] == '1') {
        position.can_create_king = true;
    } else if (creation_flag[1] == '0') {
        position.can_create_king = false;
    } else {
        assert(false && "Invalid creation flag in FEN");
    }

    free(fen);
    return position;
}


void position_print(Position *pos) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (int rank = 6; rank >= 0; --rank) {
        for (int file = 0; file <= 7; file++) {
            Bitboard square_mask = bb_from_sq(sq_get(file, rank));

            Piece current_piece = -1;
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

            if (current_piece == -1) {
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
