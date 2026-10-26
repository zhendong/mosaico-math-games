// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include "include/app_game_runtime.h"
/* Engine Host accepts source lists, not component include paths. Reference only
 * the model's public header so these translation units work on both targets. */
#include "../app_game/include/app_game.h"
#include "private/game_config.h"
#include "private/ui_draw.h"
#include "raylib_lite_raylib.h"

typedef struct {
    app_game_handle_t game;
    ui_target_t pressed;   /* Target under the finger since touch-down, if any. */
    bool pointer_down;
} host_state_t;

static int initialize(void *value, const char *asset_root)
{
    (void)asset_root;
    host_state_t *state = value;
    const app_game_config_t config = {GAME_WIDTH, GAME_HEIGHT};
    if (app_game_create(&config, &state->game) != APP_GAME_OK) {
        return -1;
    }
    InitWindow(GAME_WIDTH, GAME_HEIGHT, GAME_NAME);
    SetTargetFPS(GAME_TICK_HZ);
    return 0;
}

static void shutdown(void *value)
{
    host_state_t *state = value;
    app_game_destroy(&state->game);
}

static bool same_target(ui_target_t a, ui_target_t b)
{
    return a.valid && b.valid && a.action == b.action && a.arg == b.arg;
}

/* A tap fires on release over the same target it started on, so a finger that
 * slides off a small cell cancels instead of placing a mark by mistake. */
static void pointer(host_state_t *state, int32_t x, int32_t y, bool pressed)
{
    app_game_set_pointer(state->game, x, y, pressed);
    const app_game_snapshot_t snapshot = app_game_read(state->game);
    const ui_target_t under = ui_hit_test(&snapshot, (int)x, (int)y);
    if (pressed && !state->pointer_down) {
        state->pressed = under;
    } else if (pressed && !same_target(state->pressed, under)) {
        state->pressed = (ui_target_t) {
            0
        };
    } else if (!pressed && state->pointer_down && same_target(state->pressed, under)) {
        app_game_dispatch(state->game, under.action, under.arg);
    }
    if (!pressed) {
        state->pressed = (ui_target_t) {
            0
        };
    }
    state->pointer_down = pressed;
}

static void input(void *value, const raylib_lite_game_input_v1_t *event)
{
    host_state_t *state = value;
    if (!event) {
        return;
    }
    if (event->type == RAYLIB_LITE_GAME_INPUT_POINTER && event->track_id == 0) {
        pointer(state, event->x, event->y, event->pressed);
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_CONTROL) {
        switch (event->code) {
        case RAYLIB_LITE_GAME_CONTROL_PAUSE: app_game_set_paused(state->game, true); break;
        case RAYLIB_LITE_GAME_CONTROL_RESUME: app_game_set_paused(state->game, false); break;
        case RAYLIB_LITE_GAME_CONTROL_RESET: app_game_reset(state->game); break;
        default: break;
        }
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_ACTION && event->pressed) {
        if (event->code == GAME_ACTION_RESET) {
            app_game_reset(state->game);
        }
        if (event->code == GAME_ACTION_PAUSE) {
            app_game_set_paused(state->game, !app_game_read(state->game).paused);
        }
    }
}

static void update(void *value)
{
    app_game_update(((host_state_t *)value)->game);
}

static int render(void *value)
{
    host_state_t *state = value;
    if (!state->game) {
        return -1;
    }
    BeginDrawing();
    if (!raylib_lite_raylib_frame_available()) {
        EndDrawing();
        return -1;
    }
    const app_game_snapshot_t snapshot = app_game_read(state->game);
    ui_draw(&snapshot, &state->pressed);
    EndDrawing();
    return 0;
}

static uint32_t hash_bytes(uint32_t hash, const uint8_t *bytes, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        hash = hash * 31U + bytes[i];
    }
    return hash;
}

