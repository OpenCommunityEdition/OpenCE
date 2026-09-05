# HUD renderer: authenticated owner/API preparation (2026-09-04)

## Verified scope

This prerequisite packet starts from published
`4c862ae20ddee514c75e73adc196d6a89260b288`. It prepares the genuine shared
interfaces needed by the remaining HUD Unit renderer. It does **not** claim
the renderer implemented, any new exact bytes, or a newly completed object.
The subsequent renderer source must receive its own frozen gate and review.

The full Ninja rebuild and stable before/after comparison retain all
**6,183 exact owners out of the same 8,245-owner population**, with zero new
exact functions and zero regressions. All **251 existing parks remain valid**;
the admission audit has zero candidates/contradictions and the same four
reviewed rejections. Tests report **288 passed and 26 subtests passed**, with
only the existing optional pytest-cache permission warning.

## Source changes and evidence

- `hud_draw.h` owns the typed `hud_calculate_point`, `get_flash_duration`,
  `hud_draw_meter`, and `hud_draw_static_element` declarations. Shared pointer
  types use forward declarations, and draw-flag bit indices are named.
- `motion_sensor.h` owns `motion_sensor_draw_screen(short, boolean,
  union point2d const *)`, requiring only a narrow union forward declaration.
- `game_engine.h` and the existing Game Engine definition now accept the real
  `long player_index` argument for `game_engine_hud_draw_motion_sensor`. Its
  January body does not consume this argument; that does not make the public
  calling convention zero-argument. January's HUD caller pushes its saved
  player index at renderer +0x897, calls at +0x89B and cleans four bytes.
- `unit_hud_interface_definition.h` now owns the genuine auxiliary overlay
  and meter tag element layouts. These are complete PDB-backed types, not
  opaque prefixes invented for field offsets. Overlay type/flags are at
  +0x68/+0x6A, with total size 0x84. The meter's panel is at +0x14, with total
  size 0x144. Static assertions enforce all these values.
- The existing `hud_anchor` enum in `hud_definitions.h` supplies bottom-left
  value two; no duplicate private `hud_corner` enum or unrelated HUD Messaging
  source migration was introduced.

Root independently queried the actual HCEX PDB for both auxiliary types and
all six API parameter lists. The command pattern is:

```powershell
$dia = 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe'
$pdb = 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
& $dia -type auxilary_overlay_definition $pdb
& $dia -type auxilary_meter_definition $pdb
& $dia -sym hud_draw_meter $pdb | Select-String -Pattern 'Function       :|Data           :.*Param'
```

Repeat the final query for `hud_calculate_point`, `hud_draw_static_element`,
`get_flash_duration`, `motion_sensor_draw_screen`, and
`game_engine_hud_draw_motion_sensor`. Actual records confirm the proposal's
short, byte/boolean, real, long and const pointer types. Later PDB evidence
corroborates types and names; January remains the calling-convention and
byte authority. The scratch proposal is
`scratch/hud-renderer-owner-proposal-20260904.md`.

No caller-local public prototype, guessed SDK declaration, compiler-control
directive, fake padding, or new inline helper was used. The pre-renderer
HUD Unit sentinel remains 20 exact / 0 residual / 2 unwritten and passes the
whole-object forbidden `_point_from_line3d` emission guard.

## Full-build reproduction

```text
python -m tools.campaign.stable_verdicts snapshot scratch/hud-renderer-owner-api-before-20260904.json
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/hud-renderer-owner-api-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/hud-renderer-owner-api-before-20260904.json scratch/hud-renderer-owner-api-after-20260904.json
python -m tools.parked_functions --output scratch/hud-renderer-owner-api-parks-20260904.json
python tools/audit_object_admission.py --output scratch/hud-renderer-owner-api-admission-20260904.json
python -m pytest -q tools
git diff --check
```

The baseline was captured before the owner/API changes were compiled. The
full source tree was frozen for the build and snapshot. The later renderer
body was held in scratch until this zero-loss validation finished.

## Adjacent reviewed source reconciliation

The same work period also cleaned DrawPrimitives' existing public API-header
use and replaced its four-byte window prefix with the real existing window
type. Its separate full build retains all 8,245 owner verdicts and all 6,183
exacts. See `rasterizer_xbox_draw_primitives_obj_api_owner_followup_20260904.md`.
No new byte credit or whole-object approval follows from that cleanup; its
shared debug/frame-statistics layout and SDK ownership boundaries remain open.

The newly reconstructed Devices debug caller is deliberately **not** part of
production source: its 432-byte caller is strict, but the object emits a
forbidden 48-byte helper. Its preserved source and reopen condition are in
`devices_obj_debug_renderer_inline_owner_boundary_20260904.md`.
