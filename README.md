# math_games

Three games from Ben Orlin's *Math Games with Bad Drawings*, ported from
<https://cathy.light-pole.top/math-games> for the 480x480 touch screen:

- **Order & Chaos**: 6x6. Each turn place an X or an O. Order wins with exactly five
  in a row; Chaos wins if the board fills.
- **Ultimate Tic-Tac-Toe**: nine boards in one. Tap a board on the overview to open it
  full size, tap a cell, and the game returns to the overview with the next board
  highlighted. Your cell picks the board your opponent plays next.
- **Dandelions**: 5x5. Flowers plant, Wind blows seeds along a line from every flower.
  Seven turns each, each wind direction once. Flowers win if every cell is covered.

Each game offers two players on one screen or a computer opponent (Easy, Medium,
Hard). Built on the Raylib Lite blank template; no atlas, sounds or resource partition.

From the workspace root:

```sh
python mosaico.py game sim --project projects/math_games
python mosaico.py game sim --project projects/math_games --headless --frames 120
python mosaico.py game build --project projects/math_games
python mosaico.py iris system-update --project projects/math_games
```

Run `python mosaico.py recover` before first installation on a blank or unverified
device. New applications, changed layouts or external resources use
`iris system-update`. The immutable Vibe Mode partitions and OTA routing are
already configured; the engine starts Iris and accepts the image after its first
frame. USB High-Speed remains owned by Iris. Physical input/display behavior
still requires device validation after Host checks.

- `components/app_game`: pure C screen flow, rules and computer players.
- `components/app_game_runtime`: Engine input/lifecycle adapter, layout, drawing and JSON.
- `tests/`: native C tests for rules and flow, and `make_replays.py`, which writes
  headless simulator replays to `tests/replays/`.
- `components/app_game_runtime/private/game_config.h`: identity, pixel dimensions
  and tick frequency; never included by another component.
- `main/game_entry.c`: selects the adapter for the unchanged Engine module ABI.
- `game.sim.json`: explicit list of the same sources used by the Host module.

Dependencies flow `main -> app_game_runtime -> app_game`, with the Engine as a
platform dependency of the runtime. The model never depends on its caller or on
rendering. Public headers live under each component's `include/`; internal state
and configuration stay private. Component READMEs document contracts and usage.

Keep new application components cohesive and prefixed `app_`; keep dependencies
acyclic and entry points limited to composition/startup. Exchange handles and
value snapshots, never extern business globals or private structures. Keep public
API comments within three lines; put detailed contracts in the component README
and document units/ranges beside fields. Add an
abstraction only when a concrete independent responsibility requires it.
Third-party code retains upstream names. Pins and workspace overrides live in
`main/idf_component.yml` and the root CMake configuration. Apply local patches in
a dedicated dependency checkout selected by `override_path`; never edit
`managed_components/` in place. The prefix hook checks naming;
code review and behavior tests establish architectural and API correctness.

Git, editor and hook entry files stay at the root; format rules, project policy
and hook setup instructions live in `tools/quality/`.

Host state exposes `phase`, `tick`, pointer fields, `state_hash`, the screen and
every board (see `components/app_game_runtime/README.md`). Pause/resume/reset are
wired for Host controls and replay; there are no gameplay keyboard bindings.

Native tests, from this directory:

```sh
cc -std=c11 -Wall -Wextra -Werror -I components/app_game/include \
  tests/test_rules.c components/app_game/*.c -o /tmp/test_rules && /tmp/test_rules
cc -std=c11 -Wall -Wextra -Werror -I components/app_game/include \
  tests/test_app_flow.c components/app_game/*.c -o /tmp/test_flow && /tmp/test_flow
```

Host needs a C compiler and Pillow, and uses the same C renderer as firmware.
Firmware needs the pinned workspace ESP-IDF and initialized BSP, engine and utils.
Before using any other Raylib API, check the engine's supported public headers.
