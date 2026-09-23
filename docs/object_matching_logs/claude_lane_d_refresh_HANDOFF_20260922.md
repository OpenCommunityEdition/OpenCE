# Claude Lane D refresh handoff (2026-09-22/23)

- Worktree: `C:\halo-worktrees\claude-lane-d-refresh-20260922`
- Branch: `claude/lane-d-refresh-20260922` (local only, **not pushed**)
- Base: `fe77cd542f80de4dfd05ad35e86fe65b41d603c3` (tip of `jonas/exact-pilots`,
  contains reconciliation `4f4c85021db48dae61252d8e1301c60e1c2cc138`)
- Brief: `docs/campaign_prompts/CLAUDE_REMAINING_LANE_D_REFRESH_20260922.md`
- Machine-readable record: `claude_lane_d_refresh_manifest_20260922.json`
  (per-function rows, COMDAT admissions, ruling queue, rejected trials)

## Result

| Measure | Value |
|---|---|
| Strict exact functions gained (section_infos_equal) | **44** |
| Strict padded code bytes | **28,896** |
| Strict meaningful code bytes (objdiff function size) | **28,605** |
| Stable-verdict regressions | **0** |
| objdiff Halo code progress | 1,498,567 → 1,524,233 (+25,666); functions 7,354 → 7,388 |
| Fuzzy zero-credit movement landed | `_poll_endpoint_set` 72.27% → 98.54% (park re-measured) |
| Data-only / scorer-only gains | 0 bytes |
| `linker_common` | zero credit, untouched |
| Whole-object completions (code and data 100%) | 10 |

By category:

| Category | Functions | Padded | Code |
|---|---|---|---|
| Source reconstruction (packets 1–4) | 33 | 25,824 | 25,600 |
| Config relocation alias, no source change (packets 5–6) | 10 | 2,992 | 2,939 |
| Source + symbols.json static split (packet 7) | 1 | 80 | 66 |

The alias rows were already accepted by the semantic scorer as ordinary
relocation differences, so they add strict verdicts without moving objdiff
progress.

Whole objects completed: `bungie_net/common/prime_numbers`,
`cseries/stack_walk_windows`, `effects/contrails`, `effects/particle_systems`,
`items/items`, `sound/game_sound`, `sound/ima_adpcm`, `sound/sound_dsound_xbox`,
`structures/structure_detail_objects`, `structures/structure_visibility`.

Allowlist (49 units) residual pool: 118 rows / 90,304 B → 74 rows / 61,408 B.
The one unwritten row (`_main_crash`, 16 B) is the rejected null-write and
was not re-landed.

## Commits (oldest first)

| Packet | Commit | Content | Padded |
|---|---|---|---|
| p1 | `6099e659` | items, projectiles | 9,856 |
| p2 | `22e62492` | game_sound, sound_manager, sound_dsound_xbox, ima_adpcm | 5,088 |
| p3 | `0a571b33` | hs_compile, hs_runtime, contrails, particle_systems, structures, structure_visibility | 6,896 |
| p4 | `dc58693c` | stack_walk_windows, main, bink_playback, draw_string, prime_numbers, transport_endpoint_set_winsock | 3,984 |
| p5 | `1b439d04` | relocs.json: leaf_map node-stack alias (5 rows) | 1,424 |
| p6 | `79096027` | relocs.json: HS enum/tag/object tables + decal geometry aliases (5 rows) | 1,568 |
| p7 | `a9e23674` | structure_detail_objects real file statics + symbols.json | 80 |
| docs | (this commit) | handoff + manifest | 0 |

No header was changed. Every packet ran `scratch/lane_d/wave_gate.py`: full
ninja, stable snapshot diff (0 regressions), parked-function validation
(0 stale / 0 invalid), object admission audit (0 contradicted / 0 revoked),
fake-match scan on changed files, `git diff --check`. Every newly exact parked
function was unparked in its own packet (27 in all); `_poll_endpoint_set` was
re-measured in place because its bytes moved.

Final gates at the tip: ninja PASS; stable exact rows 7,493 → 7,537; parks
151 → 124 active; admission 51/0/10/0 → 61/0/10/0 (the +10 candidates are
the ten completed objects); pytest 1,152 passed / 5 skipped / 26 subtests
(same as baseline); castcheck clean on every landed file.

## How the work was verified

Two worker waves ran (wave 1: 16 agents, wave 2: 32 agents under the owner's
one-time cap exception), one worker per disjoint `.c`, PID-isolated `gate.py`.
Worker claims were not credited until the orchestrator:

1. re-ran `scratch/orch/land.py` (unit row diff, `_point_from_line3d` guard,
   ownership, fake scan, line endings) on every candidate;
2. read every diff for semantics and fake-matching, and checked each unusual
   shape against the first-party debug build (`scratch/orch/odbuild.py`,
   data only) or HCEA DIA2Dump. Confirmed this way: the debug-build
   detonation-timer fall-through, `(moved = TRUE) &&` and duplicate
   `magnitude_squared3d` in projectiles; the doubled
   `state = active` store in game_sound; the constant `split_horizontally`
   byte local in main; `if (ignore_levels) while (--ignore_levels)` in
   stack_walk; the repeated external test in hs_runtime; the one-field
   `variables` walk (RTC size 4) in particle_systems; the identical FD_SET
   arms in transport_endpoint_set_winsock; the unused BSP local in
   structure_visibility; HCEA file statics for structure_detail_objects;
3. ran `castcheck.py` (no new punning casts) and the COMDAT owner census
   (`scratch/orch/helper_census.py`, saved in `scratch/lane_d/comdat_census_before.txt`).

Orchestrator corrections before landing (details in the manifest):

- sound_manager: the worker's `update_channel_for_looping_sound` rewrite
  changed behaviour (the permutation-advance branch became reachable when a
  next definition exists and the chain continues), and the `(word)` cast
  removals changed NONE from 65535 to -1 through a `long` parameter. Both
  functions were reverted to baseline; the four gains are unaffected.
- hs_compile: dropped the bare discarded `hs_function_get` call (fuzzy only).
- hs_runtime: replaced the invented macro `HS_GLOBAL_DESIGNATOR_TO_DATUM_INDEX`
  with the file's own if/else index idiom (same bytes).
- structure_visibility: bare `global_structure_bsp_get();` became the
  debug-build unused local (same bytes).
- transport_endpoint_set_winsock: dropped redundant zero stores in
  `transport_initialize` (fuzzy only).
- geometry: both gains depend on the `realcmp` macro expansion (the plain
  `fabs` spelling fails the strip test) and one on a new point->vector cast,
  so geometry was held.

## COMDAT admissions (owner narrow exception)

| Object | Symbol | Basis |
|---|---|---|
| items | `_point_from_line3d` | January items.obj references it out of line (ruling); byte-identical |
| projectiles | `_point_from_line3d`, `_collision_test_line` | January projectiles.obj references both out of line; byte-identical |
| game_sound | `_vector_from_points3d` | all-inlined class; identical to January's action_alert copy |
| structures | `_scale_vector3d` | all-inlined class; identical to action_charge copy |
| particle_systems | `_local_random_range` | all-inlined class; identical to contrails copy |
| structure_visibility | `_plane3d_distance_to_point` | all-inlined class; identical to decals copy |

January shows one folded definer per header inline; production already emits
these helpers from 19–75 objects each, so each admission adds one more
definer of an established pattern. Each caller is strict exact and no sibling
regressed.

## Owner ruling queue (46 items, 36,992 B held)

28 items / 17,200 B are **strict exact under a ruling**; candidate files are
under `scratch/lane_d/w/<unit>/` (named in the manifest).

| Class | Items | Held | Exact under ruling |
|---|---|---|---|
| A authentic January bug / UB (uninitialised reads, `%s` with shorts, csprintf format-as-buffer, pointer-before-check, capacity loop) | 14 | 7,664 | 13 / 7,344 |
| B `_point_from_line3d` emission where January does not reference it | 7 | 10,816 | 1 / 1,824 (`_trigger_create_projectiles`) |
| C PDB-typed `volatile unsigned long flags` (cache_files_decompress_windows; rejected 20260914, new HCEX type record) | 5 | 2,176 | 5 / 2,176 |
| D `realcmp` macro family placement (geometry ×2, connected_geometry) | 3 | 1,984 | 3 / 1,984 |
| E header changes (player_profile return-by-value, real_math.h distance2d/distance3d) | 4 | 1,616 | 3 / 960 |
| F return ABI (`_weapon_place`) | 1 | 208 | 1 / 208 |
| G held .bss static split and new rule-24 casts (editor_flying_camera, lightning, player_effects) | 5 | 5,056 | 2 / 736 |
| H layout-only redundant assignment (`_network_connection_connect`) | 1 | 288 | 1 / 288 |
| I placeholder-named dead local (`_weather_particle_system_render`) | 1 | 1,680 | 1 / 1,680 |
| J semantic dispute / unresolved (fuzzy) | 5 | 5,504 | 0 |

Highest-value single questions:

1. **Original-bug policy (class A, 7,344 B exact).** Most items now carry
   first-party /Od+RTC evidence that the uninitialised read is authentic
   (`_RTC_UninitUse` names `result`, `allowed`, `direction`), which the earlier
   rejections did not have.
2. **PDB-typed volatile (class C, 2,176 B, one object to 46/46).**
3. **realcmp owner placement (class D, 1,984 B).** January asserts prove the
   macro exists; it is not yet in `real_math.h` and actors.c/actor_looking.c
   define it locally.
4. **`_trigger_create_projectiles` (1,824 B)** under the all-inlined
   `_point_from_line3d` class.

`_dead_camera_update` (1,248 B) and `_connect_endpoint_process@4` (304 B) were
strict exact in wave 2 but are uninitialised reads, so the orchestrator held
them under class A rather than land them.

## Remaining pool and recommendation

The 28 non-held residual rows (24,416 B) are, per their worker ledgers,
backend ties (x87 operand order, callee-saved register swaps, frame-slot
permutations) or depend on held forms. Under brief rule 6 no third wave was
spent on them. The yield now depends on owner rulings, not more agents.

Out of scope but cheap: the semantic report still lists nine `ordinary_only`
relocation-alias rows outside the Lane D allowlist (four in
`interface/ui_widget_game_data_input_functions`, plus collision_usage,
interface, thread_win32, bitmap_utilities, libcmt stream). They are the same
class as packets 5–6 and can likely be closed by relocs.json line surgery
after checking each delta.

## Reproduce

```
python scratch/lane_d/wave_gate.py final scratch/lane-d-before.json
python -m tools.campaign.stable_verdicts diff scratch/lane-d-before.json scratch/lane-d-after-final.json
python -m pytest -q -p no:cacheprovider --basetemp=scratch/pytest-final
```

`scratch/` is untracked; the baseline snapshot, per-packet gate logs
(`scratch/lane_d/gate_p1..p7.txt`), worker ledgers and ruling candidates stay
in the worktree for review.
