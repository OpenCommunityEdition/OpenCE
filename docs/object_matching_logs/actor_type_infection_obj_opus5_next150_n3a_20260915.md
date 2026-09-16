# `actor_type_infection.obj` next-150K lane, wave n3a (2026-09-15)

## Result

No production change. `source/ai/actor_type_infection.c` is byte-for-byte unchanged
(`git diff --stat` empty). The object stays at 4/5 strict exact functions.

| Function | January padded / relocs / sha16 | Real file (baseline = final) | Disposition |
| --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e...` | exact | inherited |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e0...` | exact | inherited |
| `_infection_wander_move_time` | `96 / 5 / 2b2b9cfb...` | exact | inherited |
| `_infection_wander_pause_time` | `96 / 5 / b4122501...` | exact | inherited |
| `_infection_swarm_control` | `3616 / 104 / 64eacc5c...` | `3360 / 103` residual | NOT-LANDED (census confirms the recorded premise) |

Gate (baseline and final): `== exact 4 residual 1 unwritten 0`; `_point_from_line3d` guard passed.
`config/parked.json` has **no entry for this unit** (`parkcheck.py` prints
`no parks for source/ai/actor_type_infection`), so the wave's "currently PARKED" framing does not
apply: `_infection_swarm_control` is an ordinary written residual and no parked body was edited
anywhere. `fake_match_scan.py`: 0 review leads (real file and candidate).

## Duplicate prevention

- Ledgers read in full: `_jonas_helper_pair_20260825`, `_jonas_swarm_jump_recovery_20260827`,
  `_jonas_exhaustive_census_20260830`, `_aim_jump_owner_reconciliation_20260909`,
  `_opus5_fresh_graphs_20260914` (including its lead admission correction),
  `_opus5_150k_w1_20260914`, `_opus5_250k_w3c_20260915`; worker logs
  `scratch/workers/actor_type_infection.md` and `scratch/workers/w3c_actor_type_infection.md`.
- `git log --all --oneline -- source/ai/actor_type_infection.c`: 15 refs, newest `82330421f`.
- `branch_sweep.py`: 13 unique blobs; the current file is the unique best
  (`exact 4 residual 1 sizedelta 256`); 9 blobs compile-fail, the others are worse.
- Binding prior decisions (R13): the fresh-graphs 3600/104 body was rejected and reverted because an
  already-written residual may land only when strict EXACT; w1 recorded SKIPPED-EXHAUSTED; w3c spent
  2 of 5 shapes and classified the remainder a backend tie.

## Census (structural-first, run on the REAL FILE)

Objects: `scratch/workers/n3_actor_type_infection/real.obj` (current production file recompiled) and
`v0.obj` (= wave w3c's `c1.c`, re-gated here at 3600/104 — the best known body, used only as the
measurement base).

### Frame census

| build | `sub esp` | distinct `[ebp-N]` slots | instructions |
| --- | --- | --- | --- |
| January | `0xe8` | 61 | 1127 |
| real file | `0xe4` | 61, **13 of them differ** | 1052 |
| `v0` | `0xe8` | 61, **identical set** | 1121 |

Slots the real file has and January does not: `0x12 0x21 0x80 0xde 0xe2 0xe3`.
Slots January has and the real file lacks: `0x24 0x7c 0xcc 0xd6 0xda 0xdb 0xe8`.
`[ebp-0x24]/[ebp-0x20]/[ebp-0x1c]` is January's `up` local (`up = biped->object.up;`, three integer
moves at `0x113-0x124`); `0xd6/0xda/0xdb` are `unit_control_data` fields. The real file's 4-byte
frame gap is a local-set defect, and `v0` already reproduces January's slot set exactly.

### Relocation census by target (`relocdiff --count-by-target --allow-structural`)

Every call and global target matches 1:1 between the real file and January **except**:

| target | January | real file |
| --- | --- | --- |
| `_normalize3d` | 1 | 3 |

(The `__real@*` rows that print `0` on our side are the relocation-*spelling* class — our object
defines the literal as a `.rdata` select-any COMDAT where January references it undefined;
`relocdiff` prints the `symbolic=` equivalence on each of those rows.)

