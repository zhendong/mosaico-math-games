// SPDX-License-Identifier: Apache-2.0
/* Deterministic random source shared by the easy computer players. */
#include "include/game_rules.h"

enum { RNG_FALLBACK_SEED = 0x9e3779b9U };

uint32_t game_rng_next(uint32_t *state)
{
    uint32_t x = *state ? *state : RNG_FALLBACK_SEED;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}
