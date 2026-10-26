// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pure rules and computer players for the three games. No allocation, drawing
 * or hardware access; every function works on caller-owned values. */

typedef enum {
    GAME_DIFFICULTY_EASY = 0,
    GAME_DIFFICULTY_MEDIUM,
    GAME_DIFFICULTY_HARD,
    GAME_DIFFICULTY_COUNT,
} game_difficulty_t;

/** Advance a nonzero xorshift32 state and return the next value. */
uint32_t game_rng_next(uint32_t *state);

/* ---- Order & Chaos: 6x6, Order needs exactly five of one mark in a row. ---- */
enum { OC_SIZE = 6, OC_CELLS = OC_SIZE * OC_SIZE, OC_RUN = 5 };

typedef enum { OC_EMPTY = 0, OC_X, OC_O } oc_mark_t;
typedef enum { OC_ROLE_ORDER = 0, OC_ROLE_CHAOS } oc_role_t;
typedef enum { OC_RESULT_NONE = 0, OC_RESULT_ORDER, OC_RESULT_CHAOS } oc_result_t;

typedef struct {
    uint8_t cells[OC_CELLS];   /**< oc_mark_t per cell, row-major. */
} oc_board_t;

typedef struct {
    uint8_t index;             /**< Cell 0..OC_CELLS-1. */
    uint8_t mark;              /**< OC_X or OC_O. */
} oc_move_t;

/** Return the result; on an Order win, copy the winning cells to line if non-NULL. */
oc_result_t oc_check(const oc_board_t *board, uint8_t line[OC_RUN]);
/** Choose a move for ai_role; returns false when the board is full. */
bool oc_ai_move(const oc_board_t *board, oc_role_t ai_role, game_difficulty_t difficulty,
                uint32_t *rng, oc_move_t *out);

/* ---- Ultimate Tic-Tac-Toe: nine boards; a move sends the opponent to a board. ---- */
enum { UT_BOARDS = 9, UT_CELLS = 9, UT_ANY_BOARD = -1 };

typedef enum { UT_EMPTY = 0, UT_X, UT_O, UT_DRAW } ut_mark_t;

typedef struct {
    uint8_t cells[UT_BOARDS][UT_CELLS]; /**< ut_mark_t: UT_EMPTY, UT_X or UT_O. */
    uint8_t small[UT_BOARDS];           /**< Decided small boards; UT_EMPTY while open. */
    int8_t active;                      /**< Required board 0..8, or UT_ANY_BOARD. */
    uint8_t turn;                       /**< UT_X or UT_O; X moves first. */
    uint8_t winner;                     /**< UT_EMPTY while playing, else UT_X/UT_O/UT_DRAW. */
} ut_state_t;

void ut_init(ut_state_t *state);
/** True when the board is open and allowed by the active-board rule. */
bool ut_board_playable(const ut_state_t *state, int board);
bool ut_move_valid(const ut_state_t *state, int board, int cell);
/** Apply a valid move for the player to move; returns false and changes nothing otherwise. */
bool ut_apply(ut_state_t *state, int board, int cell);
/** Choose a move for the player to move; returns false when the game is over. */
bool ut_ai_move(const ut_state_t *state, game_difficulty_t difficulty, uint32_t *rng,
                int *out_board, int *out_cell);

/* ---- Dandelions: 5x5, seven flowers against seven distinct wind directions. ---- */
enum { DA_SIZE = 5, DA_CELLS = DA_SIZE * DA_SIZE, DA_TURNS = 7, DA_DIRECTIONS = 8 };

typedef enum { DA_EMPTY = 0, DA_FLOWER, DA_SEED } da_cell_t;
typedef enum { DA_ROLE_FLOWERS = 0, DA_ROLE_WIND } da_role_t;
/** Compass order used by the rules and computer tie-breaks. */
typedef enum { DA_N = 0, DA_NE, DA_E, DA_SE, DA_S, DA_SW, DA_W, DA_NW } da_direction_t;
typedef enum { DA_RESULT_NONE = 0, DA_RESULT_FLOWERS, DA_RESULT_WIND } da_result_t;

typedef struct {
    uint8_t cells[DA_CELLS];     /**< da_cell_t per cell, row-major. */
    uint8_t used_directions;     /**< Bit per da_direction_t already blown. */
    uint8_t flowers_left;        /**< 0..DA_TURNS. */
    uint8_t wind_left;           /**< 0..DA_TURNS. */
    uint8_t turn;                /**< da_role_t to move; flowers move first. */
    uint8_t result;              /**< da_result_t. */
} da_state_t;

void da_init(da_state_t *state);
bool da_place(da_state_t *state, int cell);
bool da_blow(da_state_t *state, da_direction_t direction);
bool da_direction_used(const da_state_t *state, da_direction_t direction);
/** Choose the next action for the side to move: a cell for flowers, a direction for wind. */
bool da_ai_move(const da_state_t *state, game_difficulty_t difficulty, uint32_t *rng, int *out);

#ifdef __cplusplus
}
#endif
