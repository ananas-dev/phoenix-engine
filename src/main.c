#include "movegen.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "nnue_data.h"
#include "nnue.h"
#include "ugi.h"

#include <stdio.h>

int main() {
    movegen_init();
    position_init();

    TT tt = tt_new(128);

    Network net;
    load_network_from_bytes(&net, nnue_bin, nnue_bin_len);

    ugi_loop(tt, net);

    tt_free(&tt);
}