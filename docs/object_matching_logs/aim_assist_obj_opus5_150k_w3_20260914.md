# `aim_assist.obj` Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/game/aim_assist.c`; target `build/split/source/game/aim_assist.obj`.
- Lane `opus/150k-house-clean-20260914`, HEAD `403108e28`. Compiler VC7 13.00.9254, flags unchanged.
- **The real `aim_assist.c` is unchanged.** No header, config, park or tool was edited.
- Prior ledgers read: `aim_assist_obj_jonas_helper_wave_20260821`, `_opus5_fresh_graphs_20260914`, `_opus5_150k_w1_20260914`,
  the w1 worker notes, the 150K handoff (P9: the cone owner header was rejected at zero credit), and `scratch/w2/laws_w2.md`.
- Worker notes: `scratch/workers/w3_players_aim_assist.md`. Candidates: `scratch/workers/w3_players_aim_assist/`.

## 2. Gates

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline | `== exact 13  residual 2  unwritten 1  (of 16 listed)`, guard passed |
| Final (real file unchanged) | identical listing; tinfo rows identical to the HEAD object |
| Parks | none for this unit |
| Owner census vs `build/base` | identical |
| fake scan | 0 leads |

Newly strict exact: **none**.

## 3. Per-function outcomes

| Function | January | Outcome | Shapes this wave |
|---|---|---|---|
| `_player_aim_projectile` | 832/42 | owner-blocked | 0 new. The w1 candidate, re-gated on HEAD, is 832/42 with 278==278 instructions; only the second `normalize3d` `push ecx` placement differs |
| `_aim_assist_compute_target` | 416/16 | not landed | k1 (non-static `__inline compute_attenuation`, the w1 lever), k2 (magnetism pairs swapped), k3 (no `cosine` local) |
| `_local_player_aim_assist` | 368/12 | not landed | l1 (pointer to `target.vector`), l4 (l1 plus early returns), l5 (l1 plus explicit relative components) |

## 4. Evidence and measurements

- **`player_aim_projectile`.** Site 1 follows a flushed `add esp,0x24` plus a test/jcc, and pushes early in both builds. Site 2 follows
  `collision_test_vector` with a pending 0x20 adjust (the merged `add esp,0x24` is identical). January defers the push until after the
  three impact stores. Law detectors A4, A10, A12, A14 and A20 found no construct. Every site-2 respelling is on existing do-not-repeat lists.
- **`aim_assist_compute_target`.**
  - January COFF storage classes: `_compute_attenuation` and `_compute_composite_attenuation` are external (class 2); ours are static (3).
    The atlas marks `compute_attenuation` inline. Under k1 the function is 152==152 instructions, and only the magnetism composite's
    evaluation order differs: ours evaluates the angle call first and puts the temp in angle's dying home `[ebp+0x18]`; January evaluates
    the distance call first, with the temp at `[ebp+0x14]`.
  - k2 swaps the composite argument pairs and is byte-identical to k1. The inliner canonicalises the call order, so source-order swaps
    are useless (do not repeat).
  - k3 is 448 [size], worse.
- **`local_player_aim_assist`.**
  - January frame 0x68. January registers: ebx holds `local_player_index`, split around the private `lea ebx`; edi holds
    `target_angular_position`; esi holds `target_angular_velocity`; `player` and `aiming_unit_index` live in the pointer-parameter homes.
  - l1 (`real_vector3d const *vector= &target.vector;` in a block, 9 uses, A5 counter-law) makes the whole x87 block January's under the
    A22 younger-operand law: hsq in i,j order, `fld [mem]; fmul st(r)`, rk consumed in place, and January's epilog pops. One difference
    remains, the pitch dot term order (ours i first, January j first). Frame and registers are unchanged.
  - l5 (explicit relative components) fixes the dot order but keeps rk alive (5 pops).
  - l4 (early returns) is 352 [size].

## 5. Do-not-repeat (additions)

- `aim_assist_compute_target`: composite argument-pair swaps (k2), removing `cosine` (k3).
- `local_player_aim_assist`: early returns with function-scope locals (l4), explicit relative components (l5).
- `player_aim_projectile`: all site-2 respellings (unchanged list).

## 6. Proposals

None new. P9 (cone owner header) stays rejected until the body is strict exact.

## 7. Reopen criteria

| Function | Reopen when |
|---|---|
| `player_aim_projectile` | a source explanation for the deferred site-2 push, which would give P9 strict credit |
| `aim_assist_compute_target` | January-grounded evidence that `distance`/`angle` stay live past the magnetism composite; apply together with k1 |
| `local_player_aim_assist` | evidence changing `target_angular_position`'s allocation weight; l1 (`scratch/workers/w3_players_aim_assist/l1.c`) is the x87 lead |
