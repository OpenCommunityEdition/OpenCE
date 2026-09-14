# `editor_flying_camera.obj` second-pass park re-investigation (Opus5 150K house-clean lane, wave w3, 2026-09-14)

## Result

No real-file change and no new exact credit. `source/camera/editor_flying_camera.c` stays at **17 exact / 4 residual /
0 unwritten** (guard passes, zero park drift, owner census unchanged, fake scan 0 leads).

The wave narrows the blockers. Three of the four parks now depend on only one thing: the orchestrator's HELD ruling on
the HCEX static split (`is_scripted` anchor plus `local_player_index`). No `_point_from_line3d` COMDAT and no disputed
cast class is still involved.

| Function | January padded/relocs | w1 blocker | w3 blocker |
| --- | ---: | --- | --- |
| `_editor_camera_flying_update` | 1408 / 60 | split + `_point_from_line3d` COMDAT ruling | **split only**: exact with `add_vectors3d`, guard passes |
| `_editor_camera_set_scripted` | 432 / 40 | split + cast ruling | **split only**: point/vector casts are in the w2 admitted class |
| `_editor_camera_update` | 304 / 26 | split | split (unchanged, not re-proposed) |
| `_editor_camera_set_position_and_roll` | 432 / 30 | cast + x87 tie | split + euler3d->euler2d prefix cast + 3 x87 operand-rank blocks |

## Scope and provenance

- Baseline real-file gate `gate.py source/camera/editor_flying_camera --all --forbid-emitted-symbol _point_from_line3d`:
  17 exact / 4 residual / 0 unwritten. The orbiting park was already reopened by w1 P1 (commit 1a8ceaf44).
- Prior ledgers read: `editor_flying_camera_obj_opus5_150k_w1_20260914.md`, `_unwritten_packet_20260912.md`,
  `_owner_graph_reconciliation_20260908.md` and the older jonas ledgers. Also read: the w1 worker notes, the 150K HANDOFF
  (split HELD), and `scratch/w2/laws_w2.md`.
- `branch_sweep`: no better blob. Worker notes: `scratch/workers/w3_observer_editor_flying_camera.md`. Scratch dir:
  `scratch/workers/w3_observer_editor_flying_camera/`.
- Evidence used:
  - January COFF bytes and relocations (alndiff);
  - HCEA `blam/camera/editor_camera_set_scripted.c:46` (`(const real_point3d *)&unit_offset`);
  - HCEA `editor_camera_set_position_and_roll.c` (`*(const real_euler_angles2d *)angles`, `agreement` local);
  - the HCEX compiland static list from w1.

## Per-function outcomes

### `_editor_camera_flying_update` (park): premise refuted, split-only

**January evidence.** The unit branch is `fld unit_offset.i; fadd [eax+0x50]; add eax,0x50; ... fadd [eax+4]; fadd [eax+8]`.
This is a lazily materialised pointer argument of an inlined helper, with the component order "offset + center". Two
helpers produce it:
- `real_math.h add_vectors3d(a, b, result)` gives `result->i = a->i + b->i`, i.e. `fld a.i; fadd b.i`, with
  `a = &unit_offset` and `b = center`;
- `point_from_line3d` (the w1 form) does too, but it emits the forbidden COMDAT.

Law A4 (helper routing) applies.

| shape | result |
| --- | --- |
| f1: split + unit branch `add_vectors3d(&unit_offset, (real_vector3d *)&unit->object.bounding_sphere_center, (real_vector3d *)&position)` | 1408/60 `1e60e849` EXACT; alias gate 19/2; guard PASS |
| f2: f1 + parallel else branch `add_vectors3d(&translation, (real_vector3d *)&camera->position, (real_vector3d *)&position)` | EXACT; preferred, since both arms use one helper (avoids R4) |
| f2nosplit: the helper change on the current real file | 1408/60 `57b287db`; the only delta left is the `use_roll` load placement, which needs the split |
| c3: also `add_vectors3d(&unit_offset, &translation, &unit_offset)` for the accumulate | flying residual; January's accumulate is component-wise `+=` (rejected) |

