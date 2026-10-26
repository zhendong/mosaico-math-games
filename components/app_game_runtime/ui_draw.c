// SPDX-License-Identifier: Apache-2.0
/* Frame drawing for every screen. The Engine font has capitals, digits and
 * "/-:." only, so copy avoids other punctuation and arrows are drawn as shapes. */
#include <stdio.h>
#include "private/ui_draw.h"

#define COLOR(r, g, b) ((Color){(r), (g), (b), 255})
#define C_BG COLOR(248, 245, 240)
#define C_SURFACE COLOR(255, 255, 255)
#define C_BORDER COLOR(221, 221, 221)
#define C_TEXT COLOR(45, 45, 45)
#define C_MUTED COLOR(136, 136, 136)
#define C_ORDER COLOR(255, 107, 53)
#define C_UTTT COLOR(107, 92, 255)
#define C_DAND COLOR(46, 204, 113)
#define C_PETAL COLOR(255, 196, 0)
#define C_PETAL_CORE COLOR(230, 120, 0)
#define C_X C_ORDER
#define C_O C_UTTT

enum {
    TEXT_SMALL = 16,     /* 2x glyph scale: 12 px per character. */
    TEXT_MEDIUM = 24,    /* 3x glyph scale: 18 px per character. */
    TEXT_LARGE = 40,     /* 5x glyph scale. */
    GLYPH_ROWS = 7,
    ROUND_SEGMENTS = 8,
    HEADER_TITLE_MAX = 250,  /* Pixels between the header buttons. */
};

static const char *const s_titles[APP_GAME_KIND_COUNT] = {"ORDER AND CHAOS", "ULTIMATE TTT", "DANDELIONS"};
static const char *const s_side_names[APP_GAME_KIND_COUNT][2] = {
    {"ORDER", "CHAOS"}, {"X", "O"}, {"FLOWERS", "WIND"},
};

static Color accent(int game)
{
    static const Color colors[APP_GAME_KIND_COUNT] = {C_ORDER, C_UTTT, C_DAND};
    return colors[game < APP_GAME_KIND_COUNT ? game : 0];
}

static void text_at(const char *text, int x, int y, int size, Color color)
{
    DrawText(text, x, y, size, color);
}

static void text_center(const char *text, float cx, float cy, int size, Color color)
{
    const int scale = size / 8 > 0 ? size / 8 : 1;
    DrawText(text, (int)(cx - MeasureText(text, size) / 2.0f), (int)(cy - GLYPH_ROWS * scale / 2.0f), size, color);
}

static void panel(Rectangle r, Color fill, float roundness)
{
    DrawRectangleRounded(r, roundness, ROUND_SEGMENTS, fill);
}

/* Filled border made from two rounded rectangles, so it stays solid at any width. */
static void outlined_panel(Rectangle r, Color fill, Color outline, float width, float roundness)
{
    DrawRectangleRounded(r, roundness, ROUND_SEGMENTS, outline);
    DrawRectangleRounded(ui_rect(r.x + width, r.y + width, r.width - 2 * width, r.height - 2 * width),
                         roundness, ROUND_SEGMENTS, fill);
}

static bool is_pressed(const ui_target_t *pressed, app_action_t action, int arg)
{
    return pressed && pressed->valid && pressed->action == action && pressed->arg == arg;
}

static void draw_x(float cx, float cy, float half, float thick)
{
    DrawLineEx((Vector2) {
        cx - half, cy - half
    }, (Vector2) {
        cx + half, cy + half
    }, thick, C_X);
    DrawLineEx((Vector2) {
        cx - half, cy + half
    }, (Vector2) {
        cx + half, cy - half
    }, thick, C_X);
}

static void draw_o(float cx, float cy, float radius, float thick, Color background)
{
    DrawCircle((int)cx, (int)cy, radius, C_O);
    DrawCircle((int)cx, (int)cy, radius - thick, background);
}

