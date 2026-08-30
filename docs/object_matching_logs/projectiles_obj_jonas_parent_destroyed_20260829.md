# `projectiles.obj`: typed parent-destroyed callback (2026-08-29)

## Result

`_projectile_handle_parent_destroyed` is newly strict-exact on its first and
only production candidate compile. The wave adds one function and 112 padded
code bytes. `source/items/projectiles.obj` advances from 12/30 to 13/30
strict functions and from 416/12,208 to 528/12,208 strict padded bytes. The
object remains `NonMatching`; no completion, data, semantic exception, park,
or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_projectile_handle_parent_destroyed` | 112 | 6 | `0e0f297c27520e3030c55d3b458e2c6adee46a0b95c312abb6c98632aa32f385` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with January. Its six zero-addend relocations are:

- `+0x0C` `REL32` to `_object_get_and_verify_type`;
- `+0x28` `DIR32` to the source-file literal;
- `+0x2D` `DIR32` to the parent-index assertion literal;
- `+0x32` `REL32` to `_display_assert`;
- `+0x39` `REL32` to `_system_exit`; and
- `+0x62` `REL32` to `_object_detach`.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show a verified projectile lookup, the canonical assertion that the parent
object index is not `NONE`, clearing the named attached flag, setting the
arming and detonation timers to `1.0f`, detaching the object, and returning
`TRUE`. The assertion evidence is the exact source literal
`c:\halo\SOURCE\items\projectiles.c`, expression
`projectile->object.parent_object_index != NONE`, and line 1845.

Independent semantic and layout evidence comes from:

- `C:/Users/isabe/Documents/Codex/reference/halocea/src/projectile_handle_parent_destroyed.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/projectile_datum.h`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/_projectile_datum.h`;
  and
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/projectile_datum_flags.h`.

The donor's different Xbox aggregate layout is not imported. The production
owner extends the already authenticated PC composite projectile datum with
named tracer, detonation-timer, detonation-delta, and arming-time fields.
Compile-time assertions authenticate the composite offsets of
`detonation_timer` at `0x1F0` and `arming_time` at `0x1F8`.

The earlier iterator ledger explicitly excluded this callback because the
private projectile owner did not yet cover these fields. It did not compile a
candidate. A read-only census of all 1,493 registered worktrees and all six
unique `projectiles.c` blobs also found no prior body or bounded attempt. The
later target-setter and private-leaf waves supplied the typed owner and flag
names that meet the iterator ledger's reopening condition.

## One-shot and policy boundary

The natural typed C89 body and owner extension were frozen before one normal
candidate compile. The callback was strict immediately. No alternate body,
declaration lifetime, spelling, ordering, compiler flag, adjudication,
comparator exception, or object manipulation was tested.

The retained implementation has one parameter per line and an explicit
terminal return. It contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, representation pun, undefined behavior, synthetic anchor,
private calling convention, or byte forcing.

## Validation

- January `projectiles.obj` SHA-256:
  `0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.
- Final rebuilt `projectiles.obj` SHA-256:
  `5beb653cbed8381d09967b30d12cb7f8dbc226bdf99a1ccb18fb902503903bc5`.
- Direct comparison: all 13 written Projectiles functions are strict exact;
  the cross-TU `_unit_preprocess_node_orientations` header-risk sentinel also
  remains strict exact at 1,920 bytes, 87 relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress pass. Aggregate progress is 382/833 linked files, 4,712 /
  11,060 code functions, and 597,432 / 2,198,102 code bytes.
- Semantic audit: 470 units, 4,868 functions evaluated, 4,741 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,751 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `d9b2cb2f8501fe8def248615c6ed2357e605c8a29424a176d09832b88a655251`.
- `build/semantic_report.json` SHA-256:
  `5862c1f46bd25bf9cdca4fe5f306cef7c98f824ce4c706d2ea7f1bf35c9f370d`.
- `git diff --check`: clean apart from Git's line-ending notices.
