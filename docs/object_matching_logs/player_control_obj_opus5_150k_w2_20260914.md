# `player_control.obj` opus5 150K house-clean wave w2: park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/game/player_control.c`. Target: `build/split/source/game/player_control.obj`, the January 2002
  Xbox debug build (VC7 13.00.9254, XDK 3911). Measured with `tools/campaign/gate.py` using the unit's `build.ninja` flags.
- All three residual rows are PARKED and locked. Every experiment used scratch copies in `scratch/workers/player_control/`.
  The real source file is unchanged: `git diff --stat -- source/game/player_control.c` is empty.
  - Worker notes: `scratch/workers/player_control.md`.
- Inherited from the 100K lane and not touched: `_handle_one_player_input`.
- Ledgers read before starting:
  - fable_salvage_admission_20260902
  - input_cluster_completion_20260903
  - jonas_aiming_unit_leaf_20260828
  - the three jonas first-shot ledgers of 20260827
  - jonas_private_cluster_census_20260830
  - opus5_100k_waveD_20260914, plus its worker notes
  - the 100K HANDOFF
- `branch_sweep` covered 25 unique blobs. None is better than the current 46/3/0.
- Evidence used:
  - the January COFF (bytes, relocations, `.rdata`);
  - January PDB publics (`scratch/pdb200-current/pdb_symbols.json`);
  - the January map atlas, which records functions only for this object;
  - HCEA `get_local_player_input_blob.c` and `player_control_initialize_for_new_map.c`, for names and semantics only.

## Baseline

`gate.py source/game/player_control --all --forbid-emitted-symbol _point_from_line3d` reports
`== exact 46  residual 3  unwritten 0`, and the guard passed.

| Function | Padded | Relocs | Parked base sha16 | Target sha16 |
| --- | ---: | ---: | --- | --- |
| `_get_local_player_input_blob` | 3456 | 130 | 515cbede397cc244 | 4b4ad9ae985a33b5 |
| `_player_control_initialize_for_new_map` | 144 | 10 | 782be32752d680a0 | adb32edd759e2bbc |
| `_player_control_modify_desired_angles` | 1584 | 83 | c61e9a2f2f567473 | 6e468bddb00f141c |

## Result: two park-reopen proposals, strict EXACT in scratch

Candidate: `scratch/workers/player_control/c4.c`. It is the current real file plus only the reopened bodies and the two TU
declarations they need. The diff is `c4.diff`.

- Whole-TU gate: `== exact 48  residual 1  unwritten 0`, guard passed.
- Row-by-row against the baseline, only the two parks flip to EXACT. No baseline EXACT row is lost.
- `scratch/parkcheck.py`: `_player_control_modify_desired_angles` equals its parked base. The two reopened rows equal
  their targets (3456/130/4b4ad9ae985a33b5 and 144/10/adb32edd759e2bbc).
- `tools/fake_match_scan.py`: 0 leads, the same as HEAD.
- The `/W3` warning set is unchanged.

### New January evidence

1. **`player_control_globals` is source-static in January.**
   - The PDB publics list every neighbour in its `.bss` block: `_debug_input_target` at rva 4451892, the look-rate arrays,
     and the three `controls_*` flags. `_player_control_globals` at rva 4451888 is absent, exactly like the static
     `_player_control_update_section`.
   - No other split object references the symbol.
   - Codegen consequence: an address-not-taken static cannot be aliased by a store through a pointer.
     - In the mouse path, VC7 hoists the inlined camera-control globals load above the `input->throttle.j` store
       (`mov edx,[globals]` before `add esp,0x10`; `mov al,[edx+0xc]; test al,1`).
     - In `initialize_for_new_map`, it lets the direct `player_control_globals->field` stores run ahead of the lazy
       `push edi`.
   - These were the two last "unexplained schedules".
2. **The 1/255 multiplier is a TU `static real const` defined right after the MOUSE constants.**
   - January's `.rdata` section holds `MOUSE_YAW_SCALE, MOUSE_PITCH_SCALE, 3b808081` in 12 bytes. The fire-button
     relocation at @0x92c targets that section at +8, not a `__real` COMDAT.
   - The static const reproduces the section bytes and the relocation exactly. This is the same pattern as
     decals/projectiles `seconds_per_tick`.
   - The name `ANALOG_BUTTON_SCALE` is descriptive. January keeps no name for this object, HCEA has none, and the name has
     no effect on bytes.
3. **Type shape.** January's yaw/pitch look deltas occupy one adjacent 8-byte frame object (-0x3c/-0x38).
   - Its pitch sub-slot later hosts the button and mouse temporaries.
   - One `real_euler_angles2d look_delta` reproduces every cell from -0xc to -0x28 and the temporaries' placement. The two
     scalars do not.
4. **Measured VC7 law: the lifetime of an address-taken aggregate is its lexical scope.**
   - January's `target_angular_velocity` (-0x34) overlaps the `pitch_spin_scale`/`player_information` cell. That is only
     possible if its scope starts after that cell's last use.
   - Declaring `target_angular_position`/`target_angular_velocity` in a block that starts at the `local_player_aim_assist`
     call and encloses the magnetism `if` gives frame 0x48 and zero slot mismatches over all 1113 instructions.
   - Hoisting either aggregate to the gamepad or function scope is worse.
