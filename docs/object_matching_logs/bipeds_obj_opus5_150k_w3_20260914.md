# `bipeds.obj` Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/units/bipeds.c`; target `build/split/source/units/bipeds.obj`.
- Lane worktree `opus5-150k-house-clean-20260914`, HEAD `210aa8f81`; VC7 13.00.9254 (XDK 3911), repository flags.
- **The real `bipeds.c` is unchanged** (`git diff --stat -- source/units/bipeds.c` empty). No header, config, park entry or
  tool was edited. Everything below is scratch measurement (`scratch/workers/w3_bipeds/`), worker notes
  `scratch/workers/w3_bipeds.md`.
- Ledgers read: `bipeds_obj_opus5_150k_w2`, `bipeds_obj_opus5_100k`, `bipeds_obj_opus5_100k_waveD`, `bipeds_obj_batch7_exact_and_fuzzy`,
  `bipeds_obj_adjust_placement_fuzzy`, the 150K and 100K handoffs, `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D), w2 worker notes.
- Evidence: January target COFF (bytes, relocations, frames); January `motion_sensor.obj` and `path_obstacles.obj` copies of
  `rotate_vector2d`; HCEA `units/biped_update_turning.c` (semantics only; also uses `(real_vector2d *)&turn_axis`).
- Blasts compiled the live worktree while other w3 workers had uncommitted edits (actor_moving.c, actor_perception.c, decals.c).
  Plain and shadow compiles use the same source copy, so the changed-function lists are self-consistent; the orchestrator's full
  build remains authoritative.
- Scratch tooling: `sgate.py` (compile a probe next to shadow headers and compare every function with tinfo), `ndiff.py`
  (alndiff with frame-slot renames normalised away), `patch.py`/`run.sh` (CRLF-preserving variant builder), `blast3.py` /
  `blast_rot.py` (plain vs shadow-header compile of every TU reaching a header, per-function status vs January).

## 2. Gates

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline | `== exact 41  residual 10  unwritten 0`, guard passed |
| Final (real file unchanged) | identical listing row by row (`scratch/workers/w3_bipeds/final.gate.txt`) |
| Park drift | 8 parks, 0 drift |
| Owner census vs `build/base` and target | no new / no missing owners |
| `tools/fake_match_scan.py bipeds.c` | 0 leads |

Newly strict exact in the real file: **none**. Every non-exact row is either parked (8) or blocked by an owner-level or policy
item (update_moving, update_physics).

## 3. Park-reopen proposal: `_biped_update` (1,120 / 61) with header packet vfd-HEAD

- Candidate `scratch/workers/w3_bipeds/park_reopen_update_needs_vfd_headers.c` = current real file + only the w2 `_biped_update`
  body (C4013 prototype widths, `0.1f*0.1f`, moving-first case order, short melee quarter).
- **New at HEAD:** the count-neutral half of w2's vfd packet (deleting the duplicate `ai_reconnect_to_structure_bsp` from
  `ai/ai_runtime.h`) was already consumed by the P3 `actions.h` packet (`ac4ff3d92`). The packet is now only the units.h and
  weapons.h prototypes.
- Shadow headers `scratch/workers/w3_bipeds/sh_vfd/` -> 42/51, only `_biped_update` changes (residual -> EXACT, nSHA16
  `fe3b81e079c33a13`). parkcheck: only `_biped_update` drifts (to the target). Census clean, fake scan 0.
- Blast (`blast/result_vfd3.txt`): 76 TUs reaching units.h / weapons.h, **2 changed functions, zero exact losses**:
  `physics::_physics_update_old` residual -> residual (5280/114 both, not parked) and the HEAD `_biped_update` park body.
  units.c, vehicles.c, encounters.c, weapons.c: unchanged (w2's `_encounter_update_respawn` re-baseline is no longer needed).
- Header text (orchestrator packet, multi-line house form):
  - `source/units/units.h`, immediately before `void unit_dialogue_update(`: `struct unit_animation_update_data;`, blank line,
    `void unit_animation_start_action(long unit_index, short action);`, `void unit_cause_player_melee_damage(long unit_index);`,
    `short unit_update_animation(long unit_index, struct unit_animation_update_data *data);`
  - `source/items/weapons.h`, after `short weapon_get_first_person_animation_time(...);`:
    `void weapon_stop_reload(long weapon_index);`, `boolean weapon_prevents_melee_attack(long weapon_index);`
  - Consumers: units.h 66 direct includers; weapons.h 19 direct (actors, actor_combat, aim_assist, game_engine,
    game_engine_ctf, game_engine_oddball, players, player_control, first_person_weapons, hud, hud_draw, hud_weapon, interface,
    weapons, objects, object_types, bipeds, units, unit_scripting_commands).
- Park criterion refuted as in w2: January return widths (`test al,al`, `cmp ax,1`), literal `3c23d70b`, jump-table order,
  melee store bytes.

## 4. `_biped_update_turning` (parked, 1,712 / 49): structural gap closed in scratch, x87 operand-age tie remains

Real park body 1760/49 (typed `real_vector2d` copies). Starting from the `(real_vector2d *)` cast form (t1: 1712, frame 0x2c vs
0x30, 27 real hunks), each step below is January byte evidence:

| Step | Evidence (January) | Source change | Real hunks |
|---|---|---|---:|
| rot | `motion_sensor` `_render_blip` inline copy computes j into a temporary before storing i; the EXACT out-of-line `path_obstacles` `_rotate_vector2d` has `real j = ...; real i = ...; result->i = i; result->j = j;` | shadow real_math.h rotate_vector2d body (in-place rotation needs the temporaries) | 27 (tail aligned) |
| F | third climbs test `mov eax,[ebp-8]; mov cl,[eax+2f4h]; test cl,40h` (re-read, no CSE) | `TEST_FLAG(definition->biped.flags, _biped_climbs_anything_bit)` -> definition outlives sine, frame 0x30 | |
| E | `jnp body; jmp skip` then `test ah,41h; jne skip` = `||` jump threading | `(turn_right && progress<0) \|\| (!turn_right && progress>0)` | |
| G (A4) | `load, load, store, store, store 0` for forward | `set_real_vector3d(&forward, turn_axis.i, turn_axis.j, 0.f)` | |
| D (A10) | `mov byte [ebp-2],0` between `test al,al` and the asleep `je` | `boolean flaming = FALSE;` in an enclosing `else` block, `if (seat == flaming) flaming = TRUE;` | 10 -> 8 |
| K | `fmul 10/3` precedes `fmul throttle.i` | `turn_rate = dot*(10.f/3.f); turn_rate = turn_rate*throttle.i - throttle.j;` | 6 |
| B | `jp` to the ratio arm; the `fld 1.0` arm falls through | `if (bank*target_bank <= 0.f) bank_blend = 1.f; else {...}` | 4 |

Result 557/557 instructions, 1712/49, frame and every slot identical. Remaining: 3-4 `fld X; fmul Y` operand swaps inside the two
inline `cross_product3d` expansions (flying turn rate, climbs turn axis). Learning bisect: G alone flips the earlier flying cross,
K flips the climbs cross; unrelated-function locals and declaration order are inert -> the A22 operand age is function-local and
moves with named-versus-pointer references anywhere in the function. Tie shapes (5): component stores (1728), 2D struct copy +
named k store (flying fixed, tail 1728), two-statement K (adopted, 4), MIN macros (3 with bank MIN; best `t8m2.c`). Stopped.

Not proposed for reopen (not exact; also needs the cast ruling and the rotate_vector2d header change).

### rotate_vector2d header evidence (not a landing proposal)

The body is a proven wrong reconstruction. Decisive check: bipeds.obj already emits a surplus select-any `_rotate_vector2d`
COMDAT from the header inline. With the current body that COMDAT is `58f40bb118ff928d`; with the corrected body it is
`9a9864975806d569`, byte-identical to January's `path_obstacles.obj` `_rotate_vector2d`.

The corrected body adds local declarations inside an inline, and the declaration-count effect costs exact functions. Blast
`blast/result_rot.txt` (268 TUs reaching real_math.h, 10 changed functions):
- exact losses: `bitmap_drawing::_bitmap_copy` 2,784, `game_engine_race::_race_engine_player_update` 160, `_race_touch_flag` 336;
- residual -> residual: `motion_sensor::_render_blip` (24 -> 15 real hunks) and its `_rotate_vector2d` COMDAT,
  `rasterizer_frame_statistics_draw`, `rasterizer_xbox_models::__rasterizer_model_draw` / `__rasterizer_model_transparent_geometry_submit`,
  `bipeds::_biped_update_turning` and its COMDAT.

A one-temporary body also loses `_bitmap_copy`; extra blank lines alone are inert. A count-neutral compensation is required before
this can land.

## 5. Not landed / skipped

| Function | Classification | Evidence this wave | Reopen criterion |
|---|---|---|---|
| `_biped_update_moving` 4080 (real 4000) | owner-level argument site | re-grepped: x87-staged radius at every `&S->center, S->radius` caller, integer push at collision_bsp/real_math; no genuine macro; um1 at HEAD 4096/96, 74 real hunks | authenticated radius argument spelling, then um1 |
| `_biped_update_physics` 5376 (real 5248/112) | header + policy (R12) | collision_debug.h still lacks the six externs; stick test reads `collisions[best]` with best = -1 (`movsx ecx,[ebp-38h]; imul ecx,2Ch; cmp [ebp+ecx-354h],eax` at 0xe8a on the first pass) | collision_debug.h packet + owner ruling on the OOB read |
| `_biped_check_discard` 176 | policy (R12 varargs) | argument layout re-read: `(2, fmt, name, x, y, z, describe)` against `%s (%s) ... (%.1f %.1f %.1f)` | owner ruling |
| `_biped_get_autoaim_pill` 336 | register/schedule tie | spherical-arm tail copy only; no helper for `*height = *global_zero_vector3d` | donor |
| `_biped_get_sight_position` 416 | x87 local creation | inline point_from_line3d excluded by the guard and January's out-of-line calls | donor |
| `_biped_accelerate`, `_biped_find_nearby_support_surface` | x87 operand age (A22) | no alternative natural reference spelling in either body | predictive A22 mechanism |
| `_biped_adjust_placement` | laws_w2 D do-not-repeat | no new law | donor |

## 6. Orchestrator proposals

1. **Header packet vfd-HEAD + park-reopen `_biped_update`** (section 3). Zero exact losses in 76 TUs.
2. **Research lane (no landing):** the A22 x87 operand-age mechanism. bipeds has three parks reduced to it
   (`_biped_update_turning` 3 swaps with every structural difference closed in `scratch/workers/w3_bipeds/t8m2.c` + `sh_rot`,
   `_biped_accelerate`, `_biped_find_nearby_support_surface`). The G/K bisect in section 4 is a ready-made lab input.
3. **Evidence only:** rotate_vector2d body correction (section 4) needs a count-neutral compensation before any landing;
   `(real_vector2d *)&vector3d` casts already occur at 18 sites in landed exact source (action_charge, action_obey, actors,
   actor_combat, actor_firing_position, actor_looking, actor_moving, motion_sensor, real_math, object_lights, collision_bsp,
   collision_features) and in HCEA's biped_update_turning.

## 7. Reusable VC7 facts measured this wave

- **Late named stores move earlier operand ages.** In one function, replacing a named `forward = axis; forward.k = 0` store near the
  end with `set_real_vector3d(&forward, ...)` flipped the fld/fmul order of an inline cross product 300 bytes earlier. Unused locals
  in another function and local declaration order were inert.
- **Dead-on-exit initialiser placement.** `boolean flag = FALSE;` declared in a block enclosing an `if` is emitted between that
  if's compare and its jcc; the same initialiser inside the if body is emitted after the jcc.
- **Constant association.** `dot*(10.f/3.f)*throttle` is reassociated to `dot*throttle*K`; a product stored first and then
  multiplied keeps `dot*K` first.
- **Inline body declarations count.** Adding one local declaration inside the `rotate_vector2d` inline body in real_math.h flips
  `_bitmap_copy` in a TU that never calls it; blank lines do not.