The real file's single missing relocation is therefore not a missing call but **two surplus
out-of-line `normalize3d` calls**: January inlines `normalize3d` at the first and third
`has_direction` sites and calls it out of line only at the second, which is reachable only when the
`if (has_direction)` block sits at member-loop level rather than nested inside
`if (parent == NONE) { if (!airborne) ... }`. `v0` already carries that hoist and matches the whole
104-relocation multiset.

### Census verdict

The census **confirms** the recorded premise rather than refuting it. The real-file `[size]` and
`[relocs]` tags are the already-diagnosed structural gap whose source fix exists and is verified in
scratch (`v0`: frame, slot set, relocation multiset, call set, loop spellings, switch layouts,
jump-table case targets and literals all match January). What blocks the row is the standing rule
that an already-written residual may land only when strict EXACT, together with four backend
clusters — not missing evidence.

## Residual at the best known body (`v0`): 118 instructions, 4 clusters

1. `0x20f` (4 insns) — inlined `magnitude_squared3d` in the prop scan; A35(2) term age, diagnostic.
2. `0x874` (3 insns, +2 bytes) — inlined `normalize3d` #1: January keeps `1/magnitude` live through
   the third component multiply and pops it at the join shared with the zero path; ours folds the
   last multiply destructively and needs a `jmp` over the join pop.
3. Three `cross_product3d(&up, &direction, ...)` sites (12 insns) — only the two products whose `a`
   operand is `up.i` commute.
4. `0xbfd-0xd18` register web — carries the **entire** 16-byte padded gap.

## New evidence this wave (decisive negatives — treat as do-not-repeat)

**N1. The `cross_product3d` header spelling is byte-proven, so cluster 3 can never be closed by
respelling `real_math.h`.** `action_obey.obj` owns an out-of-line January COMDAT copy of
`_cross_product3d` that is **strict EXACT** against our `real_math.h` body:

```
0009 fld [ecx];   000b fmul [eax+4];  000e fld [ecx+4]; 0011 fmul [eax];   0013 fsubp   ; k
0015 fld [ecx+8]; 0018 fmul [eax];    001a fld [ecx];   001c fmul [eax+8]; 001f fsubp   ; j
0021 fld [ecx+4]; 0024 fmul [eax+8];  0027 fld [ecx+8]; 002a fmul [eax+4]; 0030 fsubp   ; i
```