static void draw_mark(int mark, Rectangle cell, float fraction, Color background)
{
    const float cx = cell.x + cell.width / 2, cy = cell.y + cell.height / 2;
    const float half = cell.width * fraction;
    const float thick = half / 3.0f > 3 ? half / 3.0f : 3;
    if (mark == 1) {
        draw_x(cx, cy, half * 0.85f, thick);
    } else if (mark == 2) {
        draw_o(cx, cy, half, thick, background);
    }
}

static void draw_chevron(float cx, float cy, float size, bool left, Color color)
{
    const float dx = left ? size / 2 : -size / 2;
    DrawLineEx((Vector2) {
        cx + dx, cy - size
    }, (Vector2) {
        cx - dx, cy
    }, 4, color);
    DrawLineEx((Vector2) {
        cx - dx, cy
    }, (Vector2) {
        cx + dx, cy + size
    }, 4, color);
}

static void header(const char *title, Color tint, const char *back, const char *action,
                   const ui_target_t *pressed, app_action_t action_id)
{
    DrawRectangle(0, 0, 480, UI_HEADER_H, C_SURFACE);
    DrawRectangle(0, UI_HEADER_H, 480, 1, C_BORDER);
    /* Long titles drop to the small size so they clear both header buttons. */
    const int size = MeasureText(title, TEXT_MEDIUM) <= HEADER_TITLE_MAX ? TEXT_MEDIUM : TEXT_SMALL;
    text_center(title, 240, UI_HEADER_H / 2.0f, size, C_TEXT);
    const Rectangle b = ui_back_button();
    panel(b, is_pressed(pressed, APP_ACTION_BACK, 0) ? C_BORDER : C_BG, 0.9f);
    draw_chevron(b.x + 20, b.y + b.height / 2, 7, true, C_MUTED);
    text_at(back, (int)b.x + 34, (int)(b.y + b.height / 2) - GLYPH_ROWS, TEXT_SMALL, C_MUTED);
    if (action) {
        const Rectangle r = ui_right_button();
        panel(r, is_pressed(pressed, action_id, 0) ? C_TEXT : tint, 0.9f);
        text_center(action, r.x + r.width / 2, r.y + r.height / 2, TEXT_SMALL, C_SURFACE);
    }
}

static void banner(const char *text, Color tint)
{
    outlined_panel(ui_rect(16, 64, 448, 40), C_SURFACE, tint, 2, 0.4f);
    text_center(text, 240, 84, TEXT_SMALL, C_TEXT);
}

/* ---- Home ---- */

static void draw_home(const ui_target_t *pressed)
{
    static const char *const names[APP_GAME_KIND_COUNT] = {"ORDER AND CHAOS", "ULTIMATE TIC-TAC-TOE", "DANDELIONS"};
    static const char *const subs[APP_GAME_KIND_COUNT] = {
        "6X6 - 5 IN A ROW WINS", "9 BOARDS IN 1", "5X5 - FLOWERS VS WIND",
    };
    text_at("MATH GAMES", 24, 24, TEXT_LARGE, C_TEXT);
    text_at("FROM BEN ORLIN - MATH GAMES", 26, 74, TEXT_SMALL, C_MUTED);
    text_at("WITH BAD DRAWINGS", 26, 96, TEXT_SMALL, C_MUTED);
    for (int game = 0; game < APP_GAME_KIND_COUNT; ++game) {
        const Rectangle card = ui_home_card(game);
        panel(card, is_pressed(pressed, APP_ACTION_OPEN_GAME, game) ? C_BORDER : C_SURFACE, 0.2f);
        DrawRectangle((int)card.x, (int)card.y + 10, 6, (int)card.height - 20, accent(game));
        text_at(names[game], (int)card.x + 24, (int)card.y + 26, TEXT_MEDIUM, C_TEXT);
        text_at(subs[game], (int)card.x + 24, (int)card.y + 64, TEXT_SMALL, C_MUTED);
        draw_chevron(card.x + card.width - 26, card.y + card.height / 2, 10, false, accent(game));
    }
}

/* ---- Setup and rules ---- */