**Census.**
- A surplus SELECT_ANY `_add_vectors3d` is emitted. This is the systemic class: 11 base objects emit it while their
  January objects leave it undefined, and January's `actor_combat.obj` defines it.
- `_set_real_point3d` is gone (January does not define it).

### `_editor_camera_set_scripted` (park): split-only

- January uses `sub esp,8` and pushes `_editor_camera_globals+0x20` directly, i.e.
  `(real_point3d const *)&unit_offset` (the same cast HCEA uses).
- Point/vector pointer casts are now in the w2 verifier-admitted list, so the w1 cast blocker is lifted.
- In the combined candidate `efc_c1.c` it is 432/40 `925a9c06` EXACT (alias gate).
- The else arm's single camera fetch needs the non-address-taken static (split).

### `_editor_camera_update` (park): held

- It is EXACT in `efc_c1.c` (alias), as w1 measured. The split is HELD; it is not re-proposed here.
- No split-free lever exists: the early `push esi` and single camera fetch are the signature of a non-address-taken
  static. A local alias pointer would be synthetic.

### `_editor_camera_set_position_and_roll` (park): not landed

- `c2` (split + `if (!editor_camera)` first + `(real_euler_angles2d const *)angles`, the HCEA form) gives 432/30
  `bbb3b3495ba0323e`.
- Three x87 blocks remain:
  - the `cross_product3d` i-term second product: January `fld rotation.up.j; fmul up.k`;
  - the `dot_product3d` k and j products: January `fld roll_reference; fmul facing`.
- Shape s1: a named `real roll_sign = dot_product3d(...)` before the `angle_between_vectors3d` statement (HCEA
  `agreement`; x87 L-A law). Byte-identical to c2, so inert.
- w1 had already spent `spr_a1` and the argument swap `spr_a2`.
- Classification: commutative x87 operand rank. The YOUNGER OPERAND law gives no consistent spelling.

## Combined measurement candidate (not landable)

`scratch/workers/w3_observer_editor_flying_camera/efc_c1.c` is the current real file plus:
- the w1 HCEX static split;
- `add_vectors3d` in both flying position sums;
- the set_scripted point cast.

Gate results, with `--alias is_scripted=editor_camera_globals` standing in for the held symbols.json rename:
- 20 exact / 1 residual; all 17 baseline rows kept; guard pass;
- `parkcheck`: flying, update and set_scripted drift to their target SHAs;
- fake scan 0;
- census: the systemic `_add_vectors3d` COMDAT plus the 11 static `.bss` labels w1 already disclosed.

## Other checks

- C4013/C4133 census:
  - `terminal_printf` C4013 is in the EXACT `_editor_camera_bump_speed`. `#include "interface/terminal.h"` is neutral
    (17/4, drift 0, `efc_term.c`). Not landed, because no function gains.
  - The C4133 pair at the `scripted_camera_update` call is a pointer tag-name mismatch only (camera_scripting.h
    declares `dead_camera` / `camera_control` / `scripted_camera_command`).
- Final real-file gate identical to baseline row by row; parks 4 drift 0; owner census unchanged; fake scan 0 leads.

## Reopen criteria

| Function | Reopen when |
| --- | --- |
| flying_update | the held split ruling (the COMDAT ruling is no longer needed); candidate `efc_c1.c` |
| set_scripted | the held split ruling; candidate `efc_c1.c` |
| update | the held split ruling; candidate `efc_c1.c` |
| set_position_and_roll | split ruling + euler prefix-cast ruling + a spelling that flips the three product loads |

## Do-not-repeat (this wave)

- `add_vectors3d` for the `unit_offset` accumulate (January is component-wise).
- The named dot local in set_position_and_roll (inert).
- `terminal.h` include as a lever (neutral).
