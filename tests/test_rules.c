// SPDX-License-Identifier: Apache-2.0
/* Native rule tests; build commands are in ../README.md. */
#include <stdio.h>
#include <string.h>
#include "game_rules.h"

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); ++s_failures; } } while (0)

static void test_order_chaos(void)
{
    oc_board_t board = {0};
    for (int c = 0; c < 5; ++c) {
        board.cells[c] = OC_X;
    }
    uint8_t line[OC_RUN];
    CHECK(oc_check(&board, line) == OC_RESULT_ORDER);
    CHECK(line[0] == 0 && line[4] == 4);
    board.cells[5] = OC_X;                       /* Six in a row is not a win. */
    CHECK(oc_check(&board, NULL) == OC_RESULT_NONE);
    memset(&board, 0, sizeof(board));
    for (int k = 0; k < 5; ++k) {
        board.cells[(k + 1) * OC_SIZE + 4 - k] = OC_O;   /* Anti-diagonal from (1,4). */
    }
    CHECK(oc_check(&board, NULL) == OC_RESULT_ORDER);
    /* A full board without five is a Chaos win. */
    static const char rows[OC_SIZE][OC_SIZE + 1] = {
        "XXOOXX", "OOXXOO", "XXOOXX", "OOXXOO", "XXOOXX", "OOXXOO",
    };
    for (int i = 0; i < OC_CELLS; ++i) {
        board.cells[i] = rows[i / OC_SIZE][i % OC_SIZE] == 'X' ? OC_X : OC_O;
    }
    CHECK(oc_check(&board, NULL) == OC_RESULT_CHAOS);

    /* Order AI completes an open four on every difficulty above easy. */
    memset(&board, 0, sizeof(board));
    for (int c = 0; c < 4; ++c) {
        board.cells[2 * OC_SIZE + c] = OC_X;
    }
    uint32_t rng = 1;
    for (int d = GAME_DIFFICULTY_MEDIUM; d <= GAME_DIFFICULTY_HARD; ++d) {
        oc_move_t move;
        CHECK(oc_ai_move(&board, OC_ROLE_ORDER, (game_difficulty_t)d, &rng, &move));
        oc_board_t next = board;
        next.cells[move.index] = move.mark;
        CHECK(d == GAME_DIFFICULTY_MEDIUM || oc_check(&next, NULL) == OC_RESULT_ORDER);
        CHECK(next.cells[move.index] != OC_EMPTY);
    }
    oc_move_t easy;
    CHECK(oc_ai_move(&board, OC_ROLE_CHAOS, GAME_DIFFICULTY_EASY, &rng, &easy));
    CHECK(board.cells[easy.index] == OC_EMPTY);
}

static void test_uttt(void)
{
    ut_state_t s;
    ut_init(&s);
    CHECK(s.turn == UT_X && s.active == UT_ANY_BOARD);
    CHECK(ut_apply(&s, 4, 2));                   /* X plays top-right cell of centre. */
    CHECK(s.active == 2 && s.turn == UT_O);
    CHECK(!ut_apply(&s, 4, 0));                  /* O must play in board 2. */
    CHECK(ut_apply(&s, 2, 4));
    CHECK(s.active == 4);
    /* X wins board 0 by a row; a move pointing at a decided board frees the next player. */
    ut_init(&s);
    s.cells[0][0] = s.cells[0][1] = UT_X;
    s.active = 0;
    CHECK(ut_apply(&s, 0, 2));
    CHECK(s.small[0] == UT_X);
    s.active = 3;
    s.turn = UT_O;
    CHECK(ut_apply(&s, 3, 0));                   /* Cell 0 points at won board 0. */
    CHECK(s.active == UT_ANY_BOARD);
    /* Three small boards in a row win the game. */
    ut_init(&s);
    s.small[0] = s.small[1] = UT_O;
    s.cells[2][0] = s.cells[2][1] = UT_O;
    s.turn = UT_O;
    s.active = 2;
    CHECK(ut_apply(&s, 2, 2));
    CHECK(s.winner == UT_O);
    CHECK(!ut_board_playable(&s, 5));
    /* Medium AI takes a small-board win. */
    ut_init(&s);
    s.cells[6][3] = s.cells[6][4] = UT_X;
    s.active = 6;
    int b = -1, c = -1;
    uint32_t rng = 7;
    CHECK(ut_ai_move(&s, GAME_DIFFICULTY_MEDIUM, &rng, &b, &c));
    CHECK(b == 6 && c == 5);
    CHECK(ut_ai_move(&s, GAME_DIFFICULTY_EASY, &rng, &b, &c));
    CHECK(ut_move_valid(&s, b, c));
}

static void test_dandelions(void)
{
    da_state_t s;
    da_init(&s);
    CHECK(da_place(&s, 12));                     /* Centre flower. */
    CHECK(s.turn == DA_ROLE_WIND);
    CHECK(!da_place(&s, 0));
    CHECK(da_blow(&s, DA_E));
    CHECK(s.cells[13] == DA_SEED && s.cells[14] == DA_SEED && s.cells[11] == DA_EMPTY);
    CHECK(da_direction_used(&s, DA_E));
    CHECK(da_place(&s, 0));
    CHECK(!da_blow(&s, DA_E));                   /* Directions are single-use. */
    CHECK(da_blow(&s, DA_SE));
    CHECK(s.cells[6] == DA_SEED && s.cells[24] == DA_SEED && s.cells[18] == DA_SEED);
    /* Wind wins when both sides run out of moves with gaps left. */
    da_init(&s);
    int dirs[] = {DA_N, DA_S, DA_E, DA_W, DA_NE, DA_NW, DA_SE};
    for (int t = 0; t < DA_TURNS; ++t) {
        int cell = 0;
        while (s.cells[cell] != DA_EMPTY) {
            ++cell;
        }
        CHECK(da_place(&s, cell));
        if (s.result != DA_RESULT_NONE) {
            break;
        }
        CHECK(da_blow(&s, (da_direction_t)dirs[t]) || s.result != DA_RESULT_NONE);
        if (s.result != DA_RESULT_NONE) {
            break;
        }
    }
    CHECK(s.result != DA_RESULT_NONE);
    /* AI always returns a legal action. */
    da_init(&s);
    uint32_t rng = 3;
    for (int step = 0; step < 2 * DA_TURNS && s.result == DA_RESULT_NONE; ++step) {
        int action = -1;
        CHECK(da_ai_move(&s, (game_difficulty_t)(step % GAME_DIFFICULTY_COUNT), &rng, &action));
        const bool ok = s.turn == DA_ROLE_FLOWERS ? da_place(&s, action) : da_blow(&s, (da_direction_t)action);
        CHECK(ok);
    }
    CHECK(s.result != DA_RESULT_NONE);
}

int main(void)
{
    test_order_chaos();
    test_uttt();
    test_dandelions();
    printf(s_failures ? "%d failure(s)\n" : "all rule tests passed\n", s_failures);
    return s_failures != 0;
}