static void toggle(Rectangle r, const char *label, const char *sub, bool on, bool down, Color tint)
{
    const Color fill = down ? C_BG : C_SURFACE;
    if (on) {
        outlined_panel(r, fill, tint, 3, 0.25f);
    } else {
        outlined_panel(r, fill, C_BORDER, 1, 0.25f);
    }
    text_center(label, r.x + r.width / 2, r.y + 28, TEXT_MEDIUM, on ? tint : C_TEXT);
    text_center(sub, r.x + r.width / 2, r.y + 58, TEXT_SMALL, C_MUTED);
}

static void draw_setup(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    static const char *const side_subs[APP_GAME_KIND_COUNT][2] = {
        {"MAKE 5 IN A ROW", "BLOCK 5 IN A ROW"}, {"GOES FIRST", "GOES SECOND"}, {"PLACE FLOWERS", "BLOW SEEDS"},
    };
    static const char *const levels[GAME_DIFFICULTY_COUNT] = {"EASY", "MEDIUM", "HARD"};
    const Color tint = accent(s->game);
    header(s_titles[s->game], tint, "GAMES", "RULES", pressed, APP_ACTION_SHOW_RULES);
    text_at("MODE", 20, 70, TEXT_SMALL, C_MUTED);
    toggle(ui_setup_toggle(0, 0), "2 PLAYERS", "SAME SCREEN", !s->setup.vs_computer,
           is_pressed(pressed, APP_ACTION_SET_VS_COMPUTER, 0), tint);
    toggle(ui_setup_toggle(0, 1), "VS COMPUTER", "PLAY SOLO", s->setup.vs_computer,
           is_pressed(pressed, APP_ACTION_SET_VS_COMPUTER, 1), tint);
    if (s->setup.vs_computer) {
        text_at("YOU PLAY AS", 20, 180, TEXT_SMALL, C_MUTED);
        for (int side = 0; side < 2; ++side) {
            toggle(ui_setup_toggle(1, side), s_side_names[s->game][side], side_subs[s->game][side],
                   s->setup.human_side == side, is_pressed(pressed, APP_ACTION_SET_SIDE, side), tint);
        }
        text_at("DIFFICULTY", 20, 290, TEXT_SMALL, C_MUTED);
        for (int level = 0; level < GAME_DIFFICULTY_COUNT; ++level) {
            const Rectangle r = ui_setup_difficulty(level);
            const bool on = s->setup.difficulty == level;
            if (on) {
                panel(r, tint, 0.3f);
            } else {
                outlined_panel(r, is_pressed(pressed, APP_ACTION_SET_DIFFICULTY, level) ? C_BG : C_SURFACE,
                               C_BORDER, 1, 0.3f);
            }
            text_center(levels[level], r.x + r.width / 2, r.y + r.height / 2, TEXT_SMALL, on ? C_SURFACE : C_TEXT);
        }
    } else {
        text_at("TWO PLAYERS TAKE TURNS", 20, 200, TEXT_SMALL, C_MUTED);
        text_at("ON THIS SCREEN.", 20, 222, TEXT_SMALL, C_MUTED);
    }
    const Rectangle start = ui_setup_start();
    panel(start, is_pressed(pressed, APP_ACTION_START, 0) ? C_TEXT : tint, 0.3f);
    text_center("START GAME", start.x + start.width / 2, start.y + start.height / 2, TEXT_MEDIUM, C_SURFACE);
}

