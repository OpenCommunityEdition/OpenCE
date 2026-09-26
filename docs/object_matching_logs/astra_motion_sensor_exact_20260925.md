# One-function closeout: update_motion_sensor

Canonical baseline: `f0e52c8b88f4fd95df3a2b1d2d3ee39c29e24de6`.

## Result

`source/interface/motion_sensor::_update_motion_sensor` is now strict exact:
**849 meaningful bytes, 864 padded bytes, 39 relocations**. Normalized SHA256:
`b37814ed8aaa3427a5d8e1fe1327e22cc80190383562143e94c35df03f5db06b`.

Halo credited code: **1,583,050 -> 1,583,899 / 1,770,166**.
Halo accepted functions: **7,452 -> 7,453 / 7,574**.
Halo objects remain **388/468**; motion_sensor remains **17/19** functions,
not a whole-object completion. Data credit is unchanged at **2,587,011**.
Retired only this function's park; no semantic-credit override was added.

This reconciles existing Fable P6A work; it is not a newly discovered compiler
law or an additional gain beyond that donor's previously reported 849 bytes.

## Provenance and minimal integration

Donor: `C:/halo-worktrees/fable5-research44-20260925`, source commit
`816b4acdffb43262ab99a078f5bd62f2be6e3d66`.
The original independent source/evidence review is committed at
`research/fifty_objects_20260925/w/review4_motion_sensor/REVIEW.md`.

Only `update_motion_sensor` is taken from P6A. The donor's sentinel cleanups in
the two still-residual callers are excluded. Existing canonical local names
are retained wherever equivalent locals existed: camera_position,
object_position, object_radius, displacement, custom_positions and blip_index.
Both the donor-name and canonical-name target-only forms freshly gate exact;
no matching credit depends on accepting a later-build-only rename packet.
New substantive locals describe the reconstructed points/array access.

The reconstructed 3D distance calls, scoped camera queries, custom-position
copy/subtraction, and statement order have the recorded first-party debug
evidence. All distance operands are initialized. The radius output is a real
API output, not a dead pressure local. The custom-position producer limits its
result to the supplied capacity; compaction never indexes beyond the current
input index. This patch does not import the separate held positions-array bug
in `_motion_sensor_update`.

The recovered `<=` success branches differ from the old `>` failure branches
on NaNs. This is target-backed reconstruction, not a claim of equivalence for
every possible floating-point input.

The genuine shared `check_stack_buffer` inline/macro already exists in
canonical via `466698b8`. The old non-exact-caller hold is satisfied by this
strict-exact caller, under the folded-header-inline exception documented in
`docs/campaign_house_rules.md`. No header, compiler flag, invented macro,
parenthesis lever, pragma, manual helper expansion, or new view cast is added.

## Fresh verification

- Full `ninja`: pass with objdiff 3.3.1 unchanged (SHA1
  `3130e4288d483d259d1588092c8159f8e0230e08`).
- Keyed board-wide sweep: **8,252 owners; 7,624 -> 7,625 strict exact**;
  exactly one gain, **zero regressions**. This newer owner census includes
  fastcall coverage; do not compare it directly to old 8,245-row snapshots.
- Whole-object comparison: only `_update_motion_sensor` changes among existing
  runtime sections; **64 unchanged, zero removed, four added**. Both other
  residual callers and every existing exact sibling retain their bytes.
- Added surplus: `_check_stack_buffer`, `_distance_squared3d`,
  `_vector_from_points3d`, and `__real@00000000`. Each matches January's chosen
  copy. All 12 surplus code helpers also pass strict identity checks.
- Selected-provider duplicate/coalescing tests: all four new symbols pass in
  both link orders. These bounded tests permit unresolved external references;
  they are not a full-program link or a bootability claim.
- COMMON records and existing symbol storage are unchanged.
- Parks: **78 active / 0 stale / 0 invalid**.
- Admission: **9 candidates / 0 contradicted / 1 rejected / 0 revoked**,
  unchanged from baseline.
- Changed-file fake-match scan: **0 findings before and after**.
- `python -B -m pytest -q tools`: **1,161 passed, 5 skipped, 26 subtests**.
- `git diff --check`: clean. Source retains CRLF.

Fresh local evidence is in `scratch/astra_one_20260925/`: before/after keyed
snapshots, target-only candidates, park/admission reports, and independent
`verification.json`, saved baseline object, identity comparisons and link logs.
The integrated source SHA256 is
`e5c7113f71b3177b687e5a959f241bfd015a131838d492e7f17a1a27f84bded0`.
Other ongoing research and donor worktrees are preserved unchanged.
