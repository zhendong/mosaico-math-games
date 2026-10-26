// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "game_rules.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque model owned by one loop; no concurrent or ISR access.
 * Lifecycle, actions and timing contracts: ../README.md. */
typedef struct app_game *app_game_handle_t;

typedef enum {
    APP_GAME_OK = 0,
    APP_GAME_INVALID_ARGUMENT,
    APP_GAME_NO_MEMORY,
} app_game_err_t;

typedef struct {
    uint16_t width;    /**< Viewport width in pixels, 1..65535. */
    uint16_t height;   /**< Viewport height in pixels, 1..65535. */
} app_game_config_t;

typedef enum { APP_SCREEN_HOME = 0, APP_SCREEN_SETUP, APP_SCREEN_RULES, APP_SCREEN_PLAY } app_screen_t;
typedef enum { APP_GAME_ORDER_CHAOS = 0, APP_GAME_UTTT, APP_GAME_DANDELIONS, APP_GAME_KIND_COUNT } app_game_kind_t;

/** Semantic user actions; the runtime maps touches to these. */
typedef enum {
    APP_ACTION_OPEN_GAME = 0,  /**< arg: app_game_kind_t, from Home. */
    APP_ACTION_SET_VS_COMPUTER, /**< arg: 0 two players, 1 vs computer. */
    APP_ACTION_SET_SIDE,       /**< arg: 0 first side, 1 second side (see README). */
    APP_ACTION_SET_DIFFICULTY, /**< arg: game_difficulty_t. */
    APP_ACTION_SHOW_RULES,
    APP_ACTION_START,          /**< Start from Setup, or restart while playing. */
    APP_ACTION_BACK,           /**< Rules->Setup, Setup->Home, Play->Setup, zoomed UTTT->overview. */
    APP_ACTION_OC_PICK_MARK,   /**< arg: OC_X or OC_O. */
    APP_ACTION_CELL,           /**< arg: cell of the current board or zoomed UTTT board. */
    APP_ACTION_UT_OPEN_BOARD,  /**< arg: UTTT board 0..8 to zoom into. */
    APP_ACTION_DA_WIND,        /**< arg: da_direction_t. */
} app_action_t;

typedef struct {
    bool vs_computer;
    uint8_t human_side;            /**< 0 first side (Order/X/Flowers), 1 second side. */
    uint8_t difficulty;            /**< game_difficulty_t. */
} app_setup_t;

/** Complete value snapshot for rendering and automation. */
typedef struct {
    uint32_t tick;                 /**< Update count, wraps modulo 2^32; not milliseconds. */
    int32_t pointer_x;             /**< Last pointer position in pixels. */
    int32_t pointer_y;
    bool pointer_down;
    bool paused;
    uint8_t screen;                /**< app_screen_t. */
    uint8_t game;                  /**< app_game_kind_t selected on Home. */
    app_setup_t setup;
    bool game_over;                /**< Result card is shown. */
    bool computer_thinking;        /**< Computer is to move; input is ignored. */
    uint8_t side_to_move;          /**< 0 first side, 1 second side. */
    int8_t last_cell;              /**< Last placed cell, or -1. */
    int8_t last_board;             /**< UTTT board of last_cell, or -1. */
    /* Order & Chaos */
    oc_board_t oc;
    uint8_t oc_mark;               /**< Mark the human will place: OC_X or OC_O. */
    uint8_t oc_result;             /**< oc_result_t. */
    uint8_t oc_line[OC_RUN];       /**< Winning cells when oc_result is Order. */
    /* Ultimate Tic-Tac-Toe */
    ut_state_t ut;
    int8_t ut_zoom;                /**< Board shown full size, or -1 for the overview. */
    /* Dandelions */
    da_state_t da;
} app_game_snapshot_t;

/** Copy config and create a caller-owned model; arguments must be non-NULL.
 * Width/height must be positive; clears *out_handle on failure.
 * Returns APP_GAME_OK, APP_GAME_INVALID_ARGUMENT or APP_GAME_NO_MEMORY. */
app_game_err_t app_game_create(const app_game_config_t *config, app_game_handle_t *out_handle);

/** Free the model and clear the handle. NULL pointer/handle is a no-op. */
void app_game_destroy(app_game_handle_t *handle);

/** Return to Home with default setup and zero tick; keep viewport. NULL is a no-op. */
void app_game_reset(app_game_handle_t handle);
/** Change pause state. NULL is a no-op. Pausing stops ticks and computer moves. */
void app_game_set_paused(app_game_handle_t handle, bool paused);
/** Record pointer coordinates (clamped to viewport) for automation. NULL is a no-op. */
void app_game_set_pointer(app_game_handle_t handle, int32_t x, int32_t y, bool pressed);
/** Apply a user action; returns false when it does not apply to the current state. */
bool app_game_dispatch(app_game_handle_t handle, app_action_t action, int arg);
/** Advance one fixed tick unless paused; computer moves run here after a short delay.
 * NULL handle is a no-op; no rendering or hardware access. */
void app_game_update(app_game_handle_t handle);
/** Return a value snapshot on the owning loop; NULL returns all zeros. */
app_game_snapshot_t app_game_read(app_game_handle_t handle);

#ifdef __cplusplus
}
#endif
