// SPDX-License-Identifier: Apache-2.0
/* Screen flow, turn order and computer pacing for the Math Games app. */
#include <stdlib.h>
#include <string.h>
#include "include/app_game.h"

enum {
    AI_DELAY_TICKS = 15,           /* About 0.5 s at 30 Hz before a computer move. */
    RNG_TICK_MIX = 0x45d9f3bU,
};

struct app_game {
    app_game_config_t config;
    app_game_snapshot_t state;
    uint32_t rng;
    uint32_t ai_wait;
};

static void update_turn_flags(app_game_handle_t game)
{
    app_game_snapshot_t *s = &game->state;
    switch (s->game) {
    case APP_GAME_ORDER_CHAOS: {
        int filled = 0;
        for (int i = 0; i < OC_CELLS; ++i) {
            filled += s->oc.cells[i] != OC_EMPTY;
        }
        s->side_to_move = (uint8_t)(filled & 1);   /* Order moves on even counts. */
        s->game_over = s->oc_result != OC_RESULT_NONE;
        break;
    }
    case APP_GAME_UTTT:
        s->side_to_move = s->ut.turn == UT_X ? 0 : 1;
        s->game_over = s->ut.winner != UT_EMPTY;
        break;
    default:
        s->side_to_move = s->da.turn == DA_ROLE_FLOWERS ? 0 : 1;
        s->game_over = s->da.result != DA_RESULT_NONE;
        break;
    }
    s->computer_thinking = s->screen == APP_SCREEN_PLAY && !s->game_over && s->setup.vs_computer &&
                           s->side_to_move != s->setup.human_side;
    game->ai_wait = 0;
}

static void start_game(app_game_handle_t game)
{
    app_game_snapshot_t *s = &game->state;
    memset(&s->oc, 0, sizeof(s->oc));
    s->oc_mark = OC_X;
    s->oc_result = OC_RESULT_NONE;
    memset(s->oc_line, 0, sizeof(s->oc_line));
    ut_init(&s->ut);
    s->ut_zoom = -1;
    da_init(&s->da);
    s->last_cell = -1;
    s->last_board = -1;
    s->screen = APP_SCREEN_PLAY;
    /* Human timing varies the easy computer's choices; replays stay deterministic. */
    game->rng ^= (s->tick + 1U) * RNG_TICK_MIX;
    update_turn_flags(game);
}

static void home_state(app_game_handle_t game)
{
    const uint32_t rng = game->rng;
    game->state = (app_game_snapshot_t) {
        0
    };
    game->state.screen = APP_SCREEN_HOME;
    game->state.setup.difficulty = GAME_DIFFICULTY_MEDIUM;
    ut_init(&game->state.ut);
    da_init(&game->state.da);
    game->state.ut_zoom = -1;
    game->state.last_cell = -1;
    game->state.last_board = -1;
    game->rng = rng;
    game->ai_wait = 0;
}

app_game_err_t app_game_create(const app_game_config_t *config, app_game_handle_t *ret_handle)
{
    if (!ret_handle) {
        return APP_GAME_INVALID_ARGUMENT;
    }
    *ret_handle = NULL;
    if (!config || !config->width || !config->height) {
        return APP_GAME_INVALID_ARGUMENT;
    }
    app_game_handle_t game = calloc(1, sizeof(*game));
    if (!game) {
        return APP_GAME_NO_MEMORY;
    }
    game->config = *config;
    game->rng = 1;
    home_state(game);
    *ret_handle = game;
    return APP_GAME_OK;
}

void app_game_destroy(app_game_handle_t *handle)
{
    if (handle) {
        free(*handle);
        *handle = NULL;
    }
}

void app_game_reset(app_game_handle_t game)
{
    if (game) {
        game->rng = 1;
        home_state(game);
    }
}

void app_game_set_paused(app_game_handle_t game, bool paused)
{
    if (game) {
        game->state.paused = paused;
    }
}

void app_game_set_pointer(app_game_handle_t game, int32_t x, int32_t y, bool pressed)
{
    if (!game) {
        return;
    }
    game->state.pointer_x = x < 0 ? 0 : (x >= game->config.width ? game->config.width - 1 : x);
    game->state.pointer_y = y < 0 ? 0 : (y >= game->config.height ? game->config.height - 1 : y);
    game->state.pointer_down = pressed;
}

static bool play_oc_cell(app_game_handle_t game, int cell, uint8_t mark)
{
    app_game_snapshot_t *s = &game->state;
    if (cell < 0 || cell >= OC_CELLS || s->oc.cells[cell] != OC_EMPTY) {
        return false;
    }
    s->oc.cells[cell] = mark;
    s->oc_result = oc_check(&s->oc, s->oc_line);
    s->last_cell = (int8_t)cell;
    return true;
}

static bool play_ut_cell(app_game_handle_t game, int board, int cell)
{
    app_game_snapshot_t *s = &game->state;
    if (!ut_apply(&s->ut, board, cell)) {
        return false;
    }
    s->last_board = (int8_t)board;
    s->last_cell = (int8_t)cell;
    s->ut_zoom = -1;   /* Return to the overview so both players see the whole game. */
    return true;
}

