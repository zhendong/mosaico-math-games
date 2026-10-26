// SPDX-License-Identifier: Apache-2.0
/* Ultimate Tic-Tac-Toe rules and computer player, ported from the reference web game. */
#include <limits.h>
#include <string.h>
#include "include/game_rules.h"

enum { UT_LINES = 8 };

static const uint8_t s_lines[UT_LINES][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6},
};

/* Winner of a 3x3 grid; UT_DRAW when full, UT_EMPTY while undecided. */
static uint8_t grid_result(const uint8_t grid[9])
{
    for (int i = 0; i < UT_LINES; ++i) {
        const uint8_t a = grid[s_lines[i][0]];
        if ((a == UT_X || a == UT_O) && a == grid[s_lines[i][1]] && a == grid[s_lines[i][2]]) {
            return a;
        }
    }
    for (int i = 0; i < 9; ++i) {
        if (grid[i] == UT_EMPTY) {
            return UT_EMPTY;
        }
    }
    return UT_DRAW;
}

static bool is_center(int i)
{
    return i == 4;
}

static bool is_corner(int i)
{
    return i == 0 || i == 2 || i == 6 || i == 8;
}

void ut_init(ut_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->active = UT_ANY_BOARD;
    state->turn = UT_X;
}

bool ut_board_playable(const ut_state_t *state, int board)
{
    if (board < 0 || board >= UT_BOARDS || state->winner != UT_EMPTY || state->small[board] != UT_EMPTY) {
        return false;
    }
    return state->active == UT_ANY_BOARD || state->active == board;
}

bool ut_move_valid(const ut_state_t *state, int board, int cell)
{
    return ut_board_playable(state, board) && cell >= 0 && cell < UT_CELLS &&
           state->cells[board][cell] == UT_EMPTY;
}

bool ut_apply(ut_state_t *state, int board, int cell)
{
    if (!ut_move_valid(state, board, cell)) {
        return false;
    }
    state->cells[board][cell] = state->turn;
    state->small[board] = grid_result(state->cells[board]);
    /* The cell position picks the opponent's board unless that board is decided. */
    state->active = state->small[cell] == UT_EMPTY ? (int8_t)cell : UT_ANY_BOARD;
    state->turn = state->turn == UT_X ? UT_O : UT_X;
    /* Drawn small boards count for nobody; a full set of decided boards is a draw. */
    uint8_t large[9];
    for (int i = 0; i < UT_BOARDS; ++i) {
        large[i] = state->small[i] == UT_DRAW ? UT_EMPTY : state->small[i];
    }
    const uint8_t won = grid_result(large);
    if (won == UT_X || won == UT_O) {
        state->winner = won;
    } else {
        bool all_decided = true;
        for (int i = 0; i < UT_BOARDS; ++i) {
            all_decided = all_decided && state->small[i] != UT_EMPTY;
        }
        if (all_decided) {
            state->winner = UT_DRAW;
        }
    }
    return true;
}

/* Medium heuristic: prefer centres and corners, take small wins, and avoid
 * sending the opponent to a strong board. */
static int medium_score(const ut_state_t *state, int board, int cell)
{
    int score = 0;
    score += is_center(cell) ? 3 : (is_corner(cell) ? 1 : 0);
    score += is_center(board) ? 2 : (is_corner(board) ? 1 : 0);
    uint8_t grid[9];
    memcpy(grid, state->cells[board], sizeof(grid));
    grid[cell] = state->turn;
    if (grid_result(grid) == state->turn) {
        score += 10;
    }
    if (state->small[cell] == UT_EMPTY && (is_center(cell) || is_corner(cell))) {
        score -= 2;
    }
    return score;
}

bool ut_ai_move(const ut_state_t *state, game_difficulty_t difficulty, uint32_t *rng,
                int *out_board, int *out_cell)
{
    uint8_t moves[UT_BOARDS * UT_CELLS][2];
    int count = 0;
    for (int b = 0; b < UT_BOARDS; ++b) {
        for (int c = 0; c < UT_CELLS; ++c) {
            if (ut_move_valid(state, b, c)) {
                moves[count][0] = (uint8_t)b;
                moves[count][1] = (uint8_t)c;
                ++count;
            }
        }
    }
    if (!count || !out_board || !out_cell) {
        return false;
    }
    int pick = 0;
    if (difficulty == GAME_DIFFICULTY_EASY) {
        pick = (int)(game_rng_next(rng) % (uint32_t)count);
    } else {
        /* The reference game uses the same heuristic for Medium and Hard. */
        int best = INT_MIN;
        for (int i = 0; i < count; ++i) {
            const int score = medium_score(state, moves[i][0], moves[i][1]);
            if (score > best) {
                best = score;
                pick = i;
            }
        }
    }
    *out_board = moves[pick][0];
    *out_cell = moves[pick][1];
    return true;
}
