#include <unity.h>
#include "../src/movegen.h"

void setUp(void) {
    movegen_init();
}

void tearDown(void) {
}

void start_position() {
    Position pos = position_from_fen("SSSSSS2/SSSSS2s/SSSS2ss/SSS2sss/SS2ssss/S2sssss/2ssssss 0 w 00");
    uint64_t num_nodes = perft(&pos, 3, false);
    TEST_ASSERT_EQUAL(num_nodes, 185400);
}

void tactical_position() {
    Position pos = position_from_fen("1G1SK3/G7/S1S4k/SSSS2g1/SS2ssgs/S2ssgs1/2ssssss 22 w 10");
    uint64_t num_nodes = perft(&pos, 4, false);
    TEST_ASSERT_EQUAL(num_nodes, 329941);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(start_position);
    RUN_TEST(tactical_position);
    return UNITY_END();
}
