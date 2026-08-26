# C2.Dll backend attribution: the POGO hot/cold bridge (2026-08-26)

Research record. No matching credit is claimed and no production source
changed. This closes the open question in `tools/c2dbg32/BACKEND_MODULE_MAP.md`
— "profile with breakpoints ... to build a frequency map of hot pass code by
ADDRESS, empirically — do not trust the static module map for hotness" — and
supplies the named, empirically hot pass addresses the Units residual families
need.

## 1. Why every previous static breakpoint was cold

`C2.Dll` 13.00.9254.1 is itself **profile-guided-optimized with hot/cold block
splitting**. Each function's cold blocks — the assert/error paths, which are
exactly the code that references the module's `__FILE__` string
(`f:\vs70builds\9254\vc\Compiler\Utc\src\P2\<mod>.c`) — are relocated into a
separate cold region. Measured in this image:

| region | contents |
| --- | --- |
| `0x10701000`-`0x1075ffff` | hot code: **>99.7% of all execution samples** |
| `0x107b0000`-`0x1081ffff` | cold blocks: **every module `__FILE__` reference** |

So the string-cross-reference map can only ever find cold addresses, and a
breakpoint on one never fires during a successful compile. That is a property
of the binary's layout, not a mistake in the earlier method — and it is why
static entry detection "fails in the color.c/dag.c span": that span *is* the
cold region, reached only by branches out of hot code.

## 2. The bridge that names hot code

A hot function that can assert still branches to its own cold block. So:

1. locate every module `__FILE__` string VA;
2. find each `.text` reference to it and take the int3-delimited block
   containing that reference — the module's cold block;
3. scan all of `.text` for `call`/`jmp`/`jcc rel32` whose target lands inside
   one of those cold blocks;
4. the *source* of that branch is hot code belonging to that module.

Implemented in `hotcold_bridge.py` (scratch lab). Against a real
`source/units/units.c` compile profile — 3 compiles, 28.7M EIP samples, 21.0M
in C2, 349 distinct hot functions (`eip_profile.py`, tight-loop WOW64 sampler)
— this attributes **93% of all samples** to named modules:

```text
tuple 17.1%   dag 11.4%   color 10.1%   hash 7.6%   (unnamed) 7.2%
optimize 6.9% globdf 4.7% getattr 4.2%  globlopt 4.0% p2symtab 3.9%
factor 3.8%   globopt 2.9% fg 2.7%      except 2.2%  stack 1.7%
```

## 3. Named pass addresses for the Units residual families

The addresses below are hot, empirically confirmed, and ready to breakpoint —
the first time this campaign has had them.

| residual family | module | hot function | share | attribution strength |
| --- | --- | --- | ---: | --- |
| `unit_place` RMW-fold / store slot | **dag** (scheduler) | `0x107304d1` | 2.7% | **26 dag cold-branches, no competing module** |
| same | dag | `0x1072b4b4` | 1.5% | dag(21), color(4) |
| same | dag | `0x1072a4dd` | 1.5% | dag(10), color(1) |
| `code_00198fd0` value-arm tail-merge | **factor** (cross-jump) | `0x1070c2e9` | 2.7% | factor(7), except(4) |
| first-scratch / register coloring | **color** | `0x10780825` | 1.1% | color(15) |
| same | color/dag boundary | `0x10732284` | 1.9% | color(7), dag(6) |
| `euler` frame-slot policy | **stack** | rollup 1.7% | — | see `c2_hot_modules.json` |

`0x10701000` (13.9%, the arena allocator) is the shared allocator, not a pass;
it is useful only as a stack-mining profiler, as previously recorded.

## 4. The `unit_place` differential, and what it proves

A standalone repro (`repro_place2.c`, 63 lines, no project headers) reproduces
the real divergence faithfully: two dword flag RMW pairs plus a word frame
store around a branchless clamp. Its default form folds the object pair
(`or dword [esi+4],0x20000`); the real function folds the unit pair. Four
one-factor variants were measured:

| variant | result |
| --- | --- |
| default | object pair folds — see-saw of the real function |
| v1 pair order swapped | unit pair splits, object pair folds |
| **v2 object-flags load hoisted above the clamp** | **both pairs split, damage byte-or interposed, frame store INSIDE the unit pair — January's exact shape** |
| v3 both loads hoisted | both split but stores group at the end |
| v4 object pair before damage byte | object pair folds |

**So January's shape is reachable in isolation, and the lever is where the
object-flags load is bound relative to the clamp.** Applied to the real
`unit_place` in-situ (with and without the carrier's redundant dead
`death_frame_index` pre-assignment, both isolated), the transfer **fails**:
the real function keeps folding the unit pair, byte-identical to before. This
is the probe-fidelity limit of `research/register_tie_reachability.md` §7,
now measured on a scheduling tie: the repro reproduces the vocabulary *and*
the target schedule, but the in-situ DAG has different successor pressure and
the frame store is issued as soon as its input is ready.

Debugger differential on the fold-vs-split repro pair (the only breakpoint of
five that fires on so small a TU): `0x1072b4b4` takes **8 hits when folding
and 9 when splitting**, the extra call carrying `edx=8` inserted mid-sequence
(fold `4,4,6,6,4,16,16,6` vs split `4,4,6,6,8,4,16,16,6`). Disassembly of
that block shows a **ready-list unlink/commit**: it walks a list threaded on
`node+0x14`, unlinks node `ebp`, sets **bit 3 of the flag byte at `node+6`**,
and relinks it into the list head kept at `[esp+0x1c]`. That is the scheduler
committing one tuple — so the count differential is *downstream volume* (the
split build schedules one more tuple, the separate load), not the fold/split
verdict. Same lesson as `integer_coalesce_probe2.md` §2: hit counts track
enumeration, not decisions.

## 5. Honest status and the next step

No owner became exact from the compiler-internals route this session;
`units.obj` remains 175/189. What is now in hand that was not before:

- the reason every prior static breakpoint was cold (POGO splitting), so no
  future session repeats that sweep;
- a reusable attribution method plus tooling that names 93% of hot C2 code;
- the specific hot dag/factor/color addresses for the exact residual families;
- a faithful `unit_place` repro in which January's shape is *demonstrably
  reachable*, which downgrades that residual from "scheduler tie, mechanism
  unknown" to "dag slot choice, reachable in isolation, in-situ lever not
  found".

Next step, scoped: breakpoint `0x107304d1` (the unambiguous dag function) on a
compile large enough to exercise it — the tiny repros never reach it — with
the debugger's gate mode narrowing to the `unit_place` compile window, and
read the priority comparison that orders the frame store against the unit-flag
pair. The instrument, the address, and the fail-closed source harness are all
in place; what remains is the decode itself.