static uint32_t state_hash(const void *value)
{
    const app_game_snapshot_t s = app_game_read(((const host_state_t *)value)->game);
    /* Hash named fields and byte arrays, avoiding platform-dependent struct padding. */
    uint32_t hash = s.tick;
    hash = hash * 31U + (uint32_t)s.pointer_x;
    hash = hash * 31U + (uint32_t)s.pointer_y;
    hash = hash * 31U + s.pointer_down;
    hash = hash * 31U + s.paused;
    const uint8_t fields[] = {
        s.screen, s.game, s.setup.vs_computer, s.setup.human_side, s.setup.difficulty, s.game_over,
        s.side_to_move, s.oc_mark, s.oc_result, (uint8_t)s.ut_zoom, (uint8_t)s.ut.active, s.ut.turn,
        s.ut.winner, s.da.used_directions, s.da.turn, s.da.result,
    };
    hash = hash_bytes(hash, fields, sizeof(fields));
    hash = hash_bytes(hash, s.oc.cells, sizeof(s.oc.cells));
    hash = hash_bytes(hash, &s.ut.cells[0][0], sizeof(s.ut.cells));
    return hash_bytes(hash, s.da.cells, sizeof(s.da.cells));
}

static void encode_cells(char *out, const uint8_t *cells, size_t count, const char *symbols)
{
    for (size_t i = 0; i < count; ++i) {
        out[i] = symbols[cells[i] & 3U];
    }
    out[count] = '\0';
}

static int state_json(const void *value, char *output, size_t capacity)
{
    static const char *const screens[] = {"home", "setup", "rules", "play"};
    static const char *const games[] = {"order_chaos", "uttt", "dandelions"};
    if (!output || !capacity) {
        return -1;
    }
    const app_game_snapshot_t s = app_game_read(((const host_state_t *)value)->game);
    char oc[OC_CELLS + 1], ut[UT_BOARDS * UT_CELLS + 1], small[UT_BOARDS + 1], da[DA_CELLS + 1];
    encode_cells(oc, s.oc.cells, OC_CELLS, ".XO");
    encode_cells(ut, &s.ut.cells[0][0], UT_BOARDS * UT_CELLS, ".XO");
    encode_cells(small, s.ut.small, UT_BOARDS, ".XOD");
    encode_cells(da, s.da.cells, DA_CELLS, ".FS");
    int size = snprintf(output, capacity,
                        "{\"phase\":\"%s\",\"tick\":%lu,\"pointer_x\":%ld,\"pointer_y\":%ld,"
                        "\"pointer_down\":%s,\"screen\":\"%s\",\"game\":\"%s\",\"vs_computer\":%s,"
                        "\"human_side\":%u,\"difficulty\":%u,\"side_to_move\":%u,\"game_over\":%s,"
                        "\"computer_thinking\":%s,\"oc\":\"%s\",\"oc_mark\":%u,\"oc_result\":%u,"
                        "\"ut\":\"%s\",\"ut_small\":\"%s\",\"ut_active\":%d,\"ut_zoom\":%d,\"ut_winner\":%u,"
                        "\"da\":\"%s\",\"da_used\":%u,\"da_result\":%u,\"state_hash\":\"%08lx\"}",
                        s.paused ? "paused" : "running", (unsigned long)s.tick, (long)s.pointer_x,
                        (long)s.pointer_y, s.pointer_down ? "true" : "false", screens[s.screen & 3U],
                        games[s.game % APP_GAME_KIND_COUNT], s.setup.vs_computer ? "true" : "false",
                        s.setup.human_side, s.setup.difficulty, s.side_to_move, s.game_over ? "true" : "false",
                        s.computer_thinking ? "true" : "false", oc, s.oc_mark, s.oc_result, ut, small,
                        s.ut.active, s.ut_zoom, s.ut.winner, da, s.da.used_directions, s.da.result,
                        (unsigned long)state_hash(value));
    return size < 0 || (size_t)size >= capacity ? -1 : size;
}

static const raylib_lite_game_module_v1_t s_module = {
    .descriptor = {
        RAYLIB_LITE_GAME_MODULE_ABI_V1, GAME_NAME, GAME_TITLE,
        GAME_WIDTH, GAME_HEIGHT, GAME_TICK_HZ, 1
    },
    .state_size = sizeof(host_state_t),
    .initialize = initialize, .shutdown = shutdown, .input = input,
    .update = update, .render = render,
    .state_hash = state_hash, .state_json = state_json,
};

const raylib_lite_game_module_v1_t *app_game_runtime_module(void)
{
    return &s_module;
}
