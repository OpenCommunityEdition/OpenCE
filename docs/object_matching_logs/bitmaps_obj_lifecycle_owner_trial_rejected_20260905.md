# Bitmaps lifecycle API owner trial: rejected and restored (2026-09-05)

## Result

The single genuine three-file API owner packet was rejected because it lost
one inherited Decals exact function. All three textual edits were reversed.
The restored full build retains the published Bitmaps batch at
`e50f98a194e0bac4d3f85c2fe2d8aec76b0b9f1c` with **915,646 meaningful exact
code bytes**, **6,206 credited functions**, **6,252 strict functions**, and
**391/833 Matching objects**. No newly accepted bitmap work was removed.

`bitmap_rebuild` was never compiled or imported during this trial and gains
zero credit. The target body remains an owner-boundary hold. No alternative
prototype order, header spelling, qualifier, fake local facade, compiler
control, or source-shape variant was tested.

## The one coherent packet

The prerequisite review is
`bitmaps_obj_rasterizer_owner_prerequisites_20260905.md`. The trial made only:

1. `source/rasterizer/rasterizer.h`: add the ordinary public declarations for
   `rasterizer_bitmap_new`, `rasterizer_bitmap_changed`, and
   `rasterizer_bitmap_delete` under their hardware-bitmap implementation group.
2. `source/rasterizer/rasterizer_text.h`: remove its two misplaced lifecycle
   declarations (new and changed).
3. `source/bitmaps/bitmaps.c`: include the real rasterizer owner and remove the
   two inherited C-local changed/delete declarations.

All API parameters were the existing mutable `struct bitmap_data *` contract;
new returns `boolean` and changed/delete return `void`. No types, fields,
storage, bodies, config records, or target metadata were otherwise changed.
No Opus-reserved rasterizer/device/widget implementation was edited.

Actual valid Ninja dependencies identified **48** consumers: the 47 existing
users of `rasterizer.h` plus Bitmaps. The two consumers of `rasterizer_text.h`
are already in that set. The full consumer list is retained in the prerequisite
review and frozen manifest; this was not a direct-include-only check.

## Observed failure

The ordinary full Ninja compile/report completed, but the independent full
stable-function comparison found **6,252 -> 6,251 exact functions**, zero
gains and one loss:

| Object/function | Target meaningful / padded | Relocations | Result |
| --- | ---: | ---: | --- |
| Decals / decals_delete_permanent_from_cluster | 392 / 400 | 29 | normalized code changed; inherited exactness lost |

The target normalized code hash begins `b3eeb427eeef1e02`; the trial hash
begins `6fb830e33513eb04`. Equal extent and relocation count do not make the
changed body exact. The complete runtime comparison classifies **3,612
unchanged sections, 19 compiler-local-label-only differences, and one real
runtime/owner difference** in Decals. There are no new point-from-line helper
definitions or references. The new API declarations are still semantically
correct; the result demonstrates an unacceptable compiler-context regression,
not permission to move them to a false owner or tune unrelated code.

The independent instruction audit localizes the change to four non-relocation
bytes at offsets `0x13e`, `0x13f`, `0x141`, and `0x142`: two independent
`mov` loads exchange order at `0x13d` and `0x140`. All 29 relocations agree.
This appears behaviorally equivalent but remains a real strict-code loss.

## Exact restoration proof

Root used `apply_patch` to reverse only the measured three-file textual packet;
no reset, checkout, clean, donor edit, or unrelated source restoration occurred.
The full restored Ninja build passes. The global stable census returns to
**6,252 exact with zero changes** relative to the pre-trial snapshot.
Across all 48 frozen consumers, **all 3,632 runtime sections are unchanged**,
with no COMMON/owner/point differences. The three production files have no
remaining textual Git diff. Existing line-ending/stat-only worktree marks are
not a retained source change.

An independent complete-object replay also finds all 48 restored objects
byte-identical to their frozen inputs except the COFF timestamp at bytes
4 through 7. All 1,285 function sections in this consumer set are unchanged,
with zero added, lost, or altered runtime sections. Inverse patching changed
some source line-ending bytes; normalized Git text and emitted runtime
content are restored, not claimed byte-identical source encodings.

The published Bitmaps constructor/address packet remains 18/34 exact functions
and 4,139 meaningful code bytes. Its nine-function / 3,348-byte gain remains
fully retained. ActionObey remains 20/27 and Units remains 189/189. The treemap
must be refreshed only from the restored canonical report.

## Reproduction evidence

- Capture: `scratch/bitmaps_lifecycle_owner_capture_20260905.py`.
- Before: `scratch/bitmaps-lifecycle-owner-before-20260905.json`, SHA-256
  `3a5a65e1c5c3b66367f54873416eaeed48385076a7aff0805dd1a46d84ee634d`.
- Trial: `scratch/bitmaps-lifecycle-owner-after-20260905.json`, SHA-256
  `3261c2be95e677fecd4769c29716ccd375bfc4c247077bb7abc31d961181ef3a`.
- Restored: `scratch/bitmaps-lifecycle-owner-restored-20260905.json`, SHA-256
  `738a971b6fe90f3d4e9abbb407ffa2b13f72fb040aa46044dc0823339b64c498`.
- Stable snapshots:
  `scratch/bitmaps-lifecycle-owner-stable-{before,after,restored}-20260905.json`.
- Runtime audits:
  `scratch/bitmaps-lifecycle-owner-runtime-20260905/summary.json` and
  `scratch/bitmaps-lifecycle-owner-restored-runtime-20260905/summary.json`.
- Build logs: `scratch/bitmaps-lifecycle-owner-build-20260905.log` and
  `scratch/bitmaps-lifecycle-owner-restored-build-20260905.log`.

The generic runtime auditor also passes its nine synthetic invariance/negative
checks. Frozen manifests pin complete input objects, source files, config,
dependency identity, and all 572 base-object hashes. These snapshots preserve
the failed source proposal for inspection without retaining it in production.

## Boundary

Keep `bitmap_rebuild` withheld and the inherited lifecycle declaration debt
explicit. Do not repeat this same packet or sweep header positions/spellings.
Reopen only for materially new source/owner provenance or an independently
justified compiler-context correction that itself passes the full inherited
regression and runtime-owner checks. Continue other disjoint Fable work.
