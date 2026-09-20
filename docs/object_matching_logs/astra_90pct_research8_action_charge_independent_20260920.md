> Research only: zero admission credit; no retained source/header/configuration change.

# Independent action-charge review: hold, zero credit

The measured melee-first candidate preserves all inherited controls and
ownership but remains non-exact. It is research evidence only. This reviewer
performed no compiler invocation, source/header edit or further shape.

Independent PE decoding confirms the later unoptimized dispatch at
0x43027e compares goal 2, at 0x430281 branches to 0x430293 on equality, then
compares goal 3 at 0x43028a and branches to 0x430593 on inequality. Thus the
melee body precedes the complementary non-melee body in this raw build.
The 11 RTC entries at descriptor 0x4316f4 were independently re-read.

This provides concrete support for one bounded whole-arm exchange. It does
not recover January lexical order: January +0xa1/+0xab makes the same decision
but branches to melee at +0x276 while placing non-melee code first. Even an
unoptimized later layout is evidence rather than original C text. The primary
source-revision claim remains unverified; the documented extra pre-ballistic
operations and different lead-fraction arithmetic prevent importing that later
body wholesale. No such operations were added by this candidate.

Independent text reconstruction verifies that the patch only exchanges the
two complete arms and complements the predicate from `!=/&&` to `==/||`.
Every declaration and operation within each arm is byte-for-byte unchanged;
the function prefix and suffix are unchanged. The goals are ordinary integral
state values and the tests have no side effects, so the complements preserve
the branch partition. No helper, local layout, inline option or new API is
introduced. Prior helper-linkage and local-permutation rejections remain in
force; this result does not reopen them.

| _action_charge_perform | Padded | Relocations | Instructions | Hash prefix |
| --- | ---: | ---: | ---: | --- |
| Frozen current | 3216 | 84 | 983 | 2e9beea6 |
| Melee-first | 3232 | 84 | 982 | 73dfb3c0 |
| January | 3248 | 84 | 982 | 9ed9d44b |

The allocation observation is real: the candidate writes `check_range` to
EBP-2 and `berserk_ranges` to EBP-1, matching January's roles. It tests BL
immediately after its game_time_get call at +0x385; January does so after
+0x37a. The baseline instead reloads the flag from EBP-1. This explains one
allocation change without declaration tuning, but full strict equality still
fails. Matching instruction count is not coverage credit, and the remaining
prop reload, range scheduling and helper-context differences are unresolved.

Independent complete COFF inspection confirms all 20 inherited exact target
functions remain exact. Only `_action_charge_perform` changes among existing
functions; `_action_charge_setup` retains its inherited residual. All 23 code
owners, 45 named owner records (including flags and section-selection metadata),
23 nondebug noncode sections and empty COMMON are preserved.

This unit legitimately selects `_point_from_line3d`; an absent-point rule would
be wrong here. Its sole owner remains 48 bytes and strict exact against January,
with unchanged ownership metadata. No candidate-only helper appears. Production
source and base object still equal the frozen inputs.

Final recommendation: **HOLD**, zero exact credit, no second shape. Reopen only
with independent primary support for the remaining prop lifetime/call boundary
or a genuine helper-owner mechanism that predicts complete closure while
preserving all controls. The observed branch-order allocation effect alone
does not justify a transferable compiler rule or further permutations.
IDA_AUTOMATION_UNAVAILABLE.

Evidence in this directory: `review.py`, `review.json`, `dispatch-primary.asm`,
`baseline.asm`, `candidate.asm`, `january.asm`. Producer inputs are in
`../action-charge/`: `packet.md`, `melee-arm-first.c/.obj/.patch`, `shape.json`,
`audit.json`, `primary.json`, actual `ghidra_00430170.{c,asm}` and the HCEA
typed/source-line records. Primary executable SHA256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