static void draw_rules(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    static const char *const rules[APP_GAME_KIND_COUNT][12] = {
        {
            "EACH TURN, PLACE AN X OR AN O", "ON ANY EMPTY CELL.", "",
            "ORDER WINS WITH EXACTLY 5 OF", "THE SAME MARK IN A ROW -", "ACROSS, DOWN OR DIAGONAL.", "",
            "6 IN A ROW DOES NOT COUNT.", "",
            "CHAOS WINS IF THE BOARD FILLS", "UP WITH NO 5 IN A ROW.", NULL,
        },
        {
            "NINE TIC-TAC-TOE BOARDS IN ONE.", "TAP A BOARD TO OPEN IT, THEN", "TAP A CELL.", "",
            "YOUR CELL PICKS THE BOARD YOUR", "OPPONENT PLAYS IN NEXT.", "",
            "3 IN A ROW WINS A SMALL BOARD.", "3 SMALL BOARDS IN A ROW WIN.", "",
            "SENT TO A FINISHED BOARD:", "PLAY IN ANY OPEN BOARD.",
        },
        {
            "FLOWERS PLANT ONE FLOWER ON", "ANY EMPTY CELL EACH TURN.", "",
            "WIND PICKS A DIRECTION. SEEDS", "BLOW FROM EVERY FLOWER IN A", "LINE TO THE EDGE.", "",
            "EACH DIRECTION ONCE ONLY.", "7 TURNS EACH.", "",
            "FLOWERS WIN IF EVERY CELL IS", "COVERED. OTHERWISE WIND WINS.",
        },
    };
    header("HOW TO PLAY", accent(s->game), "BACK", NULL, pressed, APP_ACTION_BACK);
    text_at(s_titles[s->game], 20, 72, TEXT_MEDIUM, accent(s->game));
    for (int i = 0; i < 12 && rules[s->game][i]; ++i) {
        text_at(rules[s->game][i], 20, 112 + i * 28, TEXT_SMALL, C_TEXT);
    }
}

/* ---- Order & Chaos ---- */

static void draw_order_chaos(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    bool winning[OC_CELLS] = {false};
    if (s->oc_result == OC_RESULT_ORDER) {
        for (int k = 0; k < OC_RUN; ++k) {
            winning[s->oc_line[k]] = true;
        }
    }
    for (int cell = 0; cell < OC_CELLS; ++cell) {
        const Rectangle r = ui_oc_cell(cell);
        const Rectangle inner = ui_rect(r.x + 2, r.y + 2, r.width - 4, r.height - 4);
        Color fill = winning[cell] ? COLOR(255, 226, 214) : C_SURFACE;
        if (is_pressed(pressed, APP_ACTION_CELL, cell)) {
            fill = C_BORDER;
        }
        if (cell == s->last_cell && !winning[cell]) {
            outlined_panel(inner, fill, C_MUTED, 2, 0.25f);
        } else {
            panel(inner, fill, 0.25f);
        }
        draw_mark(s->oc.cells[cell], inner, 0.3f, fill);
    }
    const bool order = s->side_to_move == 0;
    text_center(order ? "ORDER" : "CHAOS", 438, 92, TEXT_SMALL, order ? C_ORDER : C_UTTT);
    text_center(s->computer_thinking ? "THINKS" : "TURN", 438, 116, TEXT_SMALL, C_MUTED);
    for (int mark = OC_X; mark <= OC_O; ++mark) {
        const Rectangle r = ui_oc_mark_button(mark);
        const bool on = s->oc_mark == mark;
        const Color fill = is_pressed(pressed, APP_ACTION_OC_PICK_MARK, mark) ? C_BG : C_SURFACE;
        outlined_panel(r, fill, on ? (mark == OC_X ? C_X : C_O) : C_BORDER, on ? 4 : 1, 0.25f);
        draw_mark(mark, ui_rect(r.x, r.y + 16, r.width, r.width), 0.28f, fill);
    }
    text_center("PLACE", 438, 400, TEXT_SMALL, C_MUTED);
    text_center("MARK", 438, 422, TEXT_SMALL, C_MUTED);
}

/* ---- Ultimate Tic-Tac-Toe ---- */

static void grid_lines(Rectangle r, float inset, float thick)
{
    const float step = r.width / 3;
    for (int k = 1; k < 3; ++k) {
        DrawRectangle((int)(r.x + k * step - thick / 2), (int)(r.y + inset), (int)thick, (int)(r.height - 2 * inset),
                      C_BORDER);
        DrawRectangle((int)(r.x + inset), (int)(r.y + k * step - thick / 2), (int)(r.width - 2 * inset), (int)thick,
                      C_BORDER);
    }
}

