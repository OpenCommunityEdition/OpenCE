# P1 canonical reconciliation — 2026-09-27

Base: `e44eea91263eb0285f10ea84b1933da02c2ae721`.
Reviewed donor: `9661fd1a890ce925374ae99f1a50eb8808584c5b`.
This is the specifically owner-approved P1 correction and debit, not a new
general exception to the zero-loss or surplus-helper rules. Nothing was pushed.

## Result and honest accounting

| Row | Meaningful bytes | Padded bytes |
|---|---:|---:|
| `_player_teleport_internal`, newly strict exact | +1,285 | +1,296 |
| `_rasterizer_frame_statistics_draw`, approved exact loss | -4,165 | -4,176 |
| Net | **-2,880** | **-2,880** |

There is no net function gain. The only exact loss is the approved draw row.
`rasterizer_frame_statistics` is correctly revoked to NonMatching; Halo object
count becomes 388/468. Hardware-bitmaps admission is a separate packet, not
implicitly credited here. Halo code is 1,593,054/1,770,166 meaningful bytes and
7,467/7,574 functions; data remains 2,588,903. These report counts are distinct
from the strict whole-board gate's 8,252 owners / 7,639 exact verdicts.

## Independently checked source and storage

The two private prototypes leave `players.h`; both definitions become static.
Only the necessary powerup forward declaration remains in `players.c`.
The January symbol attribution is corrected in place for both functions.
HCEX storage, January public-symbol absence, zero cross-object undefined
references and actual source callers independently support private linkage.

The teleport rewrite restores named locals, structured loops, whole-vector
copies and the genuine point helper. `/Od` witnesses include the two vector
copies at 0x5d2cbf–0x5d2cec and the helper call at 0x5d2f18. January's type mask
at target +0x39c also supports `unit_get` for the root vehicle rather than the
previous biped-only access; the separate elevator access retains its biped
mask. The later build is corroboration, not authority over January's bytes.

After source blobs exactly equal the reviewed donor:

- players.c: `389a896179cedcdb8ca5f9d3bb0fe01fe2419391`
- players.h: `4de67e406718873655ba8408d63283a6774ab39b`

Teleport's strict normalized SHA-256 is
`8bb48b429649a3afcd41cf238ca9b1d5dba0ff61e15f2205f72512918cad7c43`,
with 61 complete relocations. The powerup helper's body is unchanged. Both
definitions have storage class 3, matching the regenerated January split.

## Gates on current canonical

- `ninja all_source progress build/report.json`: passed, all units rebuilt as
  required; objdiff remains 3.3.1.
- Stable whole-board before/after sweep: exactly teleport gained and draw lost;
  no other loss. Its nonzero regression exit is expected and explicitly
  accounted for, not reported as a zero-regression run.
- Players: 68/70 -> 69/70 exact. Only teleport changes existing code bytes.
  Data and COMMON unchanged; sole new definition is `_point_from_line3d`.
- All 32 surplus definitions (14 helpers and 18 data literals/constants) match
  January's selected providers, including full padded payload and relocation
  identity. All 64 selected-provider probes in both orders contain only
  expected unresolved-external diagnostics, no duplicate-definition errors.
  This is bounded duplicate/coalescing evidence, **not a whole-program link**.
- `/W3`: 18 -> 18; `/W4`: 106 -> 106, identical warning multisets, no C4211.
- Parks: 72 active, zero stale or invalid; no park edited.
- Admission audit: 12 candidates, 0 contradicted, 1 rejected, 0 audit revocations
  (the explicit config revocation above is separately counted).
- Fake scan: all 26 inherited leads unchanged, identical complete JSON.
- Pytest: 1,161 passed, 5 skipped, 26 subtests, both before and after.
- `git diff --check`: clean.

Local complete receipts and reusable audits are preserved under
`scratch/astra_one_more_20260927/`, particularly `p1_audit/REPORT.md`,
`provider_audit.jsonl`, `transport/P1_SOURCE_REVIEW.md`, `before.json`,
`after_p1.json`, `p1_build.log` and `pytest_after.log`. No reference executable,
compiler, SDK, PDB or object binary is added to the repository.

## Preserve the exact canary

The full pre-P1 tree is preserved by base commit e44eea91 above. Its exact draw
source blob is `429e10a1ed1aa8b00f6dd616db634551f191b81a`; players.h is
`34ee5f08686efb5c63feb7ec51767fa81da09a15`. The exact draw normalized SHA-256 is
`747b937d70b288778c244bc3e426d03294da76a99ea3afd31cc23abb5edcccec`.
The loss follows the independently justified header correction. This does not
retroactively declare the previous exact source illegitimate, nor permit
declaration-count tuning to recover its credit.

## New probes, separately zero credit

Three triangle-coplanar probes tested genuine helper calls, later-build
float-overload spelling and their combination. None became exact; all eight
exact siblings survived. The float-overload form changes January's qword
comparisons and widened-float constant to dword forms, so it is not a January
repair. The candidate files and machine-readable receipts remain in
`scratch/astra_one_more_20260927/triangle/`.

The virtual-keyboard investigation corrected an attribution: `/Od` 0x68ef60
is the physical-key event-draining processor, not direct evidence for the
selector's arm order. Reopen on correctly attributed source evidence, not
further tail permutations. Transport-startup and draw-bitmap hypotheses were
already measured negatives or held; no duplicate sweeps or production changes.

The BSP open-edge and bitmap-drawing declaration questions remain held under
their latest rulings. This packet does not lift any unrelated hold.
