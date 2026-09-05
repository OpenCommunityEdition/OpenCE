# Dynamic geometry: diagnostic BSS owner and completion boundary, 2026-09-04

## Result

Starting from canonical `56635021ee5f664a56680f7c805cafd8e76d5e71`, explicitly
initialize `reported_too_many_transparent_geometry_groups` to `FALSE` in its
existing C owner. This replaces a one-byte external COMMON request with the
one-byte external BSS definition owned by the January object. The program's
initial false state and all 17 strict-exact functions are unchanged.

No header, function body, compiler flag, target symbol, or COMDAT selection is
changed. No new code bytes or complete object are claimed. This corrects a real
storage-ownership defect; it is not a generated-byte carrier or new aggregate.

The older Opus reconciliation ledger's statement that the object was completed
is superseded by this audit. Its 17 exact-function result remains valid.

## Evidence and focused verification

The target manifest owns `_reported_too_many_transparent_geometry_groups` at
file offset 4,610,582 (`0x465A16`), with external storage class 2. A whole-source
search finds only the definition and its real diagnostic read/write sites in
this translation unit. The flag suppresses repeated transparent-group overflow
messages after the first report.

Before: COFF section 0, value 1, storage class 2: COMMON, not a BSS definition.
After: BSS offset 0, storage class 2, logical size 1, zero relocations, exactly
matching the January owner. Both target and repaired BSS normalize to SHA256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.

Focused candidate:

```powershell
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_dynavobgeom --edits scratch/dynavobgeom-owned-latch-20260904.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/dynavobgeom-owned-latch-20260904.obj
python scratch/dynavobgeom-latch-owner-audit-20260904.py
```

The gate reports **17 exact / 0 residual / 0 unwritten**, and the complete
emitted-symbol guard passes. An independent complete semantic-owner inventory
finds no old owner changed in linkage, offset, normalized bytes, or ordered
relocations; the sole new defined owner is the formerly COMMON flag. There
are no remaining COMMON symbols. Target/candidate counts are 31/59 sections and
31/44 semantic defined owners. This census does not mistake extra owners for
target progress.

## Remaining completion boundary

The following are inherited debt, not introduced by the one-line repair:

- `rasterizer_dynamic_geometry_debug_options` and
  `rasterizer_dynamic_geometry_frame_statistics` are caller-local partial
  definitions with opaque 2/28-byte and 260-byte spans. Their global declarations
  are not genuine recovered shared owner types.
- `global_window_parameters` uses another local partial type rather than the
  existing owner type/declaration.
- The candidate has a surplus 112-byte `_real_alpha_to_pixel32` code owner.
  Its absence from this January split prevents a simple physical-object claim;
  successful ordinary compatible-owner linking has not been proved here.
- Extra noncode copies are four assertion/source strings (18, 27, 29, 11 bytes),
  three stock XDK tables (88, 328, 16 bytes), and five four-byte real constants.
  This audit does not assert that these have passed final-link ownership.
- Nine authentic SDK wrapper functions have exact bodies but differing
  target/candidate storage classes. Exact function credit alone does not
  resolve their complete linkage/selection proof.

An explicit `source-layout-incomplete` object-admission rejection keeps the
object NonMatching even when its last reported data byte becomes exact. No
existing target-function credit is removed. Reopen only for authenticated
shared layouts/declarations and complete ordinary code/data ownership evidence;
do not hide the boundary with matching-only source or comparator changes.

## Canonical batch validation

Full build, stable-function regression sweep, park freshness, tests, and final
publication measurements are recorded with the accompanying batch checkpoint.
