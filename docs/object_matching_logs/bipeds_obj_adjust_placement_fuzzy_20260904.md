# `bipeds.obj` placement-adjustment fuzzy admission, 2026-09-04

## Scope and disposition

This bounded packet reconstructs only public
`biped_adjust_placement` in `source/units/bipeds.c`. Its declaration already
exists in the correct owner, `source/units/bipeds.h`; no header, target-symbol,
or data-owner change is required.

The natural first shot is source-authentic and semantically complete, but it is
not strict exact. Canonical retains it as an honest fuzzy function with the
measured park from `scratch/bipeds-adjust-placement-park-proposal-20260904.json`.
No exact code credit or whole-object completion is claimed.

## Source evidence

The reconstructed body:

- gets the datum through `biped_get` and the definition through
  `biped_definition_get`;
- requires `_biped_pill_centered_at_origin_bit` and rejects
  `_biped_flying_bit`;
- reads the project-typed `real` `collision_radius`; and
- applies `position.{x,y,z} += up.{i,j,k} * collision_radius` directly.

January's target disassembly confirms the flag byte at definition offset
`+0x2F4`, bit masks `8` and `4`, the radius load at `+0x42C`, and the direct
component updates at placement offsets `+0x18/+0x1C/+0x20` from the `up`
components at `+0x40/+0x44/+0x48`. It has no math-helper relocation.

The later HCEA implementation at
`research-cache/halocea-full-blobs-20260830/src/blam/units/biped_adjust_placement.c`
and independent raw-offset lifts in
`scratch/pastudan-halo/src/halo/units/bipeds.c` corroborate the same behavior.
These later reconstructions are semantic corroboration, not January source
authority. Independent review additionally confirmed that the known later-field
`collision_unused` width discrepancy cannot affect the preceding
`collision_radius` offset and should not be broadened into this packet.

The componentwise spelling is credible ordinary source and matches established
campaign practice for preserving January's inline schedule without emitting a
`point_from_line3d` COMDAT. No helper was manually expanded merely to avoid a
real target call: January itself contains the three x87 component operations
and no such relocation.

## Measurements

Baseline gate:

```text
python tools/campaign/gate.py source/units/bipeds --all --forbid-emitted-symbol _point_from_line3d --out scratch/bipeds-adjust-placement-baseline-20260904.obj
26 exact / 1 residual / 24 unwritten
```

Candidate gate:

```text
python tools/campaign/gate.py source/units/bipeds --edits scratch/bipeds-adjust-placement-edits-20260904.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/bipeds-adjust-placement-first-shot-20260904.obj
26 exact / 2 residual / 23 unwritten
```

Function comparison:

| Metric | January | Candidate |
| --- | ---: | ---: |
| Padded size | 96 | 96 |
| Meaningful size | 88 | 88 |
| Relocations | 2 | 2 |
| Normalized SHA-256 | `41db59000f9eb782eedfa45b2457e16898749fd2a3bc25a0e687c762181a40d4` | `5fb478d5414138003fa6ec1126c191fe0383f3c68c42523b28e455dd62e39f10` |
| Objdiff similarity |  | 98.3871% |

Both relocations have identical offsets and identities:

- `+0x0A`: `_object_get_and_verify_type`
- `+0x17`: `_tag_get`

The 39 decoded instructions are otherwise aligned. At `+0x34`, January loads
the placement-data pointer into `ECX`; the candidate loads it into `EAX`. The
remaining differences are only the corresponding `ECX`/`EAX` base register on
the six memory operands used by the three coordinate updates. There is no
semantic, branch, x87-stack, size, or relocation difference.

The fail-closed command:

```text
python tools/codegen_blocker_classifier.py build/split/source/units/bipeds.obj scratch/bipeds-adjust-placement-first-shot-20260904.obj _biped_adjust_placement --backend capstone
```

returns `UNKNOWN` / `unclassified`, confidence `NONE`, and grants no match
credit. That conservative class is retained even though manual disassembly
isolates an equivalent pointer-register allocation.

## Ownership audit

The read-only census is reproducible with:

```text
python scratch/bipeds-adjust-placement-owner-census-20260904.py
```

- Candidate versus baseline adds exactly one code owner:
  `_biped_adjust_placement`.
- No baseline code owner is removed.
- Every one of the 27 inherited target-present code owners is byte-for-byte
  unchanged between baseline and candidate; the same 26 remain strict exact
  and the inherited support-surface residual remains unchanged.
- The complete runtime non-code section inventory is identical between
  baseline and candidate.
- No data, BSS, COMMON, or undefined-external owner is added or removed.
- No surplus helper is emitted. In particular, the correctly underscored
  `_point_from_line3d` emitted-symbol guard passes.

Artifact SHA-256 values:

- edit manifest: `d85fcabde0b55cf6483607932474e8ab53aede98ec4b9861fea1919115fd8aeb`
- baseline object: `50d14410862aefa64b2c4d30e41ac4f69953d72290d49d24ba8299de6d627d2d`
- first-shot object: `b98c6021c65c8a0f40c5cb4e7ba3d0fb47086ef18c4b24d900435cf8cb29dd27`
- root's production-context gate object after replay:
  `e26644413e975190e2a6d2f0acf4a002c6625a9ac9f40a07c24557f93243a853`
- objdiff report: `eca1e657d90e0378fd55cbcba630a9289dc7b651dffa30c41d4d7f33eef9947b`

## House-rule review

The packet uses semantic public/private names, typed object/tag accessors,
named flags, project `real`, parameter-per-line formatting, and an explicit
terminal `return;`. It adds no local public prototype, address-derived name,
raw offset, fake storage/layout, type pun, inline directive, manual bitwise
predicate, assembly, or optimizer-only carrier. One natural source form was
compiled and then frozen; no declaration/lifetime/spelling lottery was run.

Independent read-only semantics review returned PASS. Root independently read
all seven existing Biped object ledgers, the complete January function and the
later HCEA reconstruction before replaying the source unchanged.

## Canonical validation

The full production build and `scratch/profile-biped-final-20260904.json`
stable sweep pass. All 6,203 exact owners from published `22a6e2c9d` survive;
the only two exact gains are the separately audited Profile pair, producing
6,205 strict owners. Biped placement adds no exact credit. The final Biped
gate is **26E/2R/23U**, and both inherited target-present code and non-code
owners are unchanged. Units remains 189 target exacts, with its separate
inherited helper veto unchanged.

All **267 parks** validate with zero stale/invalid measurements; admission
remains zero candidates/contradictions/revocations and four inherited
rejections. Tests pass **288 plus 26 subtests**. Biped source scanning has
zero findings, the no-point-COMDAT guard passes, and both protected Bitmap
hashes are unchanged. `git diff --check` passes.

Final evidence is recorded in `scratch/profile-biped-parks-20260904.json`,
`scratch/profile-biped-admission-20260904.json`, and
`scratch/pytest-profile-biped-20260904`. Root's independent classifier output
`scratch/bipeds-adjust-placement-root-classifier-20260904.json` also returns
UNKNOWN/unclassified. The physical Biped source hash before commit/EOL
normalization is
`99CA65124063FE159E8F2FC0EED57BB6B2E08994F9A8B699371CF068787256DC`.

Combined with the Profile publication packet, verified canonical totals are
**901,631 meaningful credited code bytes**, **6,159 credited functions**,
**2,028,615 credited data bytes**, and **391/833 Matching objects**. The
new Fable lane and its assignments remain untouched.
