# Frontier widget and camera-impulse reconciliation

Canonical baseline: `cdf1c42ba6687083da2c7ad2730fdce57628d72d`.
Donor: `C:/halo-worktrees/claude-remaining-frontier-20260926`, reviewed at
`798f5c2cde28387741370dd21ec5679282630723`, branch
`claude/remaining-frontier-20260926`. Donor production commits:
`b659a777e4086d34ff3036b30bdf8bc6dcd92ce8` and
`06645ef469cbe43d3617f42e2f79f9b2f925f736`.
Only their three production paths are integrated. Donor source/config matches
canonical plus precisely this delta; no earlier unreconciled prerequisite exists.
The donor and its untracked RF-T census are untouched. No push performed.
Canonical source commits: `c680c2b0` (widget) and `c0375104` (camera impulse
and park retirement). Backup ref `backup/astra-before-frontier-pair-20260926`
retains the pre-edit canonical commit.

## Fresh canonical result

| Function | Meaningful | Padded | Relocations |
|---|---:|---:|---:|
| `_widget_instance_render_recursive` | 743 | 752 | 27 |
| `_player_effect_update_camera_impulse` | 738 | 752 | 24 |
| Total | 1,481 | 1,504 | |

The ledger and verified non-tail-padding extents agree for this batch. The
donor headline's two 752-byte numbers are padded, not meaningful byte credit.
Halo code rises **1,593,739 -> 1,595,220 / 1,770,166 (90.12%)**;
Halo accepted functions **7,463 -> 7,465 / 7,574**. Whole-board strict owner
sweep **7,635 -> 7,637 / 8,252**, exactly these two gains, **zero losses**.
Halo objects remain **389 / 468**, data **2,588,903**; neither object completes.
Earlier five-byte ledger-padding caveat from the actor-path reconciliation is
unchanged, not silently repaired or newly credited here.

## Source-credibility review

### Widget rendering

The helper parameter-reuse while loop, clip copy/repoint/field-update order,
and call to the genuine color helper replace less faithful source. Independent
raw `/Od` image inspection corroborates the helper at `0x668190`, clip topology
at `0x66a240..0x66a2a4`, and alpha/-1 call and live color local at
`0x66a33d..0x66a37e`. RTC descriptor `0x66a65c` corroborates `local_clip` and
`multitexture_params`; `color` is descriptive. The later executable's SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
It was read as data, not executed; later-only source is not imported.

`0xFFFFFFFF` is an opaque-white pixel bit pattern. Its machine value is
attested; its original literal spelling is not. It is not an index sentinel
and should not be renamed `NONE`. The helper's effect on VN ordinal is fully
disclosed, but its ordinary source shape has independent evidence; no filler,
decorative parentheses or invented macro is added. The inherited `sequence`
local and its historical cleanup/provenance concern remain unchanged; the
later build has the pointer local but different uses. No cleanup is smuggled in.

### Camera impulse

Two hand-expanded comparisons become calls to the pre-existing `realcmp`
macro in its genuine owner `real_math.h`. Independent inspection finds the
macro name in January assert strings in actors, matrix_math and
biped_limp_noodle, including a call-result argument. Its expansion evaluates
each argument once; comparison types, short-circuiting and tolerance are
unchanged. No header, ABI, cast, helper or compiler-mode change is involved.

The spelling at this particular site is **inferred**, not directly attested;
the `/Od` build is neutral and September/October retail matches corroborate,
not prove, it. The two macro uses' +2 VN-ordinal effect is disclosed. This is
normal use of an authenticated subsystem API, not the invented square macros
held by Q13. The scheduler park's natural-context reopen criterion is met;
its single entry is retired only in the freshly verified exact batch. No
other hold is lifted or implicitly generalized.

## Canonical checks

- Full Ninja before/after with affected TUs freshly compiled. Independent
  `/W3` baseline/candidate compiles reproduce every non-debug section too.
- Complete section/storage comparison: **372 UI sections and 74 player-effect
  sections**. Only the respective target text sections change. Existing helper
  copies and every data section remain identical. No added/removed section,
  code/data/COMMON owner or storage/ABI disagreement; no new provider-link
  exception is required. Local debug records are excluded, not code relocations.
- Strict target identity includes full padded payload and relocation identities.
  Pinned normalized hashes and object receipts are in `RESULTS.json`.
- `/W3`: UI **17 -> 17**, player_effects **13 -> 13**; zero new warnings.
- Pytest before/after: **1,161 passed, 5 skipped, 26 subtests passed**.
- Parks **75 -> 74**, zero stale/invalid. Admission unchanged:
  **11 candidates / 0 contradicted / 1 rejected / 0 revoked**.
- Fake scan: same **26** inherited findings by path/rule/snippet, not just count.
- Source/config whitespace check passes. No scorer, normalization, flags,
  production tests, shared headers, symbols.json or Matching status changes.
- The pre-existing README edit is byte-preserved and excluded from commits;
  all seven inherited untracked research directories remain untouched.

## Evidence and reproduction

`research/astra_frontier_pair_reconcile_20260926/verify.py` runs the independent
section/storage, baseline/candidate diagnostics and receipt checks, using saved
baseline objects in `scratch/reconcile_frontier_pair_20260926/`. It reuses
reviewed utilities from the previous frontier reconciliation; it does not
change any production scorer. `RESULTS.json` pins source, tools and local gate
receipts. Selected donor evidence is archived byte-for-byte in `donor/`,
including failed probes and the failed first clean-build receipt, not only
successful summaries. Remaining donor research and all held candidates stay
in place. Private EXE/PDB/SDK/compiler/object assets are not added to Git.

The RF-T cross-worktree catalogue is a research inventory, not fresh canonical
proof for its other rows. No held, library or unverified upstream candidate
from that inventory receives source changes or matching credit in this batch.