static void draw_small_board(const app_game_snapshot_t *s, int board, const ui_target_t *pressed)
{
    const Rectangle r = ui_ut_small_board(board);
    const uint8_t decided = s->ut.small[board];
    const bool playable = ut_board_playable(&s->ut, board) && !s->computer_thinking;
    if (decided == UT_X || decided == UT_O) {
        const Color fill = decided == UT_X ? COLOR(255, 232, 222) : COLOR(232, 229, 255);
        panel(r, fill, 0.15f);
        draw_mark(decided, r, 0.34f, fill);
        return;
    }
    Color fill = playable ? COLOR(236, 233, 255) : C_SURFACE;
    if (is_pressed(pressed, APP_ACTION_UT_OPEN_BOARD, board)) {
        fill = COLOR(212, 206, 255);
    }
    if (playable) {
        outlined_panel(r, fill, C_UTTT, 4, 0.15f);
    } else {
        panel(r, fill, 0.15f);
    }
    grid_lines(r, 10, 2);
    const float step = r.width / 3;
    for (int cell = 0; cell < UT_CELLS; ++cell) {
        const Rectangle c = ui_rect(r.x + (cell % 3) * step, r.y + (cell / 3) * step, step, step);
        if (board == s->last_board && cell == s->last_cell) {
            DrawRectangleRounded(ui_rect(c.x + 4, c.y + 4, c.width - 8, c.height - 8), 0.3f, ROUND_SEGMENTS,
                                 COLOR(255, 236, 170));
            draw_mark(s->ut.cells[board][cell], c, 0.3f, COLOR(255, 236, 170));
        } else {
            draw_mark(s->ut.cells[board][cell], c, 0.3f, fill);
        }
    }
    if (decided == UT_DRAW) {
        DrawRectangleRounded(r, 0.15f, ROUND_SEGMENTS, Fade(C_BG, 0.6f));
        text_center("DRAW", r.x + r.width / 2, r.y + r.height / 2, TEXT_SMALL, C_MUTED);
    }
}

static void draw_uttt_overview(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    char line[48];
    const char who = s->ut.turn == UT_X ? 'X' : 'O';
    if (s->computer_thinking) {
        snprintf(line, sizeof(line), "COMPUTER %c IS THINKING", who);
    } else if (s->ut.active == UT_ANY_BOARD) {
        snprintf(line, sizeof(line), "%c: TAP ANY OPEN BOARD", who);
    } else {
        snprintf(line, sizeof(line), "%c: TAP THE HIGHLIGHTED BOARD", who);
    }
    banner(line, C_UTTT);
    for (int board = 0; board < UT_BOARDS; ++board) {
        draw_small_board(s, board, pressed);
    }
}

static void draw_uttt_zoom(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    char line[48];
    snprintf(line, sizeof(line), "%c: TAP A CELL", s->ut.turn == UT_X ? 'X' : 'O');
    banner(line, C_UTTT);
    const int board = s->ut_zoom;
    const Rectangle r = ui_rect(UI_UT_ZOOM_X, UI_UT_ZOOM_Y, UI_UT_ZOOM, UI_UT_ZOOM);
    outlined_panel(r, C_SURFACE, C_UTTT, 4, 0.08f);
    grid_lines(r, 14, 3);
    for (int cell = 0; cell < UT_CELLS; ++cell) {
        const Rectangle c = ui_ut_zoom_cell(cell);
        Color fill = C_SURFACE;
        if (is_pressed(pressed, APP_ACTION_CELL, cell)) {
            fill = COLOR(236, 233, 255);
            DrawRectangleRounded(ui_rect(c.x + 10, c.y + 10, c.width - 20, c.height - 20), 0.2f, ROUND_SEGMENTS, fill);
        }
        draw_mark(s->ut.cells[board][cell], c, 0.3f, fill);
    }
    /* Mini map: where this board sits in the big grid. */
    text_at("BOARD", 378, 128, TEXT_SMALL, C_MUTED);
    for (int i = 0; i < UT_BOARDS; ++i) {
        const Color fill = i == board ? C_UTTT : (s->ut.small[i] == UT_X ? C_X : (s->ut.small[i] == UT_O ? C_O : C_BORDER));
        DrawRectangle(380 + (i % 3) * 28, 150 + (i / 3) * 28, 24, 24, i == board ? fill : Fade(fill, 0.45f));
    }
    static const char *const hint[] = {"YOUR", "CELL", "PICKS", "THE", "NEXT", "BOARD"};
    for (int i = 0; i < 6; ++i) {
        text_at(hint[i], 380, 252 + i * 24, TEXT_SMALL, C_MUTED);
    }
}

