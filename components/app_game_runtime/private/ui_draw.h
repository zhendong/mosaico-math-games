// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "ui_layout.h"

/** Draw the whole frame for state; pressed highlights the touched target. */
void ui_draw(const app_game_snapshot_t *state, const ui_target_t *pressed);
