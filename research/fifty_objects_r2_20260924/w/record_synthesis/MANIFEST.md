# MANIFEST - record-count synthesis (slug record_synthesis), LAB ONLY

Base/tip: 7b9de585. Its source/config/tools are identical to 42125a52 (`git diff --stat 42125a52 7b9de585 -- source config tools`
is empty). No tracked file was edited. `git status` shows only the pre-existing untracked research/ directory.
Claim: source/interface/ui_widget_game_data_input_functions (record-synthesis). It is released at the end of this run.
math/real_math is RESERVED for Codex. The real_math.h packets below are measured, not proposed for landing now.

## Harness
- `slab.py` is the canary lab/board fingerprint (cwd=<tree>, build.ninja flags, strict `section_infos_equal`
  compared against build/split), with two fixes. The unit list covers all 3 ninja rule forms, giving **454 units**;
  the canary harness had 447 and missed ui_widget_gdif. It also supports simulated symbols.json renames for R.
- Control: a sweep of sh/ (copy of source/) matches the build/base fingerprint exactly. 454 units, 7251 EXACT,
  0 gained, 0 lost, 0 drift, 0 data drift.
- `win.py` is the solo counter (K = 0..63 top dummies). `tuwin.py` computes the joint window of any TU.
  `combos.py` and `codex_lookup.py` hold the additive arithmetic, which was validated against measured variants:
  V, E, RV, RVE, PU, MN, JAN and J1 all match.
- Variant builders are `ugd.py` (R/Rm/V/E/PU/MN/bdB + canary patch sets PA/B1) and `jtree.py` (imports the
  real_math_h_recovery jan.py edit catalogue READ-ONLY). `run_configs.py` builds, sweeps and diffs a configuration.

## Configurations measured (whole board, 454 units, vs sh)
| config | contents | gained | lost | notes |
|---|---|---|---|---|
| u_V | 6 campaign verify typedefs removed from the ui_widget_gdif .c | 0 | 0 | gdif net -16 |
| u_E | `#include "cseries/errors.h"` in the gdif .c (C4013 error()) | 0 | 3wide 1232 | gdif net +44 |
| u_R | r1 storage/order packet (ported), renames simulated | 0 | ms 1424, 3wide 1232 | January order; count shift |
| u_RV / u_RVE | R + V (+E) | 0 | 0 | object-audit repair (r1: 165/165 symbols) |
| u_PU | player_ui.h duplicate prototype (later, out-of-order copy) | 0 | 0 | net -2 in 13 TUs |
| u_MN | main.h `unsigned long` -> `unsigned long context` | 0 | 0 | net +1 |
| PA | canary packet A (rlr vp vpd vpc swcw bdB) | _collision_move_point 4752 | 0 | collisions 20/20, object audit PASS |
| PAE | PA + B1 effects half | 4752 | 0 | + effects _real_local_random_range surplus |
| C1 | PA + PU + MN | 4752 | 0 | |
| C2 / C3 / C4 | PA + PU (+MN) + R + V (+B1) | 4752 | 0 | |
| J0 | JAN tier (spl1 spl3 rlr vp vip2 vrv2 vrn2 + companions + vpc) + swcw + bdB | 0 | 5 (8,144 B) | move_point not regained (net +5 in its TU) |
| J1 | J0 + PU | **solo 704** | 5 (8,144 B): draw 4176, units preprocess 1920, ncmh handle_message 1088, alpha_bleed 560, decals_delete_permanent 400 | the only measured solo closure |

## Packets (exact files)
- **PA** (after Codex): `pkt/A_real_math.h.patch`, `A_matrix_math.c.patch`, `A_collisions.c.patch`,
  `A_cseries_windows.h.patch`, `A_bitmaps.h.patch`, plus optional `pkt/effects_owner_gated.patch` (B1). These are
  copies of the canary lane's patches. They apply cleanly at 7b9de585 with `patch -p1 --binary` (B1 without
  `--binary`).
  - Gain: +1 function, +4,752 B strict. The collisions object completes (20/20, audit PASS), subject to admission of
    the surplus COMDAT.
  - Loss: 0.
  - Owner decisions:
    - The duplicate-copy choice is **count-driven**. bdB is used instead of the ownership-natural bdI (PAI loses
      units 1920), and swcw instead of swcs.
    - COMDAT admission for the surplus `_valid_real_plane3d` in collisions/render_cameras (== January's selected copy).
    - Re-baseline the parked `__rasterizer_model_draw` (89b0d7ea -> cbfa8585).
- **PU + MN** (landable now, no reserved file): `packet/PU_player_ui.h.patch`, `packet/MN_main.h.patch`.
  - Zero-loss, 0 gain. The PU copy is chosen by definition order, not by count.
  - They shift counts (gdif net -1), so re-read codex_lookup.txt before choosing when to land them.
- **R + V** (no reserved file; owner decision): `packet/RV_ui_widget_game_data_input_functions.c.patch`,
  `packet/RV_ui_widget_game_data_input_functions.h.patch`, and `packet/R_symbols_json_ops.txt`.
  - The symbols.json ops are 41 `"static": true` flags + 2 static-local renames; edit the lines in place.
  - Zero-loss, 0 gain. The ui_widget_gdif audit fix comes from the r1 evidence.
  - R alone now LOSES 2 functions. The choice of V (or E) as compensator is count-driven.
- All patches were verified to reproduce the lab variant files byte-for-byte (CRLF), applied to a pristine sh/ copy.

## Verdict
No principled configuration closes a count-sensitive residual except PA, which closes collision_move_point with
genuine bodies but still carries the count-driven duplicate-copy owner decision.
- Solo (ui_widget_gdif, 704 B) closes only with the complete January-attested real_math.h tier plus swcw and PU (J1).
  That configuration loses 5 canaries.
- frame_statistics then needs exactly -5 records, and no principled lever set supplies that.

## Reopen criteria
1. **Codex reports.** Measure its net N in the gdif TU with `python win.py <codex tree>`, then read codex_lookup.txt.
   - N in {+1, +3, +4, +5}: MN, MN+PU or PU (genuine hygiene) closes solo.
   - N in {+2, +3}: solo closes with nothing added.
   - Re-sweep the whole board (`run_configs.py`) and every canary TU (`tuwin.py`).
2. **First-party evidence** of January's header content that changes the frame_statistics or gdif TU counts (a donor
   header, a January PDB with types). This would be needed to justify a -5 in frame_statistics, or any gdif lever set
   outside the table.
3. **An owner ruling** that accepts count-selected compensators (V, E, header consolidations).
