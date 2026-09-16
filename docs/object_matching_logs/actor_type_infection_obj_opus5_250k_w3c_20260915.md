# `actor_type_infection.obj` opus5 250K house-clean lane, wave w3c (2026-09-15)

## Result

No production change. `source/ai/actor_type_infection.c` is byte-for-byte unchanged
(`git diff --stat` empty). The object stays at 4/5 strict exact functions.

| Function | January padded / relocs / sha16 | Real file (baseline = final) | Disposition |
| --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e...` | exact | inherited |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e0...` | exact | inherited |
| `_infection_wander_move_time` | `96 / 5 / 2b2b9cfb...` | exact | inherited |
| `_infection_wander_pause_time` | `96 / 5 / b4122501...` | exact | inherited |
| `_infection_swarm_control` | `3616 / 104 / 64eacc5c...` | `3360 / 103` residual | NOT-LANDED (backend tie) |

Gate (baseline and final): `== exact 4 residual 1 unwritten 0`, `_point_from_line3d` guard passed.
No parked functions in this unit, so no park drift is possible. `fake_match_scan`: 0 review leads.

## Duplicate prevention

- Ledgers read: `_jonas_helper_pair_20260825`, `_jonas_swarm_jump_recovery_20260827`,
  `_jonas_exhaustive_census_20260830`, `_aim_jump_owner_reconciliation_20260909`,
  `_opus5_fresh_graphs_20260914` (including its lead admission correction) and
  `_opus5_150k_w1_20260914`; worker log `scratch/workers/actor_type_infection.md`.
- `git log --all -- source/ai/actor_type_infection.c`: 15 refs, newest `82330421f`.
- `branch_sweep.py`: 13 unique blobs; the current file is the unique best (4 exact / 1 residual).
- Standing lane constraint: `_infection_swarm_control` is an **already-written residual**, so under
  brief section 6 it can only land when strict EXACT. The prior lane's improved 3600/104 body was
  rejected for exactly this reason and reverted; it is research only.

## Law detector pass (w2 A1-A22, w3 A23-A45)

Run over the only non-exact row, using the prior lane's research body
`scratch/workers/actor_type_infection/r3600.c` as the measurement base (re-gated here at
3600/104, 32 normalized diff blocks / 151 instructions; the real file is 810).

**A5 (alias/accessor re-fetch) fired, and the shape was untried.** January loads
`swarm_component->flags` once at the switch join (`0xbfd  mov cx, word ptr [esi + 2]`) and reuses
that register for four separate operations: the jump-pending test (`test cl,0x10`), the attached
test (`test cl,2`), the melee-state arithmetic (`and cl,1; shl cl,2` = `state ? 4 : 0`) and the
read-modify-write of the field (`or ecx,1` / `and ecx,0xfffffffe` / `mov [esi+2],cx`). The
candidate re-fetched the field at `0xc50 (mov al,[esi+2])`. HCEA
(`halocea-review/src/blam/units/infection_swarm_control.c:757`) independently shows
`int16_t component_flags = component->flags;`; January's load position (before the jump test, not
between the two ifs) fixes the C89 spelling as a scope block that encloses both ifs.

Laws that did NOT yield an admissible shape: A35 (leaf age) explains the three x87 clusters but is
diagnostic only, and both measured closures of this exact `k,j,i -> j,k,i` family used forms this
lane forbids (R15 hand-expanded helper, R14 `_point_from_line3d` COMDAT). A43 (cross-jump survivor)
is a negative result and covers the end-region block duplication. A24/A6 do not fire: the frame
(`sub esp,0xe8`) and every slot already match. A42 would require a redundant-store owner ruling and
is not needed to explain the bytes here.

## Shapes tried (scratch only, 2 of 5)

