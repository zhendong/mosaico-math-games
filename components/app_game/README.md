# app_game

Owns the Math Games model: screen flow (Home, Setup, Rules, Play), setup choices,
turn order, computer pacing, and the rules of three games from Ben Orlin's
*Math Games with Bad Drawings*. It is pure C with no Engine, RTOS, display or other
application dependency. The private instance is defined only in `app_game.c`;
`include/app_game.h` exports an opaque handle, semantic actions and value snapshots.

| File | Responsibility |
| --- | --- |
| `app_game.c` | Screens, actions, turn flags, computer move delay |
| `rules_order_chaos.c` | 6x6 board, exactly-five check, computer player |
| `rules_uttt.c` | Nine boards, send-to-board rule, computer player |
| `rules_dandelions.c` | 5x5 board, wind rays, seven turns each, computer player |
| `rules_rng.c` | Deterministic xorshift32 used by Easy computer players |
| `include/game_rules.h` | Pure rule API shared by the model and tests |

```c
app_game_handle_t game = NULL;
const app_game_config_t config = {480, 480}; /* pixels */
if (app_game_create(&config, &game) == APP_GAME_OK) {
    app_game_dispatch(game, APP_ACTION_OPEN_GAME, APP_GAME_UTTT);
    app_game_dispatch(game, APP_ACTION_START, 0);
    app_game_dispatch(game, APP_ACTION_UT_OPEN_BOARD, 4); /* zoom into the centre board */
    app_game_dispatch(game, APP_ACTION_CELL, 2);          /* place X; back to the overview */
    app_game_update(game); /* one fixed tick, not elapsed milliseconds */
    app_game_snapshot_t snapshot = app_game_read(game);
    (void)snapshot;
    app_game_destroy(&game);
}
```

## Actions

`app_game_dispatch` returns false and changes nothing when an action does not apply
to the current screen or turn, including while paused, while the computer is to move,
or after the game ends (only `START` and `BACK` work then).

- Home: `OPEN_GAME(kind)` opens that game's Setup.
- Setup: `SET_VS_COMPUTER(0/1)`, `SET_SIDE(0/1)`, `SET_DIFFICULTY(level)`,
  `SHOW_RULES`, `START`, `BACK` (to Home). Side 0 is Order, X or Flowers; side 1
  is Chaos, O or Wind. Side and difficulty matter only against the computer.
- Rules: `BACK` returns to Setup.
- Play: `START` restarts with the same setup; `BACK` leaves a zoomed Ultimate TTT
  board for the overview, otherwise returns to Setup.
  - Order & Chaos: `OC_PICK_MARK(OC_X/OC_O)` chooses the mark, `CELL(0..35)` places it.
  - Ultimate TTT: `UT_OPEN_BOARD(0..8)` zooms into a playable board, `CELL(0..8)`
    plays in the zoomed board and returns to the overview.
  - Dandelions: `CELL(0..24)` plants on the flowers' turn, `DA_WIND(direction)` blows.

When the computer is to move, `computer_thinking` is set and the move is made by
`app_game_update` after 15 ticks (0.5 s at 30 Hz). The Order & Chaos Hard search runs
synchronously inside that tick.

## Computer players

The rules and computer players port the reference web games move for move,
including their tie-breaking order. Easy picks random legal moves. Medium uses each
game's heuristic. Hard is a depth-2 search for Order & Chaos (inner nodes try only
the first 12 empty cells, as in the reference). In Ultimate TTT and Dandelions, Hard
plays like Medium, because the reference games do the same.

## Ownership

The creator owns the handle and serializes every call on one loop. Config is
copied, snapshots contain no owned pointers, destroy clears the owner handle.
Calls are synchronous, with no locks, waits or deadlines; only create/destroy use
the allocator. Other operations perform bounded in-memory work. Creation reports
invalid arguments or allocation failure; harmless NULL behavior for other
operations is documented in the public header. Ticks wrap modulo 2^32; viewport
coordinates are pixels and are clamped.

Rendering reads snapshots and must not reach into the private struct. The rules and
flow are tested with a native C compiler in `../../tests/`.
