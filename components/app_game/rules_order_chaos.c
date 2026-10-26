// SPDX-License-Identifier: Apache-2.0
/* Order & Chaos rules and computer player, ported from the reference web game. */
#include <limits.h>
#include <string.h>
#include "include/game_rules.h"

enum {
    OC_DIRECTIONS = 4,
    OC_HARD_DEPTH = 2,
    OC_HARD_BRANCH = 12,   /* Inner search nodes try only the first 12 empty cells. */
    OC_WIN_SCORE = 1000,
};

static const int8_t s_dir[OC_DIRECTIONS][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

static bool in_bounds(int r, int c)
{
    return r >= 0 && r < OC_SIZE && c >= 0 && c < OC_SIZE;
}

/* Visit every 5-cell line; returns false when the callback stops the walk. */
typedef bool (*line_fn_t)(const oc_board_t *board, const uint8_t line[OC_RUN], int dr, int dc, void *ctx);

static bool for_each_line(const oc_board_t *board, line_fn_t fn, void *ctx)
{
    for (int d = 0; d < OC_DIRECTIONS; ++d) {
        const int dr = s_dir[d][0], dc = s_dir[d][1];
        for (int r = 0; r < OC_SIZE; ++r) {
            for (int c = 0; c < OC_SIZE; ++c) {
                if (!in_bounds(r + dr * (OC_RUN - 1), c + dc * (OC_RUN - 1))) {
                    continue;
                }
                uint8_t line[OC_RUN];
                for (int k = 0; k < OC_RUN; ++k) {
                    line[k] = (uint8_t)((r + dr * k) * OC_SIZE + c + dc * k);
                }
                if (!fn(board, line, dr, dc, ctx)) {
                    return false;
                }
            }
        }
    }
    return true;
}

typedef struct {
    uint8_t *out;
    bool found;
} win_ctx_t;

static bool find_exact_five(const oc_board_t *board, const uint8_t line[OC_RUN], int dr, int dc, void *value)
{
    win_ctx_t *ctx = value;
    const uint8_t mark = board->cells[line[0]];
    if (mark == OC_EMPTY) {
        return true;
    }
    for (int k = 1; k < OC_RUN; ++k) {
        if (board->cells[line[k]] != mark) {
            return true;
        }
    }
    /* Six in a row does not count: the cells beyond both ends must differ. */
    const int r0 = line[0] / OC_SIZE, c0 = line[0] % OC_SIZE;
    const int r1 = line[OC_RUN - 1] / OC_SIZE, c1 = line[OC_RUN - 1] % OC_SIZE;
    if (in_bounds(r0 - dr, c0 - dc) && board->cells[(r0 - dr) * OC_SIZE + c0 - dc] == mark) {
        return true;
    }
    if (in_bounds(r1 + dr, c1 + dc) && board->cells[(r1 + dr) * OC_SIZE + c1 + dc] == mark) {
        return true;
    }
    if (ctx->out) {
        memcpy(ctx->out, line, OC_RUN);
    }
    ctx->found = true;
    return false;
}

oc_result_t oc_check(const oc_board_t *board, uint8_t line[OC_RUN])
{
    win_ctx_t ctx = {line, false};
    for_each_line(board, find_exact_five, &ctx);
    if (ctx.found) {
        return OC_RESULT_ORDER;
    }
    for (int i = 0; i < OC_CELLS; ++i) {
        if (board->cells[i] == OC_EMPTY) {
            return OC_RESULT_NONE;
        }
    }
    return OC_RESULT_CHAOS;
}

typedef struct {
    uint8_t mark;
    int longest;
} longest_ctx_t;

static bool measure_line(const oc_board_t *board, const uint8_t line[OC_RUN], int dr, int dc, void *value)
{
    (void)dr;
    (void)dc;
    longest_ctx_t *ctx = value;
    int count = 0;
    for (int k = 0; k < OC_RUN; ++k) {
        const uint8_t cell = board->cells[line[k]];
        if (cell == ctx->mark) {
            ++count;
        } else if (cell != OC_EMPTY) {
            return true;   /* Blocked by the other mark. */
        }
    }
    if (count > ctx->longest) {
        ctx->longest = count;
    }
    return true;
}

/* Most marks of one kind in any line not blocked by the other kind. */
static int longest_open_line(const oc_board_t *board, uint8_t mark)
{
    longest_ctx_t ctx = {mark, 0};
    for_each_line(board, measure_line, &ctx);
    return ctx.longest;
}

static int collect_empty(const oc_board_t *board, uint8_t out[OC_CELLS])
{
    int count = 0;
    for (int i = 0; i < OC_CELLS; ++i) {
        if (board->cells[i] == OC_EMPTY) {
            out[count++] = (uint8_t)i;
        }
    }
    return count;
}

static oc_role_t result_role(oc_result_t result)
{
    return result == OC_RESULT_ORDER ? OC_ROLE_ORDER : OC_ROLE_CHAOS;
}

static int score_board(const oc_board_t *board, oc_role_t role)
{
    const oc_result_t result = oc_check(board, NULL);
    if (result != OC_RESULT_NONE) {
        return result_role(result) == role ? OC_WIN_SCORE : -OC_WIN_SCORE;
    }
    const int lines = longest_open_line(board, OC_X) + longest_open_line(board, OC_O);
    return role == OC_ROLE_ORDER ? lines : -lines;
}

static int max_int(int a, int b)
{
    return a > b ? a : b;
}

static int min_int(int a, int b)
{
    return a < b ? a : b;
}

static int minimax(oc_board_t *board, int depth, bool ai_turn, oc_role_t ai_role, int alpha, int beta)
{
    const oc_result_t result = oc_check(board, NULL);
    if (result != OC_RESULT_NONE) {
        return result_role(result) == ai_role ? OC_WIN_SCORE + depth : -OC_WIN_SCORE - depth;
    }
    if (depth == 0) {
        return score_board(board, ai_role);
    }
    uint8_t empty[OC_CELLS];
    int count = collect_empty(board, empty);
    if (count > OC_HARD_BRANCH) {
        count = OC_HARD_BRANCH;
    }
    int value = ai_turn ? INT_MIN : INT_MAX;
    for (int i = 0; i < count; ++i) {
        for (uint8_t mark = OC_X; mark <= OC_O; ++mark) {
            board->cells[empty[i]] = mark;
            const int child = minimax(board, depth - 1, !ai_turn, ai_role, alpha, beta);
            board->cells[empty[i]] = OC_EMPTY;
            if (ai_turn) {
                value = max_int(value, child);
                alpha = max_int(alpha, value);
            } else {
                value = min_int(value, child);
                beta = min_int(beta, value);
            }
            if (alpha >= beta) {
                return value;
            }
        }
    }
    return value;
}

/* Score a candidate for medium play: Order grows lines, Chaos keeps both short. */
static int medium_score(const oc_board_t *board, oc_role_t ai_role, uint8_t mark)
{
    if (ai_role == OC_ROLE_ORDER) {
        return longest_open_line(board, mark);
    }
    return -max_int(longest_open_line(board, OC_X), longest_open_line(board, OC_O));
}

bool oc_ai_move(const oc_board_t *board, oc_role_t ai_role, game_difficulty_t difficulty,
                uint32_t *rng, oc_move_t *out)
{
    uint8_t empty[OC_CELLS];
    const int count = collect_empty(board, empty);
    if (!count || !out) {
        return false;
    }
    if (difficulty == GAME_DIFFICULTY_EASY) {
        out->index = empty[game_rng_next(rng) % (uint32_t)count];
        out->mark = (game_rng_next(rng) & 1U) ? OC_X : OC_O;
        return true;
    }
    oc_board_t work = *board;
    int best = INT_MIN;
    *out = (oc_move_t) {
        empty[0], OC_X
    };
    /* Candidate order decides ties: medium walks marks then cells, hard walks
     * cells then marks, matching the reference game move for move. */
    const bool hard = difficulty == GAME_DIFFICULTY_HARD;
    for (int outer = 0; outer < (hard ? count : 2); ++outer) {
        for (int inner = 0; inner < (hard ? 2 : count); ++inner) {
            const int i = hard ? outer : inner;
            const uint8_t mark = (uint8_t)(OC_X + (hard ? inner : outer));
            work.cells[empty[i]] = mark;
            const int score = hard ? minimax(&work, OC_HARD_DEPTH, false, ai_role, INT_MIN, INT_MAX)
                              : medium_score(&work, ai_role, mark);
            work.cells[empty[i]] = OC_EMPTY;
            if (score > best) {
                best = score;
                *out = (oc_move_t) {
                    empty[i], mark
                };
            }
        }
    }
    return true;
}
