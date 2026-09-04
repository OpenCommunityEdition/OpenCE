# `rasterizer_xbox_screen_effect.obj` Opus fuzzy reconciliation (2026-09-03)

## Outcome

This packet reconciles the two substantive ordinary-C implementations from the Opus donor while making no exact-match claim. The focused gate moved the unit from `0 exact / 0 residual / 13 unwritten` to `0 exact / 2 residual / 11 unwritten`.

| Function | Target size | Candidate size | Relocations | Target normalized SHA-256 | Candidate normalized SHA-256 | objdiff |
| --- | ---: | ---: | ---: | --- | --- | ---: |
| `__rasterizer_screen_effect` | 3888 | 3872 | 290 / 290 | `d67a06f0c52e684686aae8a5a8e3a7e5e2bcf6c84e854bd525b54660cc66d7f8` | `fe3e214e0e283e629854f4b448c161f0adc9254aaf2088c339c235ff9b1bca87` | 93.197845% |
| `__rasterizer_screen_flash` | 1344 | 1344 | 118 / 118 | `f1d3e84eebfa355c8aecd1b784f3128fdd6fab0714ccc51b8feafdd988c9f7af` | `74b68c67df01d6b9be5ac42d98f9a6a94025e977476bdb250bf198eeb071fa16` | 93.827360% |

Base: `fb0f40762`. Donor: the uncommitted screen-effect file in `C:\halo-worktrees\opus-small-families-30k-20260902`, whose broad checkpoint was `bb0d690a3`. The pre-run stable snapshot is `scratch/before-screen-effect-20260903.json` (`8245` functions, `5726` exact).

## Authenticity assessment

The January assertions, switch topology, globals, calls, and all relocation identities support both reconstructions. `__rasterizer_screen_effect` implements the cinematic parameter validation, pass count, ping-pong targets, mask/scanline/noise selection, convolution/warp transforms, overbright handling, fan draws, target restoration, and balanced profiling. `__rasterizer_screen_flash` implements all six authenticated `_render_screen_flash_type_*` cases, their color math and blend states, the viewport transform, the four-vertex fan, and balanced profiling.

HaloCEA's later DX9 reconstruction independently corroborates those semantics and the parameter layout, but is treated only as build-divergent evidence. Canonical `effects/player_effects.c` authenticates the flash enum mapping, `hs_globals_external.c` authenticates the debug toggles, and `rasterizer_cinematics.c` authenticates the complete cinematic state and parameter-prefix layout. Compile-time assertions protect all relied-on offsets and sizes.

The two TU-private source helpers have descriptive semantic names and ordinary definitions; neither is forced inline nor emitted as a separate January function/COMDAT. Their natural compiler inlining is part of this translation unit's observed schedule. No unidentified address is promoted into a production identifier.

## Reconciliation work

- Removed donor-local declarations with false signatures and used owning declarations from `rasterizer_xbox.h`, `hud_draw.h`, and related subsystem headers.
- Added `main_get_window_count` to its owner, `main/main.h`, rather than declaring it at its call site.
- Replaced the donor's provisional flat window structure with canonical `rasterizer_window_begin_parameters` access and replaced the incomplete cinematic state with layout-checked TU-private definitions matching the owner.
- Replaced magic render, vertex, cull, convolution, and flash values with semantic constants and authenticated enums; retained project `real` and geometry types.
- Kept natural C89 source, explicit returns, one parameter per line, and declarations near initialization. No raw address names, inline assembly, volatile/register steering, barriers, pragmas, representation punning, fake dependencies, undefined behavior, or nonsensical source forms remain.

## Parking decision

Both functions are honest high-similarity residuals. The screen-effect residual is distributed compiler scheduling/register/x87 selection plus one 16-byte padded-size slot; the flash residual has an exact padded envelope but similarly distributed instruction selection. Neither shows a missing semantic call, branch, or relocation. Under the fuzzy-match house rule they are recorded as `unclassified` in `config/parked.json`; no compiler-tie grind was attempted.

## Verification

The focused gate reports `0 exact / 2 residual / 11 unwritten`. `ninja all_source` and report generation pass, and the semantic report scans 473 units with 6,062 functions evaluated, 5,735 semantic exact, 158 hidden exact (95,170 bytes), 5,752 accepted exact, and zero unit errors. The stable snapshot remains `8245` functions / `5726` exact; its diff reports `0` gained and `0` regressions. The parked audit reports `181 active / 0 stale / 0 invalid`, the fake-match scan reports zero review leads, `git diff --check` passes, and pytest reports `261 passed`.

The combined `ninja all_source progress semantic_progress` invocation completed the object build and semantic report but the `progress` subcommand stopped on the pre-existing semantic-data record `source/shell/shell_xbox:_rdata_002a0a88` because its configured owner `$T18267` was absent. The admission audit reaches the same unrelated fail-closed check. This packet does not alter `shell_xbox`, semantic-data configuration, or that owner; the stable per-function sweep above is the applicable regression gate.

Exact-code progress therefore remains unchanged; this packet converts two previously unwritten functions into reviewed fuzzy reconstructions without weakening any inherited exact function.
