// SPDX-License-Identifier: Apache-2.0
/* Dandelions rules and computer player, ported from the reference web game. */
#include <limits.h>
#include <string.h>
#include "include/game_rules.h"

static const int8_t s_step[DA_DIRECTIONS][2] = {
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1},
};

void da_init(da_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->flowers_left = DA_TURNS;
    state->wind_left = DA_TURNS;
    state->turn = DA_ROLE_FLOWERS;
}

bool da_direction_used(const da_state_t *state, da_direction_t direction)
{
    return (state->used_directions >> direction) & 1U;
}

/* Every flower sends seeds along the whole ray to the edge, filling empty cells. */
static void blow_cells(uint8_t cells[DA_CELLS], da_direction_t direction)
{
    uint8_t before[DA_CELLS];
    memcpy(before, cells, sizeof(before));
    const int dr = s_step[direction][0], dc = s_step[direction][1];
    for (int i = 0; i < DA_CELLS; ++i) {
        if (before[i] != DA_FLOWER) {
            continue;
        }
        for (int r = i / DA_SIZE + dr, c = i % DA_SIZE + dc;
             r >= 0 && r < DA_SIZE && c >= 0 && c < DA_SIZE; r += dr, c += dc) {
            if (cells[r * DA_SIZE + c] == DA_EMPTY) {
                cells[r * DA_SIZE + c] = DA_SEED;
            }
        }
    }
}

static int count_new_seeds(const uint8_t cells[DA_CELLS], da_direction_t direction)
{
    uint8_t after[DA_CELLS];
    memcpy(after, cells, sizeof(after));
    blow_cells(after, direction);
    int count = 0;
    for (int i = 0; i < DA_CELLS; ++i) {
        count += after[i] != cells[i];
    }
    return count;
}

static void finish_turn(da_state_t *state)
{
    bool covered = true;
    for (int i = 0; i < DA_CELLS; ++i) {
        covered = covered && state->cells[i] != DA_EMPTY;
    }
    if (covered) {
        state->result = DA_RESULT_FLOWERS;
    } else if (!state->flowers_left && !state->wind_left) {
        state->result = DA_RESULT_WIND;
    } else {
        state->turn = state->turn == DA_ROLE_FLOWERS ? DA_ROLE_WIND : DA_ROLE_FLOWERS;
    }
}

bool da_place(da_state_t *state, int cell)
{
    if (state->result != DA_RESULT_NONE || state->turn != DA_ROLE_FLOWERS || cell < 0 ||
            cell >= DA_CELLS || state->cells[cell] != DA_EMPTY || !state->flowers_left) {
        return false;
    }
    state->cells[cell] = DA_FLOWER;
    --state->flowers_left;
    finish_turn(state);
    return true;
}

bool da_blow(da_state_t *state, da_direction_t direction)
{
    if (state->result != DA_RESULT_NONE || state->turn != DA_ROLE_WIND ||
            (unsigned)direction >= DA_DIRECTIONS || da_direction_used(state, direction) || !state->wind_left) {
        return false;
    }
    blow_cells(state->cells, direction);
    state->used_directions |= (uint8_t)(1U << direction);
    --state->wind_left;
    finish_turn(state);
    return true;
}

static int flower_move(const da_state_t *state, game_difficulty_t difficulty, uint32_t *rng)
{
    int empty[DA_CELLS];
    int count = 0;
    for (int i = 0; i < DA_CELLS; ++i) {
        if (state->cells[i] == DA_EMPTY) {
            empty[count++] = i;
        }
    }
    if (difficulty == GAME_DIFFICULTY_EASY) {
        return empty[game_rng_next(rng) % (uint32_t)count];
    }
    /* Pick the cell whose flower would seed the most cells over the unused winds. */
    int best = empty[0], best_score = -1;
    for (int i = 0; i < count; ++i) {
        uint8_t cells[DA_CELLS];
        memcpy(cells, state->cells, sizeof(cells));
        cells[empty[i]] = DA_FLOWER;
        int score = 0;
        for (int d = 0; d < DA_DIRECTIONS; ++d) {
            if (!da_direction_used(state, (da_direction_t)d)) {
                score += count_new_seeds(cells, (da_direction_t)d);
            }
        }
        if (score > best_score) {
            best_score = score;
            best = empty[i];
        }
    }
    return best;
}

static int wind_move(const da_state_t *state, game_difficulty_t difficulty, uint32_t *rng)
{
    int remaining[DA_DIRECTIONS];
    int count = 0;
    for (int d = 0; d < DA_DIRECTIONS; ++d) {
        if (!da_direction_used(state, (da_direction_t)d)) {
            remaining[count++] = d;
        }
    }
    if (difficulty == GAME_DIFFICULTY_EASY) {
        return remaining[game_rng_next(rng) % (uint32_t)count];
    }
    /* Blow the direction that seeds the fewest new cells. The reference Hard
     * search compares orders that always cover the same cells, so it is greedy too. */
    int best = remaining[0], best_score = INT_MAX;
    for (int i = 0; i < count; ++i) {
        const int score = count_new_seeds(state->cells, (da_direction_t)remaining[i]);
        if (score < best_score) {
            best_score = score;
            best = remaining[i];
        }
    }
    return best;
}

bool da_ai_move(const da_state_t *state, game_difficulty_t difficulty, uint32_t *rng, int *out)
{
    if (!out || state->result != DA_RESULT_NONE) {
        return false;
    }
    if (state->turn == DA_ROLE_FLOWERS) {
        *out = flower_move(state, difficulty, rng);
    } else {
        *out = wind_move(state, difficulty, rng);
    }
    return true;
}
