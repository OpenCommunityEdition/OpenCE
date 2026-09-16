# `actor_type_infection.obj` next-150K lane, wave n4 (2026-09-15)

## Result

No production change. `source/ai/actor_type_infection.c` is byte-for-byte unchanged
(`git diff --stat` empty). The object stays at 4/5 strict exact functions.

| Function | January padded / relocs / sha16 | Real file (baseline = final) | Disposition |
| --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e…` | exact | inherited |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e0…` | exact | inherited |
| `_infection_wander_move_time` | `96 / 5 / 2b2b9cfb…` | exact | inherited |
| `_infection_wander_pause_time` | `96 / 5 / b4122501…` | exact | inherited |
| `_infection_swarm_control` | `3616 / 104 / 64eacc5c…` | `3360 / 103` residual | **NOT-LANDED** (census confirmed but already mined; residual is forbidden-category) |

Gate (baseline and final): `== exact 4  residual 1  unwritten 0`; `_point_from_line3d` emitted-symbol
guard passed. `config/parked.json` has **no entry for this unit** (`scratch/parkcheck.py` prints
`no parks for source/ai/actor_type_infection`), so no parked body exists here and no park drift is
possible. `tools/fake_match_scan.py`: 0 review leads.

## Duplicate prevention

- Ledgers read in full: `_jonas_helper_pair_20260825`, `_jonas_swarm_jump_recovery_20260827`,
  `_jonas_exhaustive_census_20260830`, `_aim_jump_owner_reconciliation_20260909`,
  `_opus5_fresh_graphs_20260914`, `_opus5_150k_w1_20260914`, `_opus5_250k_w3c_20260915`,
  `_opus5_next150_n3a_20260915`; worker logs `scratch/workers/actor_type_infection.md` and
  `scratch/workers/w3c_actor_type_infection.md`.
- `git log --all --oneline -- source/ai/actor_type_infection.c`: 15 refs, newest `82330421f`.
- `branch_sweep.py source/ai/actor_type_infection`: 13 unique blobs; the current file is the unique
  best (`exact 4 residual 1 sizedelta 256`); 9 compile-fail, `0ef1df6f` is 3 exact / 2 residual, the
  remainder are unwritten stubs.
- Binding prior decisions (R13): the fresh-graphs 3600/104 body was rejected and reverted because an
  already-written residual may land only when strict EXACT; w1 recorded SKIPPED-EXHAUSTED; w3c spent
  2 of 5 shapes and classified the remainder a backend tie; n3a ran the full census and spent 0 shapes.

## The wave's leverage, recomputed (method step 1)

`python -B -m tools.campaign.relocdiff source/ai/actor_type_infection _infection_swarm_control
scratch/workers/n4_actor_type_infection/real.obj --count-by-target --allow-structural`

| target | January | real file |
| --- | --- | --- |
| `_normalize3d` | 1 | **3** |

`TOTAL 104 / 103`; every other call and global target matches 1:1. The `__real@*` rows that print `0`
on our side are the relocation-**spelling** class, not missing references — the per-relocation listing
prints `['defined-noncode','.rdata','__real@…'] symbolic=['symbol','__real@…']` on each, i.e. our
object defines the literal as a `.rdata` select-any COMDAT where January references it undefined
(accepted systemic class, laws_w3 **A30**).

So the orchestrator's statement is exactly reproduced: **January owns nothing we lack; we own two
surplus `_normalize3d` calls.**

## Census verdict: a real extra-call defect, but already mined — and its fix is already verified

This is **not** a naming artifact: it is a genuine surplus-call defect. It is, however, **not newly
identified**. Wave n3a (same day, `…_opus5_next150_n3a_20260915.md`, section “Relocation census by
target”) published this identical table, named the source defect, and verified the fix.

Source defect in words: January inlines `normalize3d` at the first and third `has_direction` sites and
calls it out of line only at the second. That inline schedule is reachable only when the
`if (has_direction)` block sits at member-loop level rather than nested inside
`if (parent == NONE) { if (!airborne) … }`. The production file carries the nested spelling, which
also costs it January's `up` local (`up = biped->object.up;`, `[ebp-0x24]/-0x20/-0x1c`, three integer
moves at `0x113–0x124`) and leaves the frame 4 bytes short (`sub esp,0xe4` vs January `0xe8`, 13
`[ebp-N]` slots wrong).

Fix verified again this wave (method step 6). `scratch/workers/n4_actor_type_infection/v0.c`
(= n3a `v0.c` = w3c `c1.c`) gates `residual 3616 [size 3600!=3616, sha]` — **no `[relocs]` tag** — and
its post-fix census is `_normalize3d 1 / 1`, `TOTAL 104 / 104`. Frame size, slot set, call set, loop
spellings, switch layouts, jump-table case targets and literals all match January; all four siblings
stay EXACT.

