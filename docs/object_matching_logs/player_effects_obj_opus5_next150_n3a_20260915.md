# `player_effects.obj` — opus5 next-150K lane, wave n3a (2026-09-15), STRUCTURAL PARK PASS

## Result

| Measure | Baseline (HEAD) | Final |
| --- | --- | --- |
| Focused gate | 25 exact / 4 residual / 0 unwritten | 25 exact / 4 residual / 0 unwritten |
| `_point_from_line3d` guard | pass | pass |
| Parks (`scratch/parkcheck.py`) | 3 active | 3 active, drift 0 |
| `tools/fake_match_scan.py` | 0 leads | 0 leads |
| `git diff --stat -- source/effects/player_effects.c` | — | empty |

**Nothing landed, by design: the census closed the question instead.** The real file was never edited.

**Baseline correction for the lane board.** The wave packet lists this unit at `exact 24 / residual 5`
with `_player_effect_get_screen_flash` still `residual 800 [size 816!=800, sha]`. The measured gate at this
HEAD is `exact 25 / residual 4`: `_player_effect_get_screen_flash` is already strict EXACT in the real file
(landed by wave w3c, `player_effects_obj_opus5_250k_w3c_20260915.md`). The packet's row list is one wave
stale.

## Target: `_player_effect_get_camera_effect_matrix` (1312 padded / 1303 meaningful / 49 relocs) — PARKED

### The park premise, and what the census did to it

`config/parked.json`, class `unclassified`, records: *"Target and candidate carry the same ordered 49
relocation identities and the same 0x48-byte frame … **the candidate is 1280 padded bytes against
January's 1312**."*

Candidate `scratch/workers/n3_player_effects/v0.c` — the **current** real file with **only** this parked
body replaced by the w1 `pe_m6` shape (m1 `set_real_point3d` scripted translation, m2/m6 direct
`temporary_duration` / `camera_shake.duration` arguments, m3 two-pass impulse translation) — gates at
**1312 / 49 [sha]**, i.e. **January's exact padded size and relocation set**, with the whole-TU `--all`
listing identical row for row to baseline (`exact 25 residual 4`, no sibling loss, guard passes).

So **the recorded size premise is refuted**: the 32-byte gap was two source defects (the `real duration`
locals, and the one-pass `position.x = direction.i*translation + jitter.i*scale` form), both fixable in
admissible C.

### Frame census — January vs candidate

Both `sub esp, 0x48`. Slot-by-slot reference counts are identical for every slot
(`[ebp+8]` 18, `[ebp+0xc]` 6, `[ebp-4]` 17, `[ebp-8]` 27, `[ebp-0xc]` 1, `[ebp-0x10]` 1, `[ebp-0x14]` 2,
`[ebp-0x18]` 3, `[ebp-0x1c]` 3, `[ebp-0x48]` 6) **except `[ebp-0x20]` (`effect_matrix.position.x`):
January 1 reference, ours 3.** Ours lacks no slot and adds none — no missing local, no extra local, no
width gap.

### Relocation census by target

`relocdiff --count-by-target --allow-structural`: `49 target, 49 candidate, 0 differing row(s)`. Every
call and global matches by target and count. **No missing statement.**

### Therefore

The census **refutes the park's size premise** and at the same time **proves no structural source defect
remains**: no missing call, no absent or surplus frame slot, no wrong declared width, no wrong control
flow. What is left is 390 vs 389 instructions in two x87 clusters:

* **A — inlined `cross_product3d(global_up3d, &effect->direction, &axis)` (4 instruction slots).** January
  `+0x2cf fld [ebx+8]; fmul [eax]` and `+0x2d6 fld [ebx+8]; fmul [eax+4]`; ours loads the `global_up3d`
  leaf first in exactly those two products. The other four products and the k,j,i term order are
  byte-identical. A35(1) verbatim.
* **B — impulse translation (1 net instruction).** January keeps `translation * effect->direction.i` on the
  x87 stack across the `y` and `z` stores (`fld st(1)` … `fxch st(1)` … `fadd st(1)` … `fstp st(0)`); ours
  stores `position.x` and reloads it. `y` and `z` use a memory `fadd` in **both** builds, which is what
  proves the two-pass source shape.

### Shapes measured this wave (4, all scratch, all reverted)