static bool play_da(app_game_handle_t game, int value)
{
    app_game_snapshot_t *s = &game->state;
    if (s->da.turn == DA_ROLE_FLOWERS) {
        if (!da_place(&s->da, value)) {
            return false;
        }
        s->last_cell = (int8_t)value;
        return true;
    }
    return da_blow(&s->da, (da_direction_t)value);
}

static bool dispatch_play(app_game_handle_t game, app_action_t action, int arg)
{
    app_game_snapshot_t *s = &game->state;
    if (action == APP_ACTION_START) {
        start_game(game);
        return true;
    }
    if (action == APP_ACTION_BACK) {
        if (s->game == APP_GAME_UTTT && s->ut_zoom >= 0) {
            s->ut_zoom = -1;
        } else {
            s->screen = APP_SCREEN_SETUP;
            update_turn_flags(game);
        }
        return true;
    }
    if (s->game_over || s->computer_thinking) {
        return false;
    }
    bool moved = false;
    switch (action) {
    case APP_ACTION_OC_PICK_MARK:
        if (s->game != APP_GAME_ORDER_CHAOS || (arg != OC_X && arg != OC_O)) {
            return false;
        }
        s->oc_mark = (uint8_t)arg;
        return true;
    case APP_ACTION_UT_OPEN_BOARD:
        if (s->game != APP_GAME_UTTT || s->ut_zoom >= 0 || !ut_board_playable(&s->ut, arg)) {
            return false;
        }
        s->ut_zoom = (int8_t)arg;
        return true;
    case APP_ACTION_CELL:
        if (s->game == APP_GAME_ORDER_CHAOS) {
            moved = play_oc_cell(game, arg, s->oc_mark);
        } else if (s->game == APP_GAME_UTTT) {
            moved = s->ut_zoom >= 0 && play_ut_cell(game, s->ut_zoom, arg);
        } else {
            moved = s->da.turn == DA_ROLE_FLOWERS && play_da(game, arg);
        }
        break;
    case APP_ACTION_DA_WIND:
        moved = s->game == APP_GAME_DANDELIONS && s->da.turn == DA_ROLE_WIND && play_da(game, arg);
        break;
    default:
        return false;
    }
    if (moved) {
        update_turn_flags(game);
    }
    return moved;
}

bool app_game_dispatch(app_game_handle_t game, app_action_t action, int arg)
{
    if (!game || game->state.paused) {
        return false;
    }
    app_game_snapshot_t *s = &game->state;
    switch (s->screen) {
    case APP_SCREEN_HOME:
        if (action != APP_ACTION_OPEN_GAME || arg < 0 || arg >= APP_GAME_KIND_COUNT) {
            return false;
        }
        s->game = (uint8_t)arg;
        s->screen = APP_SCREEN_SETUP;
        return true;
    case APP_SCREEN_SETUP:
        switch (action) {
        case APP_ACTION_SET_VS_COMPUTER: s->setup.vs_computer = arg != 0; return true;
        case APP_ACTION_SET_SIDE: s->setup.human_side = arg ? 1 : 0; return true;
        case APP_ACTION_SET_DIFFICULTY:
            if (arg < 0 || arg >= GAME_DIFFICULTY_COUNT) {
                return false;
            }
            s->setup.difficulty = (uint8_t)arg;
            return true;
        case APP_ACTION_SHOW_RULES: s->screen = APP_SCREEN_RULES; return true;
        case APP_ACTION_START: start_game(game); return true;
        case APP_ACTION_BACK: s->screen = APP_SCREEN_HOME; return true;
        default: return false;
        }
    case APP_SCREEN_RULES:
        if (action != APP_ACTION_BACK) {
            return false;
        }
        s->screen = APP_SCREEN_SETUP;
        return true;
    default:
        return dispatch_play(game, action, arg);
    }
}

static void computer_move(app_game_handle_t game)
{
    app_game_snapshot_t *s = &game->state;
    const game_difficulty_t level = (game_difficulty_t)s->setup.difficulty;
    if (s->game == APP_GAME_ORDER_CHAOS) {
        const oc_role_t role = s->side_to_move ? OC_ROLE_CHAOS : OC_ROLE_ORDER;
        oc_move_t move;
        if (oc_ai_move(&s->oc, role, level, &game->rng, &move)) {
            play_oc_cell(game, move.index, move.mark);
        }
    } else if (s->game == APP_GAME_UTTT) {
        int board, cell;
        if (ut_ai_move(&s->ut, level, &game->rng, &board, &cell)) {
            play_ut_cell(game, board, cell);
        }
    } else {
        int value;
        if (da_ai_move(&s->da, level, &game->rng, &value)) {
            play_da(game, value);
        }
    }
    update_turn_flags(game);
}

void app_game_update(app_game_handle_t game)
{
    if (!game || game->state.paused) {
        return;
    }
    ++game->state.tick;
    if (game->state.computer_thinking && ++game->ai_wait >= AI_DELAY_TICKS) {
        computer_move(game);
    }
}

app_game_snapshot_t app_game_read(app_game_handle_t game)
{
    return game ? game->state : (app_game_snapshot_t) {
        0
    };
}