/* ---- Dandelions ---- */

static void draw_flower(float cx, float cy, float size)
{
    const float offset = size * 0.32f, petal = size * 0.3f;
    DrawCircle((int)cx, (int)(cy - offset), petal, C_PETAL);
    DrawCircle((int)(cx + offset), (int)cy, petal, C_PETAL);
    DrawCircle((int)cx, (int)(cy + offset), petal, C_PETAL);
    DrawCircle((int)(cx - offset), (int)cy, petal, C_PETAL);
    DrawCircle((int)cx, (int)cy, size * 0.22f, C_PETAL_CORE);
}

static void draw_seed(float cx, float cy)
{
    DrawLineEx((Vector2) {
        cx, cy - 16
    }, (Vector2) {
        cx, cy
    }, 2, C_DAND);
    DrawCircle((int)cx, (int)cy - 16, 3, C_DAND);
    DrawCircle((int)cx, (int)cy + 2, 6, C_DAND);
}

static void draw_dandelions(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    static const char *const dir_names[DA_DIRECTIONS] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    const bool flowers = s->da.turn == DA_ROLE_FLOWERS;
    if (s->computer_thinking) {
        banner(flowers ? "COMPUTER FLOWERS THINKING" : "COMPUTER WIND THINKING", C_DAND);
    } else {
        banner(flowers ? "FLOWERS: TAP A CELL" : "WIND: PICK A DIRECTION", C_DAND);
    }
    for (int cell = 0; cell < DA_CELLS; ++cell) {
        const Rectangle r = ui_da_cell(cell);
        const Rectangle inner = ui_rect(r.x + 2, r.y + 2, r.width - 4, r.height - 4);
        Color fill = s->da.cells[cell] == DA_SEED ? COLOR(226, 247, 234) : C_SURFACE;
        if (is_pressed(pressed, APP_ACTION_CELL, cell)) {
            fill = C_BORDER;
        }
        panel(inner, fill, 0.25f);
        const float cx = r.x + r.width / 2, cy = r.y + r.height / 2;
        if (s->da.cells[cell] == DA_FLOWER) {
            draw_flower(cx, cy, r.width * 0.72f);
        } else if (s->da.cells[cell] == DA_SEED) {
            draw_seed(cx, cy + 4);
        }
    }
    char moves[40];
    snprintf(moves, sizeof(moves), "FLOWERS %u - WIND %u LEFT", s->da.flowers_left, s->da.wind_left);
    text_center(moves, 167, 438, TEXT_SMALL, C_MUTED);
    text_center("WIND", 403, 134, TEXT_SMALL, flowers ? C_MUTED : C_DAND);
    for (int slot = 0; slot < 9; ++slot) {
        const int d = ui_da_slot_direction(slot);
        if (d < 0) {
            DrawCircle(403, UI_DA_DIR_Y + UI_DA_DIR + UI_DA_DIR_GAP + UI_DA_DIR / 2, 3, C_MUTED);
            continue;
        }
        const Rectangle r = ui_da_direction(d);
        const bool used = da_direction_used(&s->da, (da_direction_t)d);
        const bool live = !flowers && !used && !s->computer_thinking;
        if (used) {
            panel(r, C_BORDER, 0.3f);
        } else {
            outlined_panel(r, is_pressed(pressed, APP_ACTION_DA_WIND, d) ? COLOR(226, 247, 234) : C_SURFACE,
                           live ? C_DAND : C_BORDER, 2, 0.3f);
        }
        text_center(dir_names[d], r.x + r.width / 2, r.y + r.height / 2, TEXT_SMALL, used ? C_MUTED : C_TEXT);
    }
}

