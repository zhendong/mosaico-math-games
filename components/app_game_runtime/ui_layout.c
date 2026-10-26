// SPDX-License-Identifier: Apache-2.0
#include "private/ui_layout.h"

static const int8_t s_compass[9] = {DA_NW, DA_N, DA_NE, DA_W, -1, DA_E, DA_SW, DA_S, DA_SE};

Rectangle ui_back_button(void)
{
    return ui_rect(8, 8, 104, 40);
}

Rectangle ui_right_button(void)
{
    return ui_rect(372, 8, 100, 40);
}

Rectangle ui_home_card(int game)
{
    return ui_rect(16, (float)(UI_HOME_CARD_Y + game * UI_HOME_CARD_STEP), 448, 100);
}

Rectangle ui_setup_toggle(int row, int column)
{
    return ui_rect((float)(16 + column * 228), (float)(90 + row * 110), 220, 78);
}

Rectangle ui_setup_difficulty(int level)
{
    return ui_rect((float)(16 + level * 152), 310, 144, 52);
}

Rectangle ui_setup_start(void)
{
    return ui_rect(16, 390, 448, 70);
}

Rectangle ui_oc_cell(int cell)
{
    return ui_rect((float)(UI_OC_X + (cell % OC_SIZE) * UI_OC_CELL),
                   (float)(UI_OC_Y + (cell / OC_SIZE) * UI_OC_CELL), UI_OC_CELL, UI_OC_CELL);
}

Rectangle ui_oc_mark_button(int mark)
{
    return ui_rect(402, (float)(mark == OC_X ? 150 : 266), 72, 104);
}

Rectangle ui_ut_small_board(int board)
{
    return ui_rect((float)(UI_UT_X + (board % 3) * (UI_UT_SMALL + UI_UT_GAP)),
                   (float)(UI_UT_Y + (board / 3) * (UI_UT_SMALL + UI_UT_GAP)), UI_UT_SMALL, UI_UT_SMALL);
}

Rectangle ui_ut_zoom_cell(int cell)
{
    const float size = UI_UT_ZOOM / 3.0f;
    return ui_rect(UI_UT_ZOOM_X + (float)(cell % 3) * size, UI_UT_ZOOM_Y + (float)(cell / 3) * size, size, size);
}

Rectangle ui_da_cell(int cell)
{
    return ui_rect((float)(UI_DA_X + (cell % DA_SIZE) * UI_DA_CELL),
                   (float)(UI_DA_Y + (cell / DA_SIZE) * UI_DA_CELL), UI_DA_CELL, UI_DA_CELL);
}

int ui_da_slot_direction(int slot)
{
    return slot >= 0 && slot < 9 ? s_compass[slot] : -1;
}

Rectangle ui_da_direction(int direction)
{
    int slot = 0;
    while (slot < 9 && s_compass[slot] != direction) {
        ++slot;
    }
    const int step = UI_DA_DIR + UI_DA_DIR_GAP;
    return ui_rect((float)(UI_DA_DIR_X + (slot % 3) * step), (float)(UI_DA_DIR_Y + (slot / 3) * step),
                   UI_DA_DIR, UI_DA_DIR);
}

Rectangle ui_result_menu(void)
{
    return ui_rect(56, 286, 150, 56);
}

Rectangle ui_result_again(void)
{
    return ui_rect(218, 286, 206, 56);
}

static bool hit(Rectangle r, int x, int y)
{
    return CheckCollisionPointRec((Vector2) {
        (float)x, (float)y
    }, r);
}

static ui_target_t target(app_action_t action, int arg)
{
    return (ui_target_t) {
        true, action, arg
    };
}