January's own helper loads the `a` operand first in every product, including `a->i*b->k`
(`fld [ecx]; fmul [eax+8]`) — exactly what our inline expansion emits in the infection body. The
alternative hypothesis (January's header writing the `a->i` terms as `b->j*a->i`) is refuted by
bytes. Future attempts on cluster 3 must stay inside the caller and must not touch `real_math.h`.

**N2. Our build already reproduces VC7's commutation where the operands are pointer-based, so this
is not a compiler-version gap.** `matrix_math::_matrix3x3_from_forward_and_up` is strict EXACT and
commutes 3 of its 6 cross-product products in both builds (`fld [edx+4]; fmul [ecx]` for `a->i*b->j`
with `a = ecx = up`, `b = edx = forward`). A tree scan over every `cross_product3d` consumer
(`scratch/workers/n3_actor_type_infection/crossscan.py`, 89 functions) shows that **every
strict-exact cross-product site in the tree uses register/pointer bases**, and every site whose
operands are `ebp`-folded locals is non-exact. The infection anomaly is the offset-0 clause already
recorded in laws_w3 **A35(1)**: with `ebp`-folded locals January prefers the older leaf (`up.j` beats
`dir.k`, `up.k` beats `dir.j`) except at `up`'s offset 0, where it loads `direction` first. `up` is
written once by a named struct copy and never re-aged, so no naming/scoping/aliasing lever remains,
and a `{j,k}` pair that resolves both ways proves the rule is not a field-index order.

**N3. Cluster 4's 16 bytes are exactly the tail duplication of `mov al,1`, and that duplication is a
machine-level cross-jump decision, not a source spelling.** January materialises `should_jump = TRUE`
at four separate exits (`0xc06`, `0xc3c`, `0xc47`, and the speculative `0xc50 mov al,1; jne 0xc57`),
loads the component flags into `cx`, keeps the boolean in caller-saved `al`, converts it *before* the
`csmemset` call (`0xcba neg al; push 0x40; push 0; sbb eax,eax; and eax,2; mov esi,eax`) and keeps
the unit pointer in `edi`. Ours cross-jumps to one `mov bl,1`, loads the flags into `ax`, keeps the
boolean in callee-saved `bl` across the call and converts afterwards (`0xcc8 neg bl` ...
`0xcf2 sbb ebx,ebx`). Region measurement: January `0xbfd..0xd18` = 283 bytes, ours
`0xbfd..0xd08` = 267 bytes — the entire 16-byte padded gap; after it the two tails are identical up
to the `edi`<->`esi` rename. w1's `p1` already wrote the four `should_jump = TRUE;` assignments as
separate statements and VC7 still cross-jumped them into one `mov bl,1` (1115 insns, worse); w3c's
`c2` (a `word control_flags` local computed before `csmemset`) was byte-inert because VC7 sinks the
single-use `?:` to its store. The only spellings that would force the pre-call conversion are
prohibited — assigning `control.control_flags` before the `csmemset` that wipes it (dead/nonsensical
store) or inventing a second use (fake dependency). Both allocations cost exactly one callee-saved
register across the call (January `esi` = flag word + `edi` = unit; ours `ebx` = boolean +
`esi` = unit), so this is a genuine allocator/cross-jump tie of the A43 class.

## Shapes tried

None. Triage under brief section 5 stopped before any shape: strict exact requires all four clusters
at once; cluster 4 alone carries the whole size gap and has no admissible source form (N3); cluster 3
is byte-excluded from the header (N1) with no caller-side lever left (N2); clusters 1-2 are x87
emission inside inlined `real_math.h` helper bodies (A35(2), diagnostic only). Spending the shape
budget on register/x87/scheduling ties is what the wave brief forbids. `v0.c` was re-gated only as
the measurement base for the census above.

## Checks

Real file untouched, so the final real-file gate equals the baseline
(`== exact 4 residual 1 unwritten 0`, all four siblings EXACT); `_point_from_line3d` guard passed;
no parked functions in this unit (no drift possible); owner census not applicable (no production
change) — for the record, `v0`'s owner set versus the real file adds only `__real@bd888889` and
`__real@bfc90fdb` (both present in January's own symbol table) and drops the non-January
`__real@3f6eeeef`, with no new code, helper, BSS or COMMON owner; `fake_match_scan.py` reports
0 review leads for both.

## Proposals

None. No header, `config/symbols.json`, `parked.json` or build-configuration need was found, and no
candidate here depends on a FINAL owner ruling.

Recorded for the owner only, **not re-proposed** (R13 binds): the production body differs from
January in 810 of 1127 instructions, has a 4-byte frame gap, 13 wrong frame slots and two surplus
out-of-line `normalize3d` calls, which is arguably "materially incomplete" under brief section 8, and
`scratch/workers/n3_actor_type_infection/v0.c` is a complete, coherent, strictly-closer replacement
(3600/104, 118 differing instructions, all siblings exact, owner census and fake scan clean). The
100K lane rejected exactly this landing, so this wave neither lands nor re-proposes it.

## Reopen criteria (supersede the w3c ledger, narrowed by N1-N3)

Reopen only when all of the following are answerable at once, or when a same-compiler donor explains
the whole `0xbfd-0xd18` register web:

1. an admissible caller-side source reason for the `up.i`-only (offset-0) operand commutation at the
   three inlined `cross_product3d` sites — `real_math.h` is now byte-excluded (N1) and no
   naming/scoping lever remains (N2);
2. a source reason that makes `should_jump` dead at the `csmemset` call without a dead store or a
   fake second use (N3);
3. a source reason for the inlined `normalize3d` #1 reciprocal staying live to the shared join.

Start any future attempt from `scratch/workers/n3_actor_type_infection/v0.c` (3600/104, 118 insns),
never from `r3600.c` (151 insns) or from the real file. Do not re-spend the fresh-graphs
do-not-repeat list, w1's `p1`, or w3c's `c1`/`c2`.
