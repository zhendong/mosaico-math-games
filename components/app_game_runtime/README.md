# app_game_runtime

Composes `app_game` with the Engine's native/Host module ABI, input translation,
rendering and diagnostic JSON. The same implementation runs on both platforms.
It depends publicly on `examples_common` for the Engine ABI type, privately on
`app_game` and Raylib Lite. The model never depends back on this adapter.

```c
const raylib_lite_game_module_v1_t *module = app_game_runtime_module();
/* The Engine allocates module->state_size and drives the documented lifecycle. */
```

The getter returns an immutable borrowed descriptor without errors or timeouts.
The Engine allocates zeroed `state_size` bytes per instance and retains them through
shutdown. That storage holds one owned opaque model handle. Shutdown releases it,
including after partial initialization. Callbacks run serially on the owning
loop, never from an ISR or after shutdown. State/input remain valid for each call.

`initialize` borrows nullable `asset_root` for the call (this template has no assets),
returning 0 on success or -1 on model allocation/configuration failure. `render`
returns 0 on a submitted frame, -1 if unavailable. `state_json` borrows output;
capacity counts bytes including NUL, while its result excludes NUL. It returns -1
for NULL output, zero capacity or truncation. Other results follow Engine ABI v1.

Callbacks do not wait on worker tasks. Initialization and rendering use Engine
scheduling with no additional deadline. Its single drawing context permits one
active renderer; model instances remain isolated.

Project identity, viewport pixels and tick frequency (Hz) live in
`private/game_config.h`; this directory is never exported to another component.
Keep gameplay in `app_game`, platform translation and drawing here.

| File | Responsibility |
| --- | --- |
| `app_game_runtime.c` | Engine callbacks, tap detection, state hash and JSON |
| `ui_layout.c` | Every on-screen rectangle and touch hit-testing |
| `ui_draw.c` | Drawing of each screen from a model snapshot |

Drawing and hit-testing share the rectangles in `ui_layout.c`, so what is drawn is
what is tapped. A tap fires on release over the target where the touch started;
sliding off cancels it, which protects the small Ultimate TTT overview. The pressed
target is highlighted while the finger is down. The Engine font has capitals,
digits and `/ - : .` only, so on-screen copy avoids other punctuation and arrows
are drawn as shapes.

`state_json` adds `screen`, `game`, setup fields, `side_to_move`, `game_over`,
`computer_thinking` and one-character-per-cell boards: `oc` (`.XO`), `ut`
(81 cells, `.XO`), `ut_small` (`.XOD`), `da` (`.FS` for empty, flower and seed).

The current Engine Host runner accepts source lists but cannot declare component
include directories. Source files therefore reference application **public**
headers by relative path, while firmware also declares explicit CMake dependencies.
No private headers cross component boundaries and no third-party runner is patched.
