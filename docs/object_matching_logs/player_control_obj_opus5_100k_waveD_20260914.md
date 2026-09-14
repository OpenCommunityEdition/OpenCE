# `player_control.obj` opus5 100k wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/game/player_control.c`, target `build/split/source/game/player_control.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, repository `build.ninja` cflags for this object as used by
  `tools/campaign/gate.py`).
- Evidence: January target COFF (instructions, relocations, `.rdata`), January PDB publics
  (`scratch/pdb200-current/pdb_symbols.json`), HCEA `handle_one_player_input.c`, `get_local_player_input_blob.c`,
  `player_control_modify_desired_angles.c`, `player_control_initialize_for_new_map.c` (names/order/semantics only),
  Stian January lift `src/halo/game/player_control.c` (including its January map extract). No atlas or Ghidra output
  was prepared for this object.
- All four residual functions are PARKED and locked. Every experiment was made in scratch copies under
  `scratch/workersD/player_control/`. The real source file is unchanged (`git diff --stat -- source/game/player_control.c`
  empty). Worker notes: `scratch/workersD/player_control.md`.
- Prior ledgers read: `player_control_obj_fable_salvage_admission_20260902.md`,
  `player_control_obj_input_cluster_completion_20260903.md`, `player_control_obj_jonas_aiming_unit_leaf_20260828.md`,
  `player_control_obj_jonas_private_cluster_census_20260830.md`.

## Validated baseline (lane HEAD)

`gate.py source/game/player_control --all --forbid-emitted-symbol _point_from_line3d`:
`== exact 45  residual 4  unwritten 0  (of 49 listed)`, guard passed. Residuals (target padded / relocs, parked base hash):

| Function | Padded | Relocs | Base sha16 | Target sha16 |
| --- | ---: | ---: | --- | --- |
| `_get_local_player_input_blob` | 3456 | 130 | 515cbede397cc244 | 4b4ad9ae985a33b5 |
| `_handle_one_player_input` | 1568 | 95 | 12698d608adb7172 | 490eb357b2d76f34 |
| `_player_control_initialize_for_new_map` | 144 | 10 | 782be32752d680a0 | adb32edd759e2bbc |
| `_player_control_modify_desired_angles` | 1584 | 83 | c61e9a2f2f567473 | 6e468bddb00f141c |

## Result: park-reopen proposal for `_handle_one_player_input` (strict EXACT in scratch)

Candidate: `scratch/workersD/player_control/hopi_final.c` (diff vs production: `hopi_final.diff`).
Whole-TU gate: `== exact 46  residual 3  unwritten 0`; point guard passed; the other three parks equal their
`measurements.base`; `_handle_one_player_input` equals `measurements.target` (1568 / 95 / 490eb357...); owner census
identical to `build/base/source/game/player_control.obj`; `tools/fake_match_scan.py` 0 leads (production also 0).

Source change (no logic change):

1. `struct player_action action;` moves from function scope into the
   `if (local_player_get_player_index(local_player_index) != NONE)` block, its only use.
2. The seven member copies are ordered `desired_facing, control_flags, desired_weapon_index, desired_grenade_index,
   desired_zoom_level, throttle, primary_trigger`, followed by a blank line before the 0x369 assert.

Evidence:

- First divergence at baseline was only the player_action copy block: January pipelines loads one ahead of stores
  (`mov ecx,[esi+4]; mov edx,[esi+0xc]; mov eax,[esi+0x10]; mov [ebp-0x48],ecx; mov cx,[esi+0x22]; ...`) while the
  function-scope local produced sequential load/store pairs.
- Scoping lever (units carrier scoping lever, measured again here): the inner-block declaration alone switches VC7 to
  the pipelined schedule. With the inner scope, the schedule is a deterministic function of statement position (two
  different orders produced the identical position permutation).
- Statement order: all 5,040 orders of the seven whole-member copies were measured
  (`sweep_hopi.py`, `sweep/struct7_results.json`). Exactly one is strict exact. It is HCEA's independent order
  (`primary_trigger, desired_facing, control_flags, weapon, grenade, zoom, throttle`) with `primary_trigger` last.
  This is order recovery from bytes, like case order from a jump table. It is not an authenticity claim for any other
  spelling.
