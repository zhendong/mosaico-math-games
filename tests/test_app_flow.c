// SPDX-License-Identifier: Apache-2.0
/* Native flow tests; build commands are in ../README.md. */
#include <stdio.h>
#include "app_game.h"

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); ++s_failures; } } while (0)

static app_game_handle_t open_game(app_game_kind_t kind)
{
    const app_game_config_t config = {480, 480};
    app_game_handle_t game = NULL;
    CHECK(app_game_create(&config, &game) == APP_GAME_OK);
    CHECK(app_game_dispatch(game, APP_ACTION_OPEN_GAME, kind));
    CHECK(app_game_read(game).screen == APP_SCREEN_SETUP);
    return game;
}

static void run_ticks(app_game_handle_t game, int ticks)
{
    for (int i = 0; i < ticks; ++i) {
        app_game_update(game);
    }
}

static void test_uttt_zoom_flow(void)
{
    app_game_handle_t game = open_game(APP_GAME_UTTT);
    CHECK(app_game_dispatch(game, APP_ACTION_START, 0));
    app_game_snapshot_t s = app_game_read(game);
    CHECK(s.screen == APP_SCREEN_PLAY && s.ut_zoom == -1);
    CHECK(!app_game_dispatch(game, APP_ACTION_CELL, 0));         /* Must open a board first. */
    CHECK(app_game_dispatch(game, APP_ACTION_UT_OPEN_BOARD, 4));
    CHECK(app_game_read(game).ut_zoom == 4);
    CHECK(app_game_dispatch(game, APP_ACTION_BACK, 0));          /* "All" returns to overview. */
    s = app_game_read(game);
    CHECK(s.ut_zoom == -1 && s.screen == APP_SCREEN_PLAY);
    CHECK(app_game_dispatch(game, APP_ACTION_UT_OPEN_BOARD, 4));
    CHECK(app_game_dispatch(game, APP_ACTION_CELL, 2));
    s = app_game_read(game);
    CHECK(s.ut_zoom == -1);                                       /* Back to overview after a move. */
    CHECK(s.ut.cells[4][2] == UT_X && s.ut.active == 2 && s.ut.turn == UT_O);
    CHECK(!app_game_dispatch(game, APP_ACTION_UT_OPEN_BOARD, 5)); /* O is sent to board 2. */
    CHECK(app_game_dispatch(game, APP_ACTION_UT_OPEN_BOARD, 2));
    CHECK(!app_game_dispatch(game, APP_ACTION_CELL, 9));
    app_game_destroy(&game);
    CHECK(game == NULL);
}

static void test_computer_turns(void)
{
    app_game_handle_t game = open_game(APP_GAME_ORDER_CHAOS);
    CHECK(app_game_dispatch(game, APP_ACTION_SET_VS_COMPUTER, 1));
    CHECK(app_game_dispatch(game, APP_ACTION_SET_SIDE, 1));        /* Human is Chaos; computer opens. */
    CHECK(app_game_dispatch(game, APP_ACTION_SET_DIFFICULTY, GAME_DIFFICULTY_HARD));
    CHECK(app_game_dispatch(game, APP_ACTION_START, 0));
    app_game_snapshot_t s = app_game_read(game);
    CHECK(s.computer_thinking);
    CHECK(!app_game_dispatch(game, APP_ACTION_CELL, 0));          /* Input ignored while it thinks. */
    run_ticks(game, 30);
    s = app_game_read(game);
    CHECK(!s.computer_thinking && s.side_to_move == 1 && s.last_cell >= 0);
    int empty = 0;
    while (s.oc.cells[empty] != OC_EMPTY) {
        ++empty;
    }
    CHECK(app_game_dispatch(game, APP_ACTION_OC_PICK_MARK, OC_O));
    CHECK(app_game_dispatch(game, APP_ACTION_CELL, empty));
    CHECK(app_game_read(game).oc.cells[empty] == OC_O);
    CHECK(app_game_read(game).computer_thinking);
    app_game_set_paused(game, true);                               /* Pause holds the computer. */
    run_ticks(game, 60);
    CHECK(app_game_read(game).computer_thinking);
    app_game_set_paused(game, false);
    run_ticks(game, 30);
    CHECK(!app_game_read(game).computer_thinking);
    CHECK(app_game_dispatch(game, APP_ACTION_BACK, 0));
    CHECK(app_game_read(game).screen == APP_SCREEN_SETUP);
    app_game_destroy(&game);
}

static void test_full_games_end(void)
{
    /* Computer against computer by toggling sides each turn is not exposed, so play
     * Dandelions with the human side driven by the same AI the computer uses. */
    for (int level = 0; level < GAME_DIFFICULTY_COUNT; ++level) {
        app_game_handle_t game = open_game(APP_GAME_DANDELIONS);
        CHECK(app_game_dispatch(game, APP_ACTION_SET_VS_COMPUTER, 1));
        CHECK(app_game_dispatch(game, APP_ACTION_SET_DIFFICULTY, level));
        CHECK(app_game_dispatch(game, APP_ACTION_START, 0));
        uint32_t rng = 11;
        for (int step = 0; step < 100 && !app_game_read(game).game_over; ++step) {
            app_game_snapshot_t s = app_game_read(game);
            if (s.computer_thinking) {
                run_ticks(game, 20);
                continue;
            }
            int action = -1;
            CHECK(da_ai_move(&s.da, GAME_DIFFICULTY_EASY, &rng, &action));
            CHECK(app_game_dispatch(game, s.da.turn == DA_ROLE_FLOWERS ? APP_ACTION_CELL : APP_ACTION_DA_WIND, action));
        }
        CHECK(app_game_read(game).game_over);
        CHECK(app_game_dispatch(game, APP_ACTION_START, 0));          /* Play again resets. */
        CHECK(!app_game_read(game).game_over && app_game_read(game).da.flowers_left == DA_TURNS);
        app_game_destroy(&game);
    }
}

int main(void)
{
    test_uttt_zoom_flow();
    test_computer_turns();
    test_full_games_end();
    printf(s_failures ? "%d failure(s)\n" : "all flow tests passed\n", s_failures);
    return s_failures != 0;
}
