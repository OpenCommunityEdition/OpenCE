# The structural-versus-tie census: 74% of the parked pool is not tie-shaped

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

**Scope note.** Measurement only. Reads `build/split` and `build/base`, writes
nothing, recompiles nothing. No source outside Lane A's eleven translation units
was touched. Reproduce with `python scratch/orch/boardsplit.py`.

Companion to [the frame-mismatch census](claude_lane_a_frame_mismatch_census_20260920.md);
the two diagnostics are complementary and the worked example below shows why.

## The three tiers

Over all 300 non-exact functions on the board (333,973 padded bytes):

| tier | meaning | functions | bytes |
|---|---|---:|---:|
| **S1** | the emitted **reference multiset** differs - a missing or extra program reference | 73 | 108,533 |
| **S2** | references agree, the **real instruction count** differs | 132 | 138,544 |
| **T** | both already agree; only here is "tie" the default reading | 95 | 86,896 |

**Validated against the gate.** On Lane A's eleven translation units this census
now reports exactly 33 non-exact functions and `gate.py` reports exactly 33
residuals, with **no disagreement in either direction**. That check is what
caught the two errors described at the end, and it is the check anyone extending
this tool should run first.

Split by park status:

| tier | parked functions | parked bytes | classes |
|---|---:|---:|---|
| S1 | 39 | 47,029 | 31 unclassified, 3 register-allocation, 3 tu-context, 2 scheduling |
| S2 | 75 | 65,104 | 52 unclassified, 9 scheduling, 6 register-allocation, 6 tu-context, 2 private-convention |
| T | 55 | 36,160 | 25 scheduling, 23 unclassified, 6 register-allocation, 1 private-convention |

The twelve `csplit-relocation-alias` parks correctly vanish from this census
once aliasing is resolved: those functions ARE exact after alias resolution,
which is exactly what that park class records.

**114 parked functions - 112,133 of the 151,525 parked bytes, 74% - are not
tie-shaped.** They are parked as compiler ties over evidence that says our
source does not yet express the same program.

## What each tier does and does not prove

Being precise here matters more than the headline, because the obvious reading
of S1 is wrong in a way this lane proved from its own work.

**S1 is not proof that the missing reference is source-reachable.** The backend
can delete a reference all by itself. `_ai_communication_finished` sits in S1
with `d-reloc 1`, and nineteen gated source shapes across five hypotheses
established that the missing
`call _ai_communication_find_global_actor_to_talk` is a VC7 **cross-jump
selection**: both builds emit two resolution blocks and merge a different pair,
which deletes one call site. No source spelling reaches it. So S1 means *the
emitted reference multiset differs* - usually a source gap, sometimes a tail
merge. It is where to look, not a verdict.

**S2 is weaker still.** A pure backend decision changes instruction counts
routinely; the same cross-jump moved that function by 4 instructions with no
source difference at all. S2 is a prior, not a finding.

**Instruction counts are unreliable for bodies with an embedded jump table.** A
linear disassembler counts the table's dwords as instructions. Wherever a
function relocates against itself or a `_jmptable` symbol, ignore `d-insn` and
read the frame and the reference multiset instead.

## Four traps that make this census easy to get wrong

**Internal references are named differently by the two builds and are not a
program difference.** Our build emits `$L#####` jump-table labels; January's
split object relocates the same entries against **the enclosing function symbol
itself**, and names its table `<function>_jmptable`.

Counting either as a reference is catastrophic for this analysis. Before the
filter was right, `_ai_debug_render_actor` showed a **109-row reference
deficit** - on a function whose relocation count is **1,905 on both sides**.
Correcting it moved 255 functions between tiers. Any future tool doing this must
exclude, on both sides: `$L*`, `$SG*`, the function's own symbol, and
`<function>_jmptable`.

**Fourth: a relocation's TARGET NAME is not its target.** The same address can
be spelled against different symbols with different addends
(`_sense_ray_divergences+32` versus `_sense_ray_angles-4`). Compare resolved
addresses - or, as this tool now does, leave names out of the exactness test
entirely and use the relocation (address, type) list, which is what the
comparator effectively does.

