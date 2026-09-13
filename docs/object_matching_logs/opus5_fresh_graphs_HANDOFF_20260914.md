# Opus5 fresh-graphs lane — handoff (2026-09-14)

## Verdict

The lane verifies **+25 strict exact functions / +15,797 meaningful / +15,984
padded code bytes** against the pinned canonical base, with **zero lost exact
functions**. The 30,000-byte objective was **not met**: the shortfall is 14,203
meaningful bytes.

Totals, reported separately and never combined:

| total | value |
| --- | ---: |
| `verified_new_exact_meaningful` | **15,797** |
| `verified_new_exact_padded` | **15,984** |
| `fuzzy_meaningful_target_bytes_zero_credit` (12 retained bodies) | **22,548** |

The authoritative totals come from `build/semantic_report.json`: 6,972 / 1,140,161
at the base and **6,997 / 1,155,958** at the final checkpoint.

The machine-readable record is `opus5_fresh_graphs_admission_manifest_20260914.json`.
It carries per-function section index, meaningful/padded bytes, target and
candidate normalized hashes, ordered relocations and fingerprint, name
provenance, ABI/prototype owner, source evidence, header blast radius,
comparator result, owner census, point guard, review verdict and commit.

## Identity

| item | value |
| --- | --- |
| Worktree | `C:\halo-worktrees\opus5-30k-fresh-graphs-20260914` |
| Branch | `opus/30k-fresh-graphs-20260914` (local; not pushed; canonical checkout untouched) |
| Base | `c3e257e9ab9a5d7e1c9b39862f87ac2abde4359c` (fetched `jonas/exact-pilots` tip, verified by hash) |
| Start proofs | clean status; configure + full ninja pass; 6,972 / 1,140,161; frozen stable snapshot `scratch/base-snapshot.json` (8,245 / 6,937 exact, SHA-256 `39557d49a2652d18d1286e119e861983e71a0e7e0e86a80592a3f58bca69e41d`) |
| Tooling | VC7 13.00.9254 with repository per-TU flags; Ghidra 12.1.2 headless decompilation of all 14 portfolio split objects; January debug-map atlas and HCEA/Stian/punpckhdq as semantic evidence. IDA was unavailable at lane start. The IDA/PDB evidence delivered later is used by the separate `opus/pdb-ida-15k-20260914` lane. |
| Reference docs | `AGENTS.md`/`CLAUDE.md` are absent from this repository. They exist only in the Stian research repository, a different functional-lift project, and were read as context only. |

## Ordered commits (base..tip)

| commit | kind | content | new exact (padded) |
| --- | --- | --- | ---: |
| `47d9dd0da` | config | 13 January-map private names (actor_moving 7, action_obey 4, hud_draw 2), all static | 0 |
| `0569dd175` | source | encounters: update_follow, spawn_actor, post_combat_select_random_behavior | 2,480 |
| `0501bd06e` | source | hud_draw: hud_draw_meter, hud_draw_numbers | 3,168 |
| `262759b8a` | source | actor_moving graph: calculate_free exact + 8 fuzzy prerequisite bodies | 720 |
| `40e1b7964` | source | virtual_keyboard_render_internal (fuzzy) | 0 |
| `a694cf1b2` | header | actor_perception.h owns actor_perception_unreachable; action_fight.c cleanup | 0 |
| `858bedd04` | source | action_charge_perform (fuzzy) | 0 |
| `c86aa48bd` | header | real_math.h limit3d polarity + midpoint3d return | 112 |
| `8650b238b` | source | aim_assist object_compute_autoaim_target | 416 |
| `ef7bc742a` | header+config | ai_scenario_definitions.h atom modifier enums and ai reference structs; action_alert.c cleanup; two park re-measures | 0 |
| `423067414` | source | actor_moving midpoint3d | 64 |
| `6a65c04c1` | source | encounters post_combat | 1,984 |
| `c078622f2` | source | action_obey command graph (4 exact + 2 fuzzy callers) | 1,952 |
| `a76d937e7` | source | ai_script: 8 functions | 4,208 |
| `b510b1d58` | source | game_engine select_players_to_display | 496 |
| `65277e66c` | source | render_sprite build_sprite_get_group | 384 |
| `f583b471a` | docs | frontier ledgers: actor_type_infection, hs_compile, sound_dsound_xbox, weather_particle_systems | 0 |
| (this commit) | docs | handoff + manifest | 0 |

## Changed files (base..tip)

- **Source (12):** action_alert.c, action_charge.c, action_fight.c, action_obey.c,
  actor_moving.c, ai_script.c, encounters.c, aim_assist.c, game_engine.c,
  hud_draw.c, virtual_keyboard.c, render_sprite.c.
- **Headers (3):** `source/ai/actor_perception.h`, `source/math/real_math.h`,
  `source/ai/ai_scenario_definitions.h`.
- **Config (2):** `config/symbols.json` (13 in-place line renames; entry order
  preserved) and `config/parked.json` (two measurement refreshes with appended
  evidence).
- **Docs:** 14 `*_obj_opus5_fresh_graphs_20260914.md` ledgers, this handoff and
  the manifest.
- **Untouched:** every do-not-touch file (actor_looking.c, actor_perception.c,
  collisions.c, weapons.c, hud_weapon.c, devices.c) and every excluded object
  (units, matrix_math, vehicles, ai_debug), plus all tools and tests.

## Header blast radius (each prerequisite verified by its own full build)

- **`actor_perception.h` (+1 prototype):** actor_perception.c does not include this
  header. Direct includers keep their counts (action_flee 10, actor_firing_position
  16, ai_communication 40, action_fight 6). 0 regressions.
