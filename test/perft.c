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

void tactical_position_1() {
    Position pos = position_from_fen("1G1SK3/G7/S1S4k/SSSS2g1/SS2ssgs/S2ssgs1/2ssssss 22 w 10");
    uint64_t num_nodes = perft(&pos, 4, false);
    TEST_ASSERT_EQUAL(num_nodes, 329941);
}

void tactical_position_2() {
    Position pos = position_from_fen("GS1S1S2/2KS4/2SS4/GGS1Ssgs/S1S1ssgs/4ks1s/S2g1sss 21 b 00");
    uint64_t num_nodes = perft(&pos, 7, false);
    TEST_ASSERT_EQUAL(num_nodes, 518036);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(start_position);
    RUN_TEST(tactical_position_1);
    RUN_TEST(tactical_position_2);
    return UNITY_END();
}