- Assert anchors 0x35F and 0x369 leave nine source lines between them: blank + seven statements + blank, which fits
  the seven-statement form.
- Parked reopen criterion ("a natural compatible-compiler donor that explains the residual allocation schedule"): the
  schedule is explained by the same-compiler scoping law plus the byte-proven statement order. No barrier, register
  steering, invented name or logic change is involved.

## Experiment matrix (scratch only)

| Function | Shape | Result | Disposition |
| --- | --- | --- | --- |
| handle_one_player_input | h1 HCEA copy order, function-scope local | sequential pairs, 16 diff blocks | reverted |
| handle_one_player_input | h2 inner-block `action` | pipelined schedule appears, 13 blocks | kept as base |
| handle_one_player_input | h3 inner block + HCEA order | pipelined, 14 blocks | reverted |
| handle_one_player_input | inner block + 5,040 orders | exactly 1 strict EXACT (784) | park-reopen proposal |
| initialize_for_new_map | chain2 `action_flags = action_test_flags = 0` | stores +4,+0 but `push edi` still hoisted | reverted |
| initialize_for_new_map | stores in +4,+0,+8,+c source order | same as chain2 | reverted |
| initialize_for_new_map | chain4 | stores reversed, push hoisted | reverted |
| initialize_for_new_map | I1 `constants` at function scope (scoping lever) | inert (only push placement) | reverted |
| modify_desired_angles | S1 `scale * magnitude * error` | inert (1 instruction swap) | reverted |
| modify_desired_angles | S2 named `magnitude` local (Stian) | worse: sqrt moves before the compare | reverted |
| modify_desired_angles | S3 paren law `(constants->look_autolevel_scale)` | inert | reverted |
| modify_desired_angles | M1 `pitch_autolevel` at function scope | inert | reverted |
| modify_desired_angles | M2 `error` declared then assigned | inert | reverted |
| get_local_player_input_blob | G1 `fabs(clamped_yaw) >= look_pegging_threshold` with the acceleration arm first | closes the floating branch block exactly | scratch only (park locked) |
| get_local_player_input_blob | G2 `yaw *= speed; pitch *= speed; yaw = PIN(yaw); pitch = PIN(pitch)` | closes the magnetism clamp block exactly | scratch only (`glpib_g2.c`) |

## Residual classification (remaining parks)

- `_player_control_initialize_for_new_map`: callee-save `push edi` placement tie. The store order follows source order;
  the push placement did not move for any natural shape. Scheduling, unresolved.
- `_player_control_modify_desired_angles`: one-instruction independent-instruction scheduling tie in the
  look_autolevel_scale else arm (integer argument load vs x87 multiply).
- `_get_local_player_input_blob`: measured facts: (a) January multiplies `buttons[_button_fire]` by 1/255 from an
  anonymous 4-byte `.rdata` slot at `MOUSE_YAW_SCALE+8`, in the same section as the two public MOUSE constants, with no
  public name. Reproducing it needs a TU static constant whose name cannot be authenticated. (b) Frame-slot coloring
  permutation (frame 0x48 in both). (c) The inlined camera-control test on the mouse path is scheduled as
  `mov al,[edx+0xc]; test al,1`, with the globals load hoisted. G1 and G2 are proven improvements for a future reopen.

## Do not repeat

- init_for_new_map: store reorders, chained zero stores and constants scoping do not move `push edi`.
- modify_desired_angles: operand reorders, paren wrapping, a named magnitude local and local scoping of
  pitch_autolevel/error are inert or worse.
- handle_one_player_input: do not re-sweep. The unique order is recorded above.

## Reopen criteria

- initialize_for_new_map / modify_desired_angles: unchanged from `config/parked.json`.
- get_local_player_input_blob: authoritative January name for the anonymous 1/255 `.rdata` constant, plus scoping
  evidence that explains the slot coloring. Start from G1+G2 (`scratch/workersD/player_control/glpib_g2.c`).

## Disposition

- No real-file change; no newly exact function is credited in this wave.
- ORCHESTRATOR park-reopen proposal: `_handle_one_player_input` (1568 padded bytes) from `hopi_final.c`.
