# `projectiles.obj`: typed private leaf cluster (2026-08-29)

## Result

The newly authenticated projectile runtime owner unlocks three strict-exact
private leaves: `_projectile_kill_tracer`, `_projectile_make_tracer`, and
`_projectile_handle_deleted_object`. Their single shared candidate compile
adds three functions and 144 padded code bytes. `source/items/projectiles.obj`
advances from 9/30 to 12/30 strict functions and from 272/12,208 to
416/12,208 strict padded bytes. The object remains `NonMatching`; no
completion, data, semantic exception, park, or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_projectile_kill_tracer` | 48 | 1 | `40a57b24126f6c130372e09bef9993562b9dde948eb103c50328e5746ac1e583` |
| `_projectile_make_tracer` | 48 | 1 | `0380462bf0649ca1ea326c028cf47e5278e675de9270f08a0f99b0a8859a8552` |
| `_projectile_handle_deleted_object` | 48 | 1 | `43be501ebdec878f60afcd785c829b28157f36e1f6347d6f77d1a6bc9ae9c45b` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with January. Each leaf has one zero-addend `REL32`
relocation at `+0x0A` to `_object_get_and_verify_type`.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show the projectile object mask and typed object lookup in all three leaves.
The tracer pair clears or sets named bit one in the projectile flags at
`0x1DC`. The deletion hook compares the named target object at `0x1E8` and
stores `NONE` only on equality.

Independent semantic and naming evidence comes from:

- `C:/Users/isabe/Documents/Codex/reference/halocea/src/projectile_kill_tracer.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/projectile_make_tracer.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/projectile_handle_deleted_object.c`;
  and
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/projectile_datum_flags.h`.

The preceding target-object-setter wave authenticated the PC composite
projectile owner from this repository's `_object_datum` and `_item_datum`.
This wave extends that evidence only with HCEA's database-named projectile
flag indices. It does not import the donor's different Xbox aggregate layout.

The earlier Projectiles iterator ledger excluded all three leaves specifically
because the private datum did not yet have a canonical typed owner. It did not
compile them. The new owner satisfies that ledger's explicit reopening
condition.

## Candidate and cross-TU gate

All three natural typed bodies were frozen together before one Projectiles
candidate compile and were independently strict immediately. No body spelling,
declaration lifetime, branch, helper, or compiler-control alternative was
tested.

The first aggregate gate found a transitive header-only regression: exporting
the private flag enum and otherwise unused private leaf prototypes through
`projectiles.h` changed assertion-literal ownership in
`units.obj:_unit_preprocess_node_orientations`. Its 1,920 code bytes remained
the same shape, but the strict relocation-owner fingerprint was no longer
January's. The declarations were therefore internalized in `projectiles.c`,
where their ownership naturally belongs; none of the three candidate bodies
changed. A direct clean rebuild then restored the Units function to exact and
kept all four typed projectile leaves exact. No regression is accepted or
hidden by this wave.

The retained code is readable C89-compatible C with one parameter per line
and explicit terminal returns. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or offset
access, pointer/integer reconstruction, representation pun, undefined
behavior, synthetic anchor, private calling convention, or byte forcing.

## Validation

- January `projectiles.obj` SHA-256:
  `0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.
- Final rebuilt `projectiles.obj` SHA-256:
  `b6630ec2fd65b10b9b3b3977984b942ac056eabb14a05cb5084db79e17a21d1e`.
- Direct comparison: all four typed projectile leaves are strict; direct
  comparison also confirms `_unit_preprocess_node_orientations` is restored
  to strict exact.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,721/8,246 functions, and
  653,623/1,922,669 padded code bytes. This wave adds three functions and 144
  bytes without losing an inherited exact owner.
- Semantic audit: 470 units, 4,866 functions evaluated, 4,739 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,749 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `c582b86b499ce7e3c2ee74b7e2bbd717fdece6366be07933e93facfc1b2ac40b`.
- `build/semantic_report.json` SHA-256:
  `be438e2f97b0e40097ffb0d4d438800e6208d4f27e368b0de47ab0878d89c3ba`.
- `git diff --check`: clean apart from Git's line-ending notices.