## Why nothing could land

1. **Policy.** `_infection_swarm_control` is an already-written residual, so under brief §6/§8 it may
   enter production only when strict EXACT. The 100K/fresh-graphs lane rejected this exact 3600/104
   body for that reason and reverted it. R13 binds; n3a did not re-propose it and neither does n4.
2. **The remaining bytes are forbidden-category work.** After the structural fix the residual is 118
   instructions in 4 clusters:
   1. `0x20f` inlined `magnitude_squared3d` term order — x87, **A35(2)**, 4 insns, 0 bytes;
   2. `0x874` inlined `normalize3d` #1 keeps `1/magnitude` live to the join shared with the zero path —
      x87, 3 insns, +2 bytes;
   3. three inlined `cross_product3d(&up,&direction,…)` sites with `up.i`-only commutation — x87,
      **A35(1)**; byte-excluded from `real_math.h` by n3a **N1** (action_obey owns a strict-EXACT
      January COMDAT copy of `_cross_product3d` that matches our header body) and with no caller-side
      lever left (n3a **N2**, 89-function tree scan: every strict-exact cross-product site in the tree
      uses register/pointer bases, every `ebp`-folded-local site is non-exact);
   4. `0xbfd..0xd18` register web plus per-exit `mov al,1` duplication — register allocation and
      cross-jump, **A43** (negative result). January 283 bytes vs ours 267 = **the entire 16-byte
      padded gap**.

   Strict exact needs all four at once, and cluster 4 alone carries the whole size gap. The only
   spellings that would force January's pre-`csmemset` conversion
   (`neg al; push 0x40; push 0; sbb eax,eax; and eax,2; mov esi,eax`) are a dead store into a field the
   `csmemset` then wipes, or an invented second use — both PROHIBITED. Measured dead ends: w1 `p1`
   (four separate `should_jump = TRUE;` statements; VC7 still cross-jumped to one `mov bl,1`, 1115
   insns, worse) and w3c `c2` (`word control_flags` computed before `csmemset`; byte-inert, VC7 sinks
   the single-use `?:` to its store).

   Clusters 1–3 are x87-ordering work and cluster 4 is register-allocation/cross-jump work; the OWNER
   DIRECTIVE 2026-09-15 forbids both in this wave.

## Shapes tried

None (0 of 5). Triage under brief §5 and the wave directive stopped before any shape. `v0.c` was
compiled only as the measurement base for the post-fix census above.

## Checks

Real file untouched, so the final real-file gate equals the baseline (`== exact 4 residual 1
unwritten 0`, all four siblings EXACT); `_point_from_line3d` guard passed; no parks in this unit
(`parkcheck.py`: `no parks`), so no park drift is possible; owner census not applicable (no production
change) — for the record, `v0`'s owner set versus the real file adds only `__real@bd888889` and
`__real@bfc90fdb` (both present in January's own symbol table) and drops the non-January
`__real@3f6eeeef`, with no new code, helper, BSS or COMMON owner; `fake_match_scan.py` reports 0
review leads for the real file.

## Proposals

None. No header, `config/symbols.json`, `config/parked.json` or build-configuration need was found,
and no candidate here depends on a FINAL owner ruling.

Deliberately **not re-proposed** (R13, no new evidence since the adjudication): landing `v0.c` in
production as a §8 replacement for a “materially incomplete” body. For the owner's record only, the
production body differs from January in 810 of 1127 instructions, has a 4-byte frame gap, 13 wrong
frame slots and two surplus out-of-line `normalize3d` calls, and
`scratch/workers/n4_actor_type_infection/v0.c` is a complete, coherent, strictly-closer replacement
(3600/104, 118 differing instructions, siblings exact, owner census and fake scan clean). The 100K
lane rejected exactly this landing.

## Reopen criteria (unchanged from n3a; N1–N3 still bind)

Reopen only when all of the following are answerable at once, or when a same-compiler donor explains
the whole `0xbfd..0xd18` register web:

1. an admissible caller-side source reason for the `up.i`-only (offset-0) operand commutation at the
   three inlined `cross_product3d` sites — `real_math.h` is byte-excluded (N1) and no naming/scoping
   lever remains (N2);
2. a source reason that makes `should_jump` dead at the `csmemset` call without a dead store or a fake
   second use (N3);
3. a source reason for the inlined `normalize3d` #1 reciprocal staying live to the shared join.

Start any future attempt from `scratch/workers/n4_actor_type_infection/v0.c` (3600/104, 118 insns),
never from `r3600.c` (151 insns) or from the real file. Do not re-spend the fresh-graphs
do-not-repeat list, w1 `p1`, or w3c `c1`/`c2`.