5. The waveD findings were re-confirmed by ablation:
   - G1: the pegging `>=` branch sense, with the acceleration arm first.
   - G2: `yaw *= speed; pitch *= speed;` followed by the two PIN clamps.

Ablation on the final candidate (each change removed alone):

| Removed | Result |
| --- | --- |
| G1 | 6 structural differences |
| G2 | 1114 instructions |
| `look_delta` struct | 94 slot mismatches |
| aim-assist block | frame 0x4c / slot swaps |
| static const | relocation identity |
| static global | mouse-path schedule |

Every change is required.

### `initialize_for_new_map` shapes under the static global

| Shape | Result |
| --- | --- |
| local `globals`, source order | push hoisted, 4 differences |
| local, `action_test_flags` first | push hoisted |
| direct stores, `action_flags` first | push correct, store order +0,+4 |
| inlined `player_control_action_test_reset()` + stores (local or direct) | store order +0,+4, fixed by reset's own exact body |
| chained zero | 4 differences |
| direct stores `action_test_flags, action_flags, suppressed_action_flags, flags` | **EXACT** |

With the direct form, store order equals source order: the +0-first and +4-first shapes emit exactly their source order.
January's +4,+0,+8,+c is therefore the statement order, recovered from the bytes.

## ORCHESTRATOR-PROPOSAL (required before unparking)

1. `source/game/player_control.h`: delete the line `extern struct player_control_globals_data *player_control_globals;`.
   - Consumers: 21 `.c` files include the header:
     - ai_debug
     - bored_camera, director, editor_flying_camera, following_camera
     - player_effects
     - aim_assist, game, game_engine, players, player_control
     - hs_library_external
     - input_abstraction
     - first_person_weapons, hud, hud_draw, hud_weapon, interface, motion_sensor, ui_widget
     - main
   - Only `player_control.c` uses the symbol.
   - Scratch shadow-header proof (`scratch/workers/player_control/shadowcheck.py`): each of the 20 other consumers was
     compiled with a copy of the header lacking that line. The per-function tinfo JSON is byte-identical in every unit
     (ai_debug gated without the guard because of its pre-existing COMDAT).
   - `c5_shadow_header.c` (the candidate including the shadow header) still gates `exact 48 residual 1`.
2. `config/symbols.json` line 22510, edited in place:
   `{ "file_offset": 4451888, "flags": 0, "name": "_player_control_globals", "static": true },`.
   This matches January's PDB-proven internal linkage, and our object now defines it as a static `.bss` symbol instead of
   COMMON.
3. Optional, names only: add a static label for the anonymous constant after line 14776:
   `{ "file_offset": 2475572, "flags": 0, "name": "_ANALOG_BUTTON_SCALE", "static": true },`.
4. Unpark `_get_local_player_input_blob` and `_player_control_initialize_for_new_map` from `c4.c` in a dedicated commit,
   after 1 and 2.

Owner census of `c4` against `build/base` and the target:
- a new static `.rdata` label `_ANALOG_BUTTON_SCALE` inside the January-identical 12-byte MOUSE section (base: 8 bytes);
- `__real@3b808081` is no longer emitted, and January has none;
- `_player_control_globals` changes from COMMON to a static 4-byte `.bss` definition.

There are no other owner changes.

## Not landed: `_player_control_modify_desired_angles`

- The static global does not change it. Frame, slots and relocations are identical.
- The single remaining difference is one transposition in the `look_autolevel_scale` else arm: January emits
  `mov eax,[ebp+8]` before `fmul [edx+0x54]`.
- New shapes this wave, all worse:
  - one `interpolate_scalar` call with a `?:` argument (521 instructions);
  - a named `speed` local with one call;
  - `look_angles`/`look_vector`/`tilt` hoisted to the camera block (frame 0xb4);
  - `marker`/`marker_angles` at function scope (frame 0xb8).
- Classification: independent-instruction scheduling tie. The park criterion is unchanged.

## Do not repeat

- glpib lower-frame experiments that were inert:
  - single, pair and triple scope moves of `look_delta`/`target_angular_position`/`doubled_spin`/`yaw_spin_scale`/
    `pitch_spin_scale` to function, gamepad or look scope;
  - declaration order;
  - `player_information`/`unit` inline forms;
  - `inverse_zoom` written as a division;
  - adding 0-12 file-scope declarations;
  - `scale_vector2d` address-taking.
- Mouse-path shapes that were inert without the static global:
  - `!= FALSE` or `?1:0` normalisation;
  - an explicit TEST_FLAG condition;
  - an inverted `if`;
  - `mouse` scoped inside the `if`;
  - `camera_control_is_active` body variants, which break that function's own exactness.
- Do not remove the out-of-line `signed_angular_difference`/`interpolate_scalar` definitions. Doing so loses two exact
  functions. A cut-off earlier run's `v1.c` did this.

## Reopen criteria

- The two reopen proposals meet their park criteria. Each park asked for "authoritative January … records or a natural
  compatible-compiler donor that explains the schedule". The explanation is January's own PDB linkage record (static global)
  plus same-compiler measured laws (type shape; the lifetime of an address-taken aggregate is its lexical scope). No barrier,
  register steering, invented dependency or logic change is used.
- `modify_desired_angles`: unchanged. Reopen only with new local or source provenance for the else-arm schedule.