**Third: compare NORMALIZED bytes, not raw bytes.** See the correction note at
the end - raw comparison pulled 238 already-exact functions into the census.

## The worked example: why you need both censuses

`_ai_debug_render_actor` (`source/ai/ai_debug`, 24,976 bytes - the largest single
residual in the AI portfolio) after the filter is corrected:

- section size: **24,976 on both sides**
- relocation count: **1,905 on both sides**
- external reference multiset: **identical, zero differences**
- real instruction counts: within a few of each other

By the reference census it is a pure tie. By the **frame** census it is
structural: `sub esp,0x810` against our `0x81c`, and the entire deep half of the
frame is a uniform `+0xc` shift (`0x2b0 -> 0x2bc`, `0x310 -> 0x31c`,
`0x410 -> 0x41c`, `0x510 -> 0x51c`, `0x710 -> 0x71c`, `0x810 -> 0x81c`) with an
identical callee multiset. One extra 12-byte local - `sizeof(real_point3d)` -
and `alndiff`'s 1,473 "differing regions" are overwhelmingly the same
instruction at a different `[ebp-X]`.

**Neither census alone finds it. Run both.**

## Triage order this implies

1. `sub esp,N` (`scratch/orch/boardframes.py`). A mismatch is structural by law
   and is the cheapest possible signal.
2. External reference multiset (`scratch/orch/relsites.py`,
   `tools/campaign/relocdiff.py --allow-structural`), with internal references
   excluded on both sides. A difference is where to look - not yet a verdict.
3. Real instruction count with trailing padding stripped, and only for functions
   without an embedded jump table.
4. Only when all three agree is "backend tie" the honest default - and even then
   read the slot census before saying so.

## What this does not say

- It does not say 112,133 parked bytes will close. It says they were classified
  as unreachable on evidence that does not support the classification. Several
  will turn out to be genuine ties for reasons the park never recorded, and
  `_ai_communication_finished` is proof that a function can be S1 and still be
  unreachable.
- It does not authorise editing anything outside Lane A's eleven translation
  units. Most of these rows belong to other owners and are reported for them.
- A park is a statement about a *residual*; this census disputes the *class* and
  the reopening criterion, not the residual. Reopening one means doing the work,
  not deleting the park.

## Provenance

`scratch/orch/boardsplit.py`; full output at `scratch/orch/boardsplit.txt`.
300 non-exact function pairs over 482 object pairs.

### Two corrections applied, both caught by disagreeing with the gate

**Symbol aliasing (the larger of the two).** The same address can be spelled
against different symbols: January writes `_sense_ray_divergences+32` where we
write `_sense_ray_angles-4`. The comparator resolves that; comparing relocation
target *names* does not. `_actor_move_initialize`, which `gate.py` reports
**EXACT** and `relocdiff --allow-structural` reports as `sha equal, 0 differing
rows`, leaked into the structural tier purely on name differences. Fixing the
exactness test to be "equal normalized bytes **and** equal relocation
(address, type) list" - names deliberately excluded, because the comparator
resolves them - removed **123** more already-exact functions and cut S1 from 184
to 73.

### Correction applied before publication

The first version of this census compared **raw** section bytes. A function is
exact when its **normalized** bytes match - the comparator zeroes the relocated
field at every site - so comparing raw bodies calls a function non-exact
whenever a relocated dword happens to hold a different link-time placeholder.
That pulled **238 already-exact functions** into the census (661 -> 423) and
inflated S2 (317 -> 132) and T (160 -> 107). S1 was unaffected, because a
function with a genuine reference difference is non-exact either way, and so
were all the parked totals, because a parked function is non-exact by
construction.

Caught by cross-checking Lane A's rows against its own `gate.py` census:
`_actor_select_firing_position` (5,248 B) appeared here as a tie candidate while
`gate.py` reports `source/ai/actor_firing_position` as `exact 26 residual 1`
with `_pre_evaluator_attack` as the only residual. If a census disagrees with
the gate, the census is wrong.
