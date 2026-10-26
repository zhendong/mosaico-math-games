// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include "../../app_game/include/app_game.h"
#include "raylib_lite_raylib.h"

/* Screen geometry shared by touch hit-testing and drawing (480x480 pixels).
 * Keeping both on one table guarantees that what is drawn is what is tapped. */

enum {
    UI_HEADER_H = 56,
    UI_HOME_CARD_Y = 128,
    UI_HOME_CARD_STEP = 112,
    UI_OC_CELL = 64,
    UI_OC_X = 12,
    UI_OC_Y = 70,
    UI_UT_SMALL = 112,
    UI_UT_GAP = 6,
    UI_UT_X = 69,
    UI_UT_Y = 112,
    UI_UT_ZOOM = 342,
    UI_UT_ZOOM_X = 18,
    UI_UT_ZOOM_Y = 116,
    UI_DA_CELL = 62,
    UI_DA_X = 12,
    UI_DA_Y = 112,
    UI_DA_DIR = 46,
    UI_DA_DIR_GAP = 4,
    UI_DA_DIR_X = 330,
    UI_DA_DIR_Y = 160,
};

/** A touch target: the action it sends and its argument. */
typedef struct {
    bool valid;
    app_action_t action;
    int arg;
} ui_target_t;

static inline Rectangle ui_rect(float x, float y, float w, float h)
{
    return (Rectangle) {
        x, y, w, h
    };
}

Rectangle ui_back_button(void);
Rectangle ui_right_button(void);
Rectangle ui_home_card(int game);
Rectangle ui_setup_toggle(int row, int column);   /* row 0 mode, row 1 side. */
Rectangle ui_setup_difficulty(int level);
Rectangle ui_setup_start(void);
Rectangle ui_oc_cell(int cell);
Rectangle ui_oc_mark_button(int mark);
Rectangle ui_ut_small_board(int board);
Rectangle ui_ut_zoom_cell(int cell);
Rectangle ui_da_cell(int cell);
Rectangle ui_da_direction(int direction);
Rectangle ui_result_menu(void);
Rectangle ui_result_again(void);

/** Compass slot 0..8 (row-major, 4 is the centre) holding a direction, or -1. */
int ui_da_slot_direction(int slot);

/** Return the target under (x, y) for the current state; invalid when none. */
ui_target_t ui_hit_test(const app_game_snapshot_t *state, int x, int y);
