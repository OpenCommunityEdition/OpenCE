# `actor_type_infection.obj` opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

No production change. `source/ai/actor_type_infection.c` is byte-for-byte
unchanged (`git diff --stat` empty). The object stays at 4/5 strict exact.

| Function | January padded / relocs / sha16 | Real file (baseline = final) | Disposition |
| --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e...` | exact | inherited |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e0...` | exact | inherited |
| `_infection_wander_move_time` | `96 / 5 / 2b2b9cfb...` | exact | inherited |
| `_infection_wander_pause_time` | `96 / 5 / b4122501...` | exact | inherited |
| `_infection_swarm_control` | `3616 / 104 / 64eacc5c...` | `3360 / 103` residual | SKIPPED-EXHAUSTED |

Gate (baseline and final): `== exact 4 residual 1 unwritten 0`, `_point_from_line3d`
guard passed. No parked functions in this unit. `fake_match_scan`: 0 leads.

## Duplicate prevention

- Ledgers read: `_jonas_helper_pair_20260825`, `_jonas_swarm_jump_recovery_20260827`,
  `_jonas_exhaustive_census_20260830`, `_aim_jump_owner_reconciliation_20260909`,
  `_opus5_fresh_graphs_20260914` (including its lead admission correction: the
  3600/104 structural rewrite was rejected because it was not strict exact, and reverted).
- `git log --all -- source/ai/actor_type_infection.c`: 15 refs, newest `82330421f`.
- `branch_sweep.py`: 13 unique blobs; current is the best (4 exact / 1 residual).

## Re-measurement of the rejected research candidate (scratch only)

The rejected 3600/104 candidate (`opus5-30k-fresh-graphs-20260914/scratch/workers/actor_type_infection/rejected_realfile_3600.c`,
copied to `scratch/workers/actor_type_infection/r3600.c`) still gates
3600/104 in this worktree, with the frame (`sub esp, 0xe8`) and relocation
multiset matching and all siblings exact. A relocation-spelling-normalized
`alndiff` leaves four independent clusters, none structural:

1. `0x20f`: prop-scan delta magnitude x87 order (January `jj+kk+ii`, candidate `kk+jj+ii`).
2. `0x874`: inline `normalize3d` #1. January keeps `1/mag` on the stack and pops
   it at the join; the candidate consumes it destructively (`fmul [dir.k]`) and
   jumps over the zero-path pop (+2 bytes).
3. `cross_product3d(&up, &direction, ...)` at three sites: only the `up.i`
   products commute (January `fld dir.k; fmul up.i`); `up.j`/`up.k` products do not.
4. End of the member-loop body (`0xbfd`–`0xdac`, carries the 16-byte padded gap):
   January materialises `should_jump = 1` separately at each true exit (four
   `mov al,1` blocks, the last `mov al,1; jne; mov al,[should_jump]`), reads the
   swarm-component flags once into `cx` and reuses them for the attached-arm melee
   state, keeps `should_jump` in `al`, and computes `control_flags`
   (`neg al; sbb eax,eax; and eax,2; mov esi,eax`) between the `csmemset`
   pushes, with the unit pointer in `edi`. The candidate merges one `mov bl,1`
   block, reloads the flags, keeps `should_jump` in callee-saved `bl` across the
   call, and uses `esi` for the unit.

No frame-size gap, missing call or relocation, or loop-spelling difference
remains, so the orchestrator's reopen condition (genuinely new structural
evidence) is not met.

## Shapes tried (scratch only)

| Shape | Result | Kept |
| --- | --- | --- |
| r3600 (rejected research candidate, re-gated) | 3600/104, 4 clusters above | no (not exact; written residual) |
| p1: `if (flag) should_jump = TRUE; else if (has_direction && ground_ticks >= 45) { if (...) should_jump = TRUE; else if ... }` | 3600/104, still `bl` and merged TRUE block, 1115 insns (worse) | no; also an identical-branch spelling |

## Proposals

None (no header or configuration need).

## Reopen criteria

Reopen `_infection_swarm_control` only with a same-compiler donor or an
admissible source reason for cluster 4 (caller-saved `should_jump`, per-exit
TRUE materialisation, pre-call `control_flags`) together with an explanation of
the `up.i`-only operand commutation (cluster 3) and the `normalize3d` #1 join
pop (cluster 2). The 3600/104 structural rewrite alone cannot land and must not
be re-spent.