static ui_target_t hit_setup(const app_game_snapshot_t *s, int x, int y)
{
    for (int column = 0; column < 2; ++column) {
        if (hit(ui_setup_toggle(0, column), x, y)) {
            return target(APP_ACTION_SET_VS_COMPUTER, column);
        }
        if (s->setup.vs_computer && hit(ui_setup_toggle(1, column), x, y)) {
            return target(APP_ACTION_SET_SIDE, column);
        }
    }
    for (int level = 0; s->setup.vs_computer && level < GAME_DIFFICULTY_COUNT; ++level) {
        if (hit(ui_setup_difficulty(level), x, y)) {
            return target(APP_ACTION_SET_DIFFICULTY, level);
        }
    }
    if (hit(ui_setup_start(), x, y)) {
        return target(APP_ACTION_START, 0);
    }
    if (hit(ui_right_button(), x, y)) {
        return target(APP_ACTION_SHOW_RULES, 0);
    }
    return (ui_target_t) {
        0
    };
}

static ui_target_t hit_board(const app_game_snapshot_t *s, int x, int y)
{
    if (s->game == APP_GAME_ORDER_CHAOS) {
        for (int mark = OC_X; mark <= OC_O; ++mark) {
            if (hit(ui_oc_mark_button(mark), x, y)) {
                return target(APP_ACTION_OC_PICK_MARK, mark);
            }
        }
        for (int cell = 0; cell < OC_CELLS; ++cell) {
            if (s->oc.cells[cell] == OC_EMPTY && hit(ui_oc_cell(cell), x, y)) {
                return target(APP_ACTION_CELL, cell);
            }
        }
    } else if (s->game == APP_GAME_UTTT) {
        for (int i = 0; i < 9; ++i) {
            if (s->ut_zoom < 0 && ut_board_playable(&s->ut, i) && hit(ui_ut_small_board(i), x, y)) {
                return target(APP_ACTION_UT_OPEN_BOARD, i);
            }
            if (s->ut_zoom >= 0 && ut_move_valid(&s->ut, s->ut_zoom, i) && hit(ui_ut_zoom_cell(i), x, y)) {
                return target(APP_ACTION_CELL, i);
            }
        }
    } else if (s->da.turn == DA_ROLE_FLOWERS) {
        for (int cell = 0; cell < DA_CELLS; ++cell) {
            if (s->da.cells[cell] == DA_EMPTY && hit(ui_da_cell(cell), x, y)) {
                return target(APP_ACTION_CELL, cell);
            }
        }
    } else {
        for (int d = 0; d < DA_DIRECTIONS; ++d) {
            if (!da_direction_used(&s->da, (da_direction_t)d) && hit(ui_da_direction(d), x, y)) {
                return target(APP_ACTION_DA_WIND, d);
            }
        }
    }
    return (ui_target_t) {
        0
    };
}

ui_target_t ui_hit_test(const app_game_snapshot_t *s, int x, int y)
{
    if (s->screen == APP_SCREEN_HOME) {
        for (int game = 0; game < APP_GAME_KIND_COUNT; ++game) {
            if (hit(ui_home_card(game), x, y)) {
                return target(APP_ACTION_OPEN_GAME, game);
            }
        }
        return (ui_target_t) {
            0
        };
    }
    if (s->screen == APP_SCREEN_PLAY && s->game_over) {
        if (hit(ui_result_menu(), x, y)) {
            return target(APP_ACTION_BACK, 0);
        }
        return hit(ui_result_again(), x, y) ? target(APP_ACTION_START, 0) : (ui_target_t) {
            0
        };
    }
    if (hit(ui_back_button(), x, y)) {
        return target(APP_ACTION_BACK, 0);
    }
    if (s->screen == APP_SCREEN_SETUP) {
        return hit_setup(s, x, y);
    }
    if (s->screen != APP_SCREEN_PLAY) {
        return (ui_target_t) {
            0
        };
    }
    if (hit(ui_right_button(), x, y)) {
        return target(APP_ACTION_START, 0);
    }
    return s->computer_thinking ? (ui_target_t) {
        0
    } : hit_board(s, x, y);
}
