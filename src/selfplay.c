#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#include "list.h"
#include "movegen.h"
#include "position.h"
#include "tt.h"
#include "nnue_data.h"
#include "nnue.h"
#include "search.h"
#include "state.h"

#define NUM_PROCESS 15
#define GAMES_PER_PROCESS 128000

typedef struct {
    Position position;
    int32_t score;
} ScoredPosition;

typedef struct {
    ScoredPosition elems[1024];
    int size;
} PositionList;

int main() {
    movegen_init();
    position_init();

    for (int process = 0; process < NUM_PROCESS; process++) {
        __pid_t pid = fork();

        if (pid == 0) {
            srand(getpid());

            // Silence child process
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            printf("Child process %d starting\n", process);

            PositionList pos_list;

            TT tt_1 = tt_new(128);
            TT tt_2 = tt_new(128);

            char path[128];
            snprintf(path, 128, "selfplay-%d.txt", process);

            FILE *file = fopen(path, "w");

            if (file == NULL) {
                printf("Failed to open %s\n", path);
                return 1;
            }

            Network net;
            load_network_from_bytes(&net, nnue_bin, nnue_bin_len);

            State state_1;
            State state_2;

            state_1.tt = tt_1;
            state_1.net = net;

            // Think 20ms
            state_1.max_time = 20;
            state_2.max_time = 20;

            state_2.tt = tt_2;
            state_2.net = net;

            for (int game = 0; game < GAMES_PER_PROCESS; game++) {
                Position position = position_from_fen(FEN_START, &net);
                state_1.position = position;

                list_clear(&pos_list);

                memset(state_1.tt.entries, 0, state_1.tt.size * sizeof(TTEntry));
                memset(state_2.tt.entries, 0, state_2.tt.size * sizeof(TTEntry));

                list_clear(&state_1.game_history);
                list_clear(&state_2.game_history);

                list_push(&state_1.game_history, position.hash);
                list_push(&state_2.game_history, position.hash);

                atomic_store(&state_1.stopped, false);
                atomic_store(&state_2.stopped, false);

                GameState white_result;

                int remaining_random_moves = rand() % 9 + 1;

                for (;;) {
                    SearchResult search_result;

                    if (remaining_random_moves > 0) {
                        MoveList moves;
                        legal_moves(&position, &moves);
                        search_result = (SearchResult){
                            .best_move = moves.elems[rand() % moves.size],
                            .score = 0,
                        };
                        remaining_random_moves--;
                    } else if (position.side_to_move == COLOR_WHITE) {
                        state_1.position = position;
                        search_result = search(&state_1);
                    } else {
                        state_2.position = position;
                        search_result = search(&state_2);
                        // Use score from white's POV
                        search_result.score *= -1;
                    }

                    // End game if mating score is found
                    if (abs(search_result.score) >= 90000) {
                        if (position.side_to_move == COLOR_WHITE) {
                            white_result = STATE_WIN;
                            break;
                        } else {
                            white_result = STATE_LOSS;
                            break;
                        }
                    }

                    position = make_move(&position, &net, search_result.best_move);

                    // Check threefold
                    int repetition_counter = 0;
                    for (int i = 0; i < state_1.game_history.size; i++) {
                        if (state_1.game_history.elems[i] == position.hash) {
                            repetition_counter++;
                        }
                    }

                    if (repetition_counter >= 2) {
                        white_result = STATE_DRAW;
                        break;
                    }

                    GameState game_state = position_state(&position);

                    if (game_state != STATE_ONGOING) {
                        if (game_state == STATE_DRAW) {
                            white_result = game_state;
                        } else if ((position.side_to_move == COLOR_WHITE && game_state == STATE_WIN) || (
                                       position.side_to_move == COLOR_BLACK && game_state == STATE_LOSS)) {
                            white_result = STATE_WIN;
                        } else {
                            white_result = STATE_LOSS;
                        }

                        break;
                    }

                    list_push(&state_1.game_history, position.hash);
                    list_push(&state_2.game_history, position.hash);

                    // Skip tactical positions
                    if (position.ply >= 10) {
                        if (bb_popcnt(position.pieces[COLOR_WHITE][PIECE_KING]) == 0 || bb_popcnt(
                                position.pieces[COLOR_BLACK][PIECE_KING]) == 0) {
                            continue;
                        }
                        if (position.can_create_general || position.can_create_king) {
                            continue;
                        }
                    }

                    // Skip random setup moves
                    if (remaining_random_moves + 1 > 0) {
                        continue;
                    }

                    if (pos_list.size >= 1024) {
                        printf("Game too long!\n");
                        return 1;
                    }

                    ScoredPosition scored_position = {
                        .position = position,
                        .score = search_result.score
                    };
                    list_push(&pos_list, scored_position);
                }

                for (int i = 0; i < pos_list.size; i++) {
                    fprintf(file, "%s|%d|%d\n", position_to_fen(&pos_list.elems[i].position), pos_list.elems[i].score,
                            (int) white_result);
                }
            }

            tt_free(&tt_1);
            tt_free(&tt_2);

            fclose(file);

            return 0;
        }
    }

    for (int i = 0; i < NUM_PROCESS; i++) {
        int result;
        wait(&result);

        printf("Child exited with result %d\n", result);
    }
}