- **`real_math.h` (185 includers):** the combined proposal failed, and a bisection
  isolated the added `pin_normal_to_cone3d` prototype as the sole cause (3 inherited
  exact regressions and 4 park drifts). The admitted packet keeps only the
  `limit3d` and `midpoint3d` inline-body corrections: 0 regressions, +2 exact
  (`_limit3d`, `_reciprocal_square_root`).
- **`ai_scenario_definitions.h` (17 includers):** 0 strict losses. The
  declaration context flips two already-parked fuzzy functions between documented
  equivalent states: `_ai_communication_update_speech_timers` (the recurring EBX/DI
  commuting load pair) and `_encounter_update_respawn` (a reload swap). Their park
  measurements were refreshed with appended evidence, keeping the same size and
  relocations.

## Integration rulings made by the orchestrator

- **Consumer-local function prototypes were not admitted.** One was converted to a
  header owner (actor_perception_unreachable). One blocked a fuzzy body, which was
  withheld (`player_aim_projectile`).
- **TU-local enum/struct copies are admitted only with a disclosure comment**, and
  only when no shared header declares the type yet. This follows canonical's
  existing practice ("encounters.h does not yet declare these"). Header
  consolidation is queued where the genuine header would collide with an active
  lane: hud number/meter definitions vs `hud_weapon.c`; actor combat-status levels
  vs `actor_perception.c`.
- **Per-TU inline renames** (`#define distance3d distance3d_inline` plus an
  external prototype) were accepted only where the file already uses that
  mechanism and January has the out-of-line call relocation.
- **Adversarial review was run for every landed unit.** Five findings led to
  reverts or fixes: the infection rewrite, the crosshair-style duplicated store in
  action_obey command_begin, stray braces, indentation, and inaccurate enum
  provenance comments.

## Owner census disclosures

- **action_obey:** header-inline SELECT_ANY copies `_distance_squared3d`,
  `_dot_product3d`, `_real_random_range` and `_vector_from_points3d`, each of which
  January references as an undefined external, plus an unreferenced
  `_dot_product2d`.
- **encounters:** one unreferenced header-inline SELECT_ANY `_square_root`.
- These belong to the systemic surplus-COMDAT class already present in canonical
  base objects. No new non-inline code, data, BSS or COMMON owner was added.

## Final verification (tip before this docs commit)

| gate | result |
| --- | --- |
| configure + `ninja all_source progress semantic_progress` | pass |
| accepted ledger | 6,997 / 1,155,958 |
| stable diff base → final | 25 gained / 15,984 padded / **0 regressions** |
| parked functions | 465 active / 0 stale / 0 invalid |
| object admission | 0 candidates / 0 contradictions / 7 rejections (unchanged) |
| pytest tools | 1,151 passed / 5 skipped / 26 subtests |
| `git diff --check` base..tip | pass |
| fake-match scan (changed `.c`) | 2 findings, both inherited in hud_draw.c |
| `_point_from_line3d` guard | passes for every changed unit except action_charge, whose own January object owns the exact 48-byte section (inherited) |

## Fuzzy retained (zero exact credit)

- **actor_moving (8 previously unwritten):** path_refresh, destination_update,
  calculate_movement, calculate_controlled_by_aiming, avoidance_setup,
  test_avoidance_vector, vector_avoidance_find_direction, vector_avoidance.
- **Others:** virtual_keyboard_render_internal, action_charge_perform,
  action_obey_command_begin, action_obey_describe_command.
- Sizes, hashes, relocations, similarity and blockers are in the manifest.

## Rejected or withheld trials (all reverted)

| unit / function | trial | why rejected |
| --- | --- | --- |
| actor_type_infection | 3600/104 rewrite of a written residual | not exact |
| aim_assist `_player_aim_projectile` | fuzzy body | its header prototype regresses real_math.h consumers |
| real_math.h | `pin_normal_to_cone3d` prototype | declaration-count regression (bisected) |
| actor_moving `_actor_move_avoidance_setup` | exact form via `distance2d` | point-to-point2d representation casts |
| action_charge | `__inline` on 11 local helpers | changes COMDAT selection of 11 exact siblings with no exact gain |
| hud_draw pixel32 converters and `fast_ftol_C` | hand-x87 packets | owner policy on new inline assembly |

## Blocked inventory (66 rows in the manifest)

Dominant classes:
- register/x87/scheduling ties (encounters, actor_moving, sound_dsound_xbox,
  weather, game_engine, ai_script);
- `_point_from_line3d` emission policy (weather `update_physics`);
- owner-ruled hs_compile families (unused call-result local, third
  expression_index reference);
- the render_sprite parameter-home write needing a UB ruling;
- actor_type_infection register cluster;
- action_charge setup frame-cell packing.

Owner-actionable proposals with unlock values:
1. Weather: include `bitmaps/bitmaps.h` to fix the C4013 double-push of
   `rgb_colors_interpolate`. This is a runtime defect repair and is byte-neutral
   for siblings.
2. Consolidate the hud number/meter structs and actor enum headers once
   `hud_weapon.c` and `actor_perception.c` are free.
3. Admit the pixel32 asm helpers (568 padded).
4. Decide the action_charge helper linkage.

## Overlap statement

- **Canonical:** base `c3e257e9a`; the canonical checkout was never modified.
- **Parallel lane `opus/pdb-ida-15k-20260914`** (opened during this lane at the same
  base) owns interface, projectiles, rasterizer_xbox_active_camouflage,
  stack_walk_windows and actor_combat. Its no-overlap list names every file this lane
  changed, and this lane's planned actor_combat wave was cancelled to avoid overlap.
- **Before integration:** re-check canonical for new commits touching the 12 source
  files, 3 headers and 2 configs above.

Scratch evidence stays untracked under this worktree's `scratch/`: snapshots,
logs, Ghidra exports, atlas dumps, worker reports and verifier probes.
