# Independent review: Action Alert and Action Obey source context

**Verdict: both function changes are admissible. Action Alert is eligible for
whole-object admission; Action Obey is not.** No blocking source or ownership
finding was identified. This reviewer made no source/configuration change,
ran no Ninja, and made no commit.

## Frozen baseline and complete object comparison

The review rebuilt `git show 47d2e7b69f1c1277123e979296b6f0a7de00c80b:source/ai/...`
into independent scratch objects before comparing the integrated sources.
It did not use the already integrated `build/base` object as the old baseline.
Every compile used `gate.py --source --all` with the emitted
`_point_from_line3d` guard and distinct scratch outputs.

Reproduction: `scratch/astra-alias-actions-review.py`. Complete measurements,
source hashes, exact function sets, and ownership results are stored in
`scratch/astra-alias-actions-review.json`. Four independent gate logs and
objects use the `astra-alias-action_{alert,obey}-{head,integrated}` prefix.

| Check | Action Alert | Action Obey |
|---|---:|---:|
| Strict functions before | 11 / 12 | 25 / 27 |
| Strict functions after | 12 / 12 | 26 / 27 |
| Inherited exact losses | 0 | 0 |
| Changed code sections | next_position only | command_begin only |
| Named runtime owners before / after | 18 / 18 | 143 / 143 |
| Added or lost owners | 0 | 0 |
| COMMON symbols | 0 | 0 |
| Runtime noncode equality against frozen baseline | All owners equal | All owners equal |
| point_from_line3d emission | Absent | Absent |

All named owner offsets, storage classes, types, section flags, and names are
unchanged from the frozen baseline. Runtime data sections are compared by
owner and semantic relocation records, not physical section index. Debug
source-path metadata is excluded from runtime equality, not from the saved
raw objects.

`action_alert_next_position` now contributes **770 meaningful / 784 padded
bytes / 18 relocations**, normalized SHA-256
`677fd6cc1a9e66de32123c4614de5a0cc1188501baee69ea20fd5bf74eac3fe4`.
`action_obey_command_begin` contributes **4,320 meaningful / 4,320 padded
bytes / 167 relocations**. Complete hashes are in the review JSON. Independent
`relocdiff.py` output proves zero differing rows for each function in
`scratch/astra-alias-action_{alert,obey}-relocdiff.txt`.

Total: **+5,090 meaningful / +5,104 padded exact bytes / +2 functions**.

## Source semantics and redundant assignments

The reviewer examined the source diff against HEAD and the primary debug
disassemblies preserved by `astra_90pct_context_packet_20260920.md`.
The debug image is a **2020/v140 cross-build**, not a proven same-source-
revision January compiler witness. Its semantic correspondence is supported
by source assertion paths/lines, call topology, and the full function graph;
January's exact code and relocations determine the admitted result.

For Alert, the new single-return result graph preserves all original cases:
passenger/disabled-order and missing encounter return NONE; valid repeat
returns the current position; scanning and availability bookkeeping are
unchanged; random and ordered traversal return the selected position. The
direction output is written only on the same ordered traversal path. Every
result read is dominated by initialization or an assigning branch. Moving
locals into the encounter and scan branches introduces no uninitialized
access, changes no array extent, and calls no new helper.

The otherwise redundant result assignments have independent machine
provenance. Debug entry `0x42E650` first writes NONE to `[ebp-8]` at
`0x42E67F`, writes NONE to that same result again in the disabled/passenger
arm at `0x42E6A6`, and again in the no-available-position arm at `0x42E913`.
The repeat/random/traversal assignments all reach the shared result exit.
These stores therefore preserve authenticated branch statements; they are
not unexplained fillers added because the byte comparison improves.

For Obey, three NONE indices now initialize before `look_duration`, matching
debug `0x4383B7..0x4383D4`. The valid explicit-look arm rereads command+4 and
stores the duration again at `0x438409..0x438411`. Other look modes retain
the dominating initializer, and no path reads an uninitialized duration.
The existing random-look overrides remain intact. Debug
`0x438F48` then `0x438F54` independently demonstrates jump-delay storage
before pause-timer storage. The two destination members are distinct; neither
assignment adds a dependency or alias trick.

The full integrated files have zero lexical fake-match findings in
`scratch/astra-alias-actions-fake.json`. No volatile, register forcing,
forced inline/noinline, handwritten helper arithmetic, foreign prototype,
new UB, duplicate type facade, or extra behavioral owner is introduced.
Signatures retain project types and parameter formatting; both functions
return explicitly.

## Iterator lifetime and scope

The added inner block encloses the complete prop search, ending before the
player fallback. No prop pointer or iterator is consumed after that block;
the chosen prop index intentionally remains in the outer look-mode scope.
The fallback data iterator and head-position temporary remain independently
initialized in their own scopes. This is a meaningful lifetime boundary.

January passes the prop iterator at `[ebp-0x1c]` to the external iterator
API, then reuses overlapping storage for the player fallback's 12-byte
head_position at `[ebp-0x20]`. The HCEA PDB separately names both a
`prop_iterator iterator` and a `data_iterator iterator` at frame offset
0x50, corroborating separate logical iterator lifetimes. The PDB has no
explicit lexical Block record, so exact brace text is inferred, not claimed
as recovered source. Combined target storage reuse, independent typed-local
records, and the coherent prop-search/fallback boundary support this scope.

The existing no-extra-scope control produces 4,336 instead of 4,320 bytes and
preserves the same 25 exact siblings. It demonstrates that the lifetime
boundary matters; its admissibility comes from the primary lifetime evidence,
not merely that negative control. No arbitrary declaration padding is used.

## Whole-object admission

**Action Alert:** all twelve target functions are strict exact. The candidate
has exactly the same twelve named code owners and six named data owners as
January, with no target-absent code or missing code. Every data owner compares
strict exact against January, including bytes and semantic relocations. The
six payloads total 75 raw bytes and account for **80 aligned report data
bytes**; current report is 80/80 data, 12/12 functions, and 2,120/2,120
meaningful code bytes (2,240 padded code bytes).

The genuine math-header ownership was already independently reconciled in
`action_alert_obj_natural_math_ownership_reconciliation_20260905.md`; its
ordinary header helpers and their linkage are unchanged by this packet.
No object-admission veto names Action Alert. A fresh read-only admission
audit lists it as a candidate with zero function/data gaps, and the required
frozen owner review above now passes. It is therefore eligible for root to
mark whole-object Matching after the whole-tree gates.

**Action Obey:** the existing `_normalize3d` object-admission veto remains.
The frozen/current comparison finds the same fourteen target-absent math
code owners, including the owner family named by that veto. The sole
remaining target function, action_obey_control, is also residual. These are
inherited boundaries, not regressions, and no surplus helper receives credit.
Do not mark the object Matching or remove its ownership rejection. The new
command_begin function credit stands independently.

The read-only admission audit result is saved in
`scratch/astra-alias-actions-admission.json`. It reports zero contradictions
or revocations. Root remains responsible for final whole-tree tests, park
validation, admission status, and the local commit.
