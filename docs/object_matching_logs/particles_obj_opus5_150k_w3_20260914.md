# `particles.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Result

| gate (`--forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (lane HEAD `403108e28`) | 18 | 2 | 0 |
| final real file | **19** | 1 | 0 |
| scratch candidate with the proposed park reopen (`nf1.c`) | 20 | 0 | 0 |

Newly strict EXACT in the real file: `_particle_update_physics` (928 padded, 35 relocations, normalized sha16
`2713b2fb111b6b63`). The row diff against the baseline listing changes only that row. The guard passes, the parked
`_particle_next_frame` does not drift in the real file, the owner census against `build/base` shows no new or lost owner,
and `tools/fake_match_scan.py` reports 0 leads (0 at HEAD). No header or config change.

## `_particle_update_physics` — EXACT (landed)

Baseline: 944 padded / 36 relocations (w1 best scratch `p_pu2.c`: 960/36). The w1 ledger had recovered the `< 0.0625f`
then-arm and the direct field spelling in the attached branch, but the shared `particle_die` block and the
particle_index/definition register homes were left open.

January evidence from the aligned listing:

1. `mov byte [ebp-1],0` (settled) is emitted after the at-rest `jne` and between the `'pphy'` tag_get pushes, so `settled` and
   `physics` are initialised inside the not-at-rest block, settled first (law A6 scope; house rule 17 declare+init).
2. `particle_index` lives in EBX/[ebp+8], split with the velocity address `lea ebx,[edi+0x48]`, and `definition` is reloaded
   into ESI right after `point_physics_update`. Once the locals are block-scoped this also reproduces the single shared
   `particle_die` block (35 relocations) and January's block layout.
3. The collided-with-structure value is `and eax,4`, homed at [ebp-0x18] and re-tested with `test eax,eax`. That is VC7's CSE of
   `TEST_FLAG(collision_flags, _point_physics_collided_with_structure_bit)` used twice. A named `unsigned long` local (built from
   a hand `(unsigned)FLAG(...)` mask) takes ESI and starves `definition`.
4. The direction copy loads `.i`, `.j`, lea, store, load `.k`. That is a struct copy from `particle->translational_velocity`,
   not through a `velocity` pointer alias (law A5 alias removal; the address is a CSE).

Shapes (whole-TU gates; all siblings exact in every row):

| shape | change | result |
| --- | --- | --- |
| p0 | w1 `p_pu2.c` | 960/36 |
| p1 | p0 without the velocity alias | 960/36 (inert alone) |
| p2 | p0 with physics/velocity/settled block-scoped | 928/35, 56 blocks |
| p4 | p2 with settled declared before physics | settled store matches |
| p5 | p4 with `TEST_FLAG` twice instead of the named collided local | only the direction-copy order left |
| p7 | p5 without the velocity alias | **strict EXACT**, landed |

The retained source is ordinary C: block-scoped declarations with initializers, `TEST_FLAG` instead of a hand mask,
`&particle->translational_velocity` / `particle->translational_velocity.<c>` field access, and the at-rest-then-arm polarity.

## `_particle_next_frame` (parked) — PARK-REOPEN PROPOSED

Law A3 (single exit) refutes the park's register-allocation premise.

- **January bytes.** January materialises `result = TRUE` into AL before the flags test and spends AL as the bit-0 mask
  (`mov cl,[esi+2]; mov al,1; test al,cl`). AL=1 survives to the frame_index>0 return. `mov al,1` is rematerialised only on
  the forward-increment path, where tag_block_get_element_with_size clobbers EAX. That is one `return result;` whose phi merges
  TRUE with `particle_next_sequence` results; VC7 duplicates the epilogues.
- **Parked body.** It returns from three mid-function sites, so VC7 constant-propagates TRUE and rematerialises it.
- **Candidate** `scratch/workers/w3_weather_particle_systems_particles_etc/nf1.c`:
  - Construction: the current real file plus only this body. The two early `return result;` sites become if/else arms and one
    `return result;` sits at the bottom. The statements and their order are unchanged.
  - Result: strict EXACT 208/8, sha16 `5b20757045cb3e9b`, 20/20 exact.
  - It is also exact when applied to HEAD's file (`nf_head.c`).
- **Checks.** Guard pass; owner census clean; fake scan 0. parkcheck reports drift to the target hash (expected).
- **Park criterion.** "Natural same-compiler TU-context evidence that explains the EAX/ECX lifetime choice" is met by the
  single-exit result phi. The w2 law A3 closed 10 functions by the same mechanism.

## Rejected / do-not-repeat

- update_physics: a velocity alias with block scoping (p2/p4, not exact); a named `collided_with_structure` local.

## Reopen criteria

- None for update_physics (exact).
- next_frame: orchestrator unpark with `nf1.c`.