/* ---- Result card ---- */

static void result_text(const app_game_snapshot_t *s, const char **title, const char **detail)
{
    if (s->game == APP_GAME_ORDER_CHAOS) {
        const bool order = s->oc_result == OC_RESULT_ORDER;
        *title = order ? "ORDER WINS" : "CHAOS WINS";
        *detail = order ? "5 IN A ROW" : "BOARD FULL - NO 5 IN A ROW";
    } else if (s->game == APP_GAME_UTTT) {
        *title = s->ut.winner == UT_X ? "X WINS" : (s->ut.winner == UT_O ? "O WINS" : "DRAW");
        *detail = s->ut.winner == UT_DRAW ? "NO WINNER THIS TIME" : "3 BOARDS IN A ROW";
    } else {
        const bool flowers = s->da.result == DA_RESULT_FLOWERS;
        *title = flowers ? "FLOWERS WIN" : "WIND WINS";
        *detail = flowers ? "EVERY CELL IS COVERED" : "SOME CELLS STAY EMPTY";
    }
}

static void draw_result(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    const char *title, *detail;
    result_text(s, &title, &detail);
    DrawRectangle(0, 0, 480, 480, Fade(C_TEXT, 0.35f));
    const Rectangle card = ui_rect(40, 150, 400, 210);
    panel(card, C_SURFACE, 0.12f);
    text_center(title, 240, 196, TEXT_LARGE, accent(s->game));
    text_center(detail, 240, 246, TEXT_SMALL, C_MUTED);
    const Rectangle menu = ui_result_menu(), again = ui_result_again();
    outlined_panel(menu, is_pressed(pressed, APP_ACTION_BACK, 0) ? C_BORDER : C_SURFACE, C_BORDER, 2, 0.3f);
    text_center("MENU", menu.x + menu.width / 2, menu.y + menu.height / 2, TEXT_SMALL, C_TEXT);
    panel(again, is_pressed(pressed, APP_ACTION_START, 0) ? C_TEXT : accent(s->game), 0.3f);
    text_center("PLAY AGAIN", again.x + again.width / 2, again.y + again.height / 2, TEXT_SMALL, C_SURFACE);
}

static void draw_play(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    static const char *const zoom_titles[UT_BOARDS] = {
        "TOP LEFT", "TOP", "TOP RIGHT", "LEFT", "CENTRE", "RIGHT", "BOTTOM LEFT", "BOTTOM", "BOTTOM RIGHT",
    };
    const bool zoomed = s->game == APP_GAME_UTTT && s->ut_zoom >= 0;
    const ui_target_t *live = s->game_over ? NULL : pressed;
    header(zoomed ? zoom_titles[s->ut_zoom] : s_titles[s->game], accent(s->game), zoomed ? "ALL" : "MENU", "NEW", live,
           APP_ACTION_START);
    if (s->game == APP_GAME_ORDER_CHAOS) {
        draw_order_chaos(s, live);
    } else if (s->game == APP_GAME_UTTT) {
        if (zoomed) {
            draw_uttt_zoom(s, live);
        } else {
            draw_uttt_overview(s, live);
        }
    } else {
        draw_dandelions(s, live);
    }
    if (s->game_over) {
        draw_result(s, pressed);
    }
}

void ui_draw(const app_game_snapshot_t *s, const ui_target_t *pressed)
{
    ClearBackground(C_BG);
    switch (s->screen) {
    case APP_SCREEN_HOME: draw_home(pressed); break;
    case APP_SCREEN_SETUP: draw_setup(s, pressed); break;
    case APP_SCREEN_RULES: draw_rules(s, pressed); break;
    default: draw_play(s, pressed); break;
    }
}
