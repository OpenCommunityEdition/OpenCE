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

Over all 661 non-exact functions on the board (582,597 padded bytes):

| tier | meaning | functions | bytes |
|---|---|---:|---:|
| **S1** | the emitted **reference multiset** differs - a missing or extra program reference | 184 | 151,253 |
| **S2** | references agree, the **real instruction count** differs | 317 | 313,792 |
| **T** | both already agree; only here is "tie" the default reading | 160 | 117,552 |

Split by park status:

| tier | parked functions | parked bytes | classes |
|---|---:|---:|---|
| S1 | 39 | 47,029 | 31 unclassified, 3 register-allocation, 3 tu-context, 2 scheduling |
| S2 | 75 | 65,104 | 52 unclassified, 9 scheduling, 6 register-allocation, 6 tu-context, 2 private-convention |
| T | 67 | 39,392 | 25 scheduling, 23 unclassified, 12 csplit-alias, 6 register-allocation, 1 private-convention |

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

## The trap that makes this census easy to get wrong

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
661 non-exact function pairs over 482 object pairs.