| Shape | Change | size/relocs | normalized diff blocks / insns | Kept |
| --- | --- | --- | --- | --- |
| r3600.c | prior-lane research body (measurement base) | 3600/104 | 32 / 151 | no |
| c1.c | A5: C89 scope block with `short component_flags = swarm_component->flags;` used by the jump test, the attached test and the melee ternary; the `SET_FLAG` write-back keeps the field lvalue | 3600/104 | **29 / 118** | no (not exact) |
| c2.c | c1 + `word control_flags = should_jump ? FLAG(_unit_control_jump_bit) : 0;` declared before `csmemset`, testing whether January's pre-call `neg al; sbb eax,eax; and eax,2; mov esi,eax` is source-reachable | 3600/104 | 29 / 118 | no -- **byte-inert** (instruction stream identical to c1); not proposed |

`c1` removes January's only genuine re-fetch difference in the end region and brings the packet
tail's register pair (`mov cl,[ebp-0x38]; mov dl,[ebp-0x60]`) to January's form. It does not change
size or the relocation multiset and is still not exact, so nothing landed.

## Residual decomposition after c1 (118 instructions, 4 clusters)

1. `0x20f` prop-scan delta magnitude: January `j^2` then `k^2` then `i^2`; ours `k,j,i`
   (A35(2) youngest-first from the single-expression `magnitude_squared3d` body). 4 instructions.
2. `0x874` inline `normalize3d` #1: January keeps `1/magnitude` live through the third component
   multiply (`fld [dir.k]; fmul st(1)`) and pops it at the join shared with the zero path
   (`fstp st(0)`); ours folds the last multiply destructively (`fmul [dir.k]`) and needs a `jmp`
   over the join pop. 3 instructions, +2 bytes.
3. Three `cross_product3d(&up, &direction, ...)` sites (`0x8e6/0x8ee`, `0x9c0/0x9ce`,
   `0xb59/0xb64`): only the two products containing `up.i` commute (January `fld dir.k; fmul up.i`
   and `fld dir.j; fmul up.i`), while `up.j x dir.i` loads `up.j` first in both builds. 12
   instructions. The asymmetry is inconsistent with both "younger leaf first" and "older leaf
   first", and `up` is never named in the source (it is a struct copy passed only by address), so
   there is no naming/aging lever to pull.
4. End region `0xbfd-0xd10`: register web -- January {flags in `cx`, jump boolean in caller-saved
   `al` converted to `2` before `csmemset` and parked in `esi`, unit pointer in `edi`}, ours
   {flags in `ax`, jump boolean in callee-saved `bl` converted after the call, unit pointer in
   `esi`} -- plus January's per-exit `mov al,1` duplication against VC7's single merged
   `mov bl,1` block. This cluster carries the entire 16-byte padded gap.

Nothing structural remains: frame size, relocation multiset, call set, loop spellings, switch
layouts, jump-table case targets and literals all match. Classification: **backend tie**
(x87 leaf order plus a post-allocation register web and cross-jump merge).

## Proposals

None. No header, `config/symbols.json`, `parked.json` or build-configuration need was found.

## Reopen criteria (supersede the w1 ledger)

Only a strict-EXACT body can land (written residual). Reopen only with:

1. an admissible source reason for the `up.i`-only operand commutation in `cross_product3d`
   (cluster 3) that keeps ordinary helper calls -- no hand-expanded helper body (R15) and no new
   candidate-only COMDAT owner (R14; `_point_from_line3d` forms are OUT in this lane); **and**
2. a source reason that keeps the jump boolean in a caller-saved register with per-exit `TRUE`
   materialisation (cluster 4).

Measured dead ends for (2): the w1 separate-`should_jump = TRUE;` else-if chain (p1) and this
wave's pre-`csmemset` `control_flags` local (c2, byte-inert). Any future attempt should start from
`scratch/workers/w3c_actor_type_infection/c1.c` (29 blocks / 118 instructions), not from
`r3600.c` (32 / 151), and must not re-spend the fresh-graphs do-not-repeat list.