| id | shape | result |
| --- | --- | --- |
| `v0.c` | current real file + the w1 m1/m2/m3/m6 body | **1312 / 49**, clusters A + B only, siblings unchanged |
| `v1.c` | `position.x = position.x + effect->jitter.i * scale;` (×3) instead of `+=` | byte-identical to v0 |
| `v2.c` | pass 1 reordered `y, z, x` | y/z stores reach January's offsets; **x still spills and reloads** |
| `v3.c` | `scale_vector3d` + `scale_vector3d` into a temp + `add_vectors3d`, with the admitted point→vector casts | **frame 0x48 → 0x54**; January's frame has no room for a temporary vector — helper routing through a temp is structurally refuted |
| `v4.c` | v0 **minus m1** (scripted translation as three direct `z, y, x` stores) | **cluster A vanishes (the cross product becomes byte-exact)** while the scripted block degrades to eval/store ×3 (393 insns vs 390) |

### The blocker, stated precisely

**(a) The two blocks are mutually exclusive.** January's scripted branch evaluates all three translation
products and *then* stores x, y, z (`+0x204..+0x229`) — only the `set_real_point3d` grouping reproduces
that (A4); the direct-store spelling interleaves eval and store (v4, and the parked body). But that
grouping is exactly what flips the two `cross_product3d` leaf loads in the **other arm of the same
if/else** — a distant count-driven tie inside one function (the declaration-count oracle, not leaf age:
the two blocks never execute together). v0 vs v4 measures the exclusive-or directly at this HEAD, which is
the first time it has been reproduced since w1. The only third grouping (three named product locals) is
refuted by the frame census, exactly as v3 was.

**(b) January never stores the x product.** `+=` (v0), `x = x + e` (v1), "x last" (v2) all reload from
memory, and a temp vector costs frame (v3). Every source form with January's property is asymmetric — one
component as a single combined expression, or a named `real` for x alone — i.e. a shape written to
reproduce bytes rather than source a programmer would write. None was tried, under house rule 8 and the
no-fake-matching gate.

**No reopen is proposed** (a reopen needs strict EXACT; an R11 size-changing "strictly closer" re-baseline
is not proposable). The park is untouched.

**Reopen criterion, sharpened.** The park's size, frame, slot and relocation premises are all satisfied by
`scratch/workers/n3_player_effects/v0.c` (1312 / 49, four differing instruction slots). Reopen needs
**(a)** a source form that groups three evaluations before three stores in the scripted branch *without*
the `set_real_point3d` inline, or an admissible count-neutral compensation for the flip it causes, **and
(b)** a natural source form in which `translation * effect->direction.i` is never stored to `position.x`
before the jitter add.

## Rows not spent

| Function | Padded | Classification | Why not spent |
| --- | ---: | --- | --- |
| `_player_effect_add_continuous_effect` | 320 | x87 evaluation-order tie, no structural delta | 5 shapes already spent (w1 ×3, w3c ×2); both leaves are the same memory operand so A35 cannot discriminate; outside the structural mandate |
| `_player_effect_start` (parked) | 736 | A35(1) offset-0 leaf-age tie, 2 instructions | identical size, frame, relocations and instruction count; no admissible untried shape (w1, w3c) |
| `_player_effect_update_camera_impulse` (parked) | 752 | x87 term order in inlined helpers | identical size, frame, relocations and instruction count; A24/A6/A31/A35/A46 run as detectors by w3c, none fires |

## Observation for the lane (no tree change requested)

Three of this unit's four residuals — `_player_effect_get_camera_effect_matrix` cluster A,
`_player_effect_start`, `_player_effect_update_camera_impulse` — are one defect class: the operand load
order of terms inside inlined `real_math.h` `cross_product3d` / `dot_product3d` bodies, with **no**
structural delta anywhere (identical frames, identical relocation sets, and identical instruction counts
in two of the three). That is a systemic inline-IL question (A35 / A41), not three independent per-site
source defects; per-site spellings are now spent across four waves (w1, the 0904 closeout, w3c, n3a).

## Proposals

None. No header, config, symbols.json or parked.json change is requested. No park reopen is proposed.

## Verification performed

- `python -B tools/campaign/gate.py source/effects/player_effects --all --forbid-emitted-symbol
  _point_from_line3d` → `exact 25 residual 4 unwritten 0`, before and after (real file never edited).
- `--source scratch/workers/n3_player_effects/v0.c --all` → identical row for row to baseline; the target
  row moves `[size 1280!=1312, sha]` → `[sha]` at 1312 / 49.
- `python -B -m tools.campaign.alndiff` and `relocdiff --count-by-target --allow-structural` for v0…v4;
  per-slot `[ebp±N]` censuses via `scratch/workers/n3_player_effects/slots.py`.
- `python -B scratch/parkcheck.py source/effects/player_effects <real.obj>` → `parks 3 drift 0`.
- `python -B tools/fake_match_scan.py source/effects/player_effects.c` → 0 review leads.
- No ninja, configure, pytest or git state command was run; no header, config or other source file was
  touched.

Details, byte evidence and the full census tables are in `scratch/workers/n3_player_effects.md`.
