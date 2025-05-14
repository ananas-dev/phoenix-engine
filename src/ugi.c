#include "ugi.h"
#include "state.h"
#include "search.h"
#include "list.h"
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>

pthread_t search_thread;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_ASYNC_JS(char *, em_fgets, (const char* buf, size_t bufsize), {
  return await new Promise((resolve, reject) => {
      if (Module.pending_lines.length > 0) {
        resolve(Module.pending_lines.shift());
      } else {
        Module.pending_fgets.push(resolve);
      }
  }).then((s) => {
      // convert JS string to WASM string
      let l = s.length + 1;
      if (l >= bufsize) {
        // truncate
        l = bufsize - 1;
      }
      Module.stringToUTF8(s.slice(0, l), buf, l);
      return buf;
  });
});

#define readline(buf, bufsize) em_fgets((buf), (bufsize))
#else
#define readline(buf, bufsize) fgets((buf), (bufsize), stdin)
#endif

void *search_thread_func(void *arg) {
    State *state = arg;
    search(state);
    return NULL;
}

void ugi_loop(TT tt, Network net) {
    char input[4096];

    State state;
    state.position = position_from_fen(FEN_START, &net);
    state.tt = tt;
    state.net = net;
    list_clear(&state.game_history);
    atomic_store(&state.stopped, true);

    while (1) {
        readline(input, sizeof(input));

        char *token = strtok(input, " \n");

        if (!token) continue;

        if (strcmp(token, "ugi") == 0) {
            printf("id name Clippy\n");
            printf("id author Lucien Fiorini\n");
            // printf("option name hash type spin default 128 min 1 max 2048\n");
            printf("ugiok\n");
        } else if (strcmp(token, "isready") == 0) {
            printf("readyok\n");
        } else if (strcmp(token, "uginewgame") == 0) {
            list_clear(&state.game_history);
        } else if (strcmp(token, "position") == 0) {
            list_clear(&state.game_history);

            token = strtok(NULL, " \n");

            if (token && strcmp(token, "startpos") == 0) {
                state.position = position_from_fen(FEN_START, &net);
                token = strtok(NULL, " \n");
            } else if (token && strcmp(token, "fen") == 0) {
                char fen[128] = "";
                token = strtok(NULL, " \n");

                int fields = 0;
                while (token && strcmp(token, "moves") != 0 && fields < 6) {
                    if (fen[0] != '\0') strcat(fen, " ");
                    strcat(fen, token);
                    token = strtok(NULL, " \n");
                    fields++;
                }

                if (fields == 5) {
                    state.position = position_from_fen(fen, &net);
                }
            }

            if (token && strcmp(token, "moves") == 0) {
                while ((token = strtok(NULL, " \n")) != NULL) {
                    Move move = uci_to_move(token);
                    state.position = make_move(&state.position, &net, move);
                    list_push(&state.game_history, state.position.hash);
                }
            }
        } else if (strcmp(token, "go") == 0) {
            if (!atomic_load(&state.stopped)) {
                atomic_store(&state.stopped, true);
                pthread_join(search_thread, NULL);
                atomic_store(&state.stopped, false);
            }
            int64_t p1time = -1, p2time = -1, p1inc = 0, p2inc = 0;

            token = strtok(NULL, " \n");
            while (token != NULL) {
                if (strcmp(token, "p1time") == 0) {
                    token = strtok(NULL, " \n");
                    if (token) p1time = strtoll(token, NULL, 10);
                } else if (strcmp(token, "p2time") == 0) {
                    token = strtok(NULL, " \n");
                    if (token) p2time = strtoll(token, NULL, 10);
                } else if (strcmp(token, "p1inc") == 0) {
                    token = strtok(NULL, " \n");
                    if (token) p1inc = strtoll(token, NULL, 10);
                } else if (strcmp(token, "p2inc") == 0) {
                    token = strtok(NULL, " \n");
                    if (token) p2inc = strtoll(token, NULL, 10);
                }
                token = strtok(NULL, " \n");
            }

            if (p1time == -1 || p2time == -1) continue;

            // Simplistic time management
            state.max_time = state.position.side_to_move == COLOR_WHITE
                                 ? (p1time / 30)
                                 : (p2time / 30);

            atomic_store(&state.stopped, false);

            pthread_create(&search_thread, NULL, search_thread_func, &state);
        } else if (strcmp(token, "stop") == 0) {
            if (!atomic_load(&state.stopped)) {
                atomic_store(&state.stopped, true);
                pthread_join(search_thread, NULL);
            }
        } else if (strcmp(token, "query") == 0) {
            token = strtok(NULL, " \n");

            if (strcmp(token, "p1turn") == 0) {
                if (state.position.side_to_move == COLOR_WHITE) {
                    printf("response true\n");
                } else {
                    printf("response false\n");
                }
            } else if (strcmp(token, "gameover") == 0) {
                if (position_state(&state.position) != STATE_ONGOING) {
                    printf("response true\n");
                } else {
                    printf("response false\n");
                }
            } else if (strcmp(token, "result") == 0) {
                GameState game_state = position_state(&state.position);
                if (game_state == STATE_DRAW) {
                    printf("response draw\n");
                } else if (game_state == STATE_ONGOING) {
                    printf("response none\n");
                } else if ((state.position.side_to_move == COLOR_WHITE && game_state == STATE_WIN) || (
                               state.position.side_to_move == COLOR_BLACK && game_state == STATE_LOSS)) {
                    printf("response p1win\n");
                } else {
                    printf("response p2win\n");
                }
            }
        } else if (strcmp(token, "quit") == 0) {
            if (!atomic_load(&state.stopped)) {
                atomic_store(&state.stopped, true);
                pthread_join(search_thread, NULL);
            }

            break;
        } else if (strcmp(token, "d") == 0) {
            position_print(&state.position);
        }

        fflush(stdout);
    }
}
