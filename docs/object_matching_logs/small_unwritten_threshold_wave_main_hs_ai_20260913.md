# Small unwritten Main, HS, and AI packet (2026-09-13)

## Scope and result

This packet reconstructs three previously unwritten functions with ordinary,
typed source. One function is strict exact and two are retained as documented
fuzzy research baselines with zero exact-match credit.

| unit | function | result | meaningful exact bytes | padded extent |
| --- | --- | --- | ---: | ---: |
| `source/main/main` | `_main_framerate_render` | strict exact | 666 | 672 |
| `source/hs/hs_runtime` | `_render_debug_trigger_volumes` | fuzzy, parked | 0 | 1088 / 1088 |
| `source/ai/ai_script` | `_ai_scripting_allegiance_broken` | fuzzy, parked | 0 | 80 / 64 |

The net credited gain from this packet is therefore **one strict function and
666 meaningful exact code bytes**. The two complete fuzzy bodies remain useful
source and receive no exact credit.

## Main exact closure

`main_framerate_render` is reconstructed from the preserved natural source at
`fd0de0a58`, then checked against the January target. Target evidence fixes the
otherwise easy-to-misread details:

- frame time uses `MAX(main_globals.seconds_elapsed, 0.01f)`;
- the displayed rate is green at `>= 30` and red below it;
- the three four-byte formatting buffers are explicitly terminated after
  `_snprintf`;
- the existing `fast_ftol` helper supplies January's x87 conversion schedule;
- the render-camera bounds load precedes the progress conversion; and
- HUD font access goes through a named TU-private semantic prefix with a
  compile-time offset assertion, not a raw byte-offset dereference.

The focused comparison is instruction-for-instruction identical: 208 target
instructions and 208 candidate instructions. The whole-TU focused gate moves
Main from 82 exact / 11 residual / 2 unwritten to 83 exact / 11 residual / 1
unwritten, preserving all inherited exact owners. `_main_crash` remains
unwritten because manufacturing its target null write would not be acceptable
source reconstruction.

## HS retained fuzzy body

`render_debug_trigger_volumes` follows the HCEA implementation and preserved
project history: typed trigger-volume records and enums, normal render-debug
and collision APIs, project `real`/point/vector/color types, and the authentic
edge construction. It uses direct center-component arithmetic so that the
January inline schedule is preserved without emitting a
`point_from_line3d` code owner.

January and candidate are both 1088 padded bytes, have the same 34 relocation
destinations, and contain 342 decoded instructions. Objdiff reports 94.96407
percent. The remaining broad difference is VC7 local-stack allocation and
register scheduling. No declaration-order or equivalent-expression search was
used to pressure code generation. The whole-TU gate remains 60 exact / 6
residual / 0 unwritten and preserves all inherited exact owners.

## AI retained fuzzy body

`ai_scripting_allegiance_broken` is the natural boolean policy: reject either
`NONE` team, otherwise require both `game_team_is_ally` and
`game_team_is_enemy`. Those APIs already live in the correct allegiance owner.
The candidate retains the plain source shape recovered in `49e2d882e`.

January is 80 padded bytes and the candidate is 64, with the same two call
relocations; objdiff reports 83.333336 percent. January hoists one FALSE
materialization before the first guard, repeats it after the calls, and delays
the EDI save. Four ordinary source forms previously converged on the same
64-byte lowering, so artificial live ranges or duplicate conditions are not
admitted. The full AI-script gate is 101 exact / 15 residual / 0 unwritten and
preserves every inherited exact owner.

## Verification

The following focused gates pass, including the forbidden-emitted-symbol
guard:

```text
python tools/campaign/gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/hs/hs_runtime --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/ai_script --all --forbid-emitted-symbol _point_from_line3d
```

All exact-byte claims are limited to `_main_framerate_render`. A full canonical
build, semantic report regeneration, and global regression sweep remain the
orchestrator's publication gate.
