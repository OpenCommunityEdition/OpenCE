# `projectiles.obj`: typed target-object setter (2026-08-29)

## Result

`_projectile_set_target_object_index` is newly strict-exact on its first and
only production candidate compile. The wave adds one function and 32 padded
code bytes. `source/items/projectiles.obj` advances from 8/30 to 9/30 strict
functions and from 240/12,208 to 272/12,208 strict padded bytes. The object
remains `NonMatching`; no completion, data, semantic exception, park, or
compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_projectile_set_target_object_index` | 32 | 1 | `5177013d53007b0bbb842f6166f57102134d6161a4e638faf45aecde58e02b8f` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with January. The sole zero-addend `REL32` relocation
is at `+0x0A` to `_object_get_and_verify_type`.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show the projectile object mask, the verified object lookup, and one named
long store at composite projectile offset `0x1E8`.

Independent semantic evidence comes from
`C:/Users/isabe/Documents/Codex/reference/halocea/src/projectile_set_target_object_index.c`
and its typed projectile headers. The Xbox donor's aggregate object layout is
not imported because January's PC item layer differs. Instead, the production
type composes this repository's already verified `_object_datum` and
`_item_datum`, then names the authenticated leading projectile fields. A
compile-time assertion fixes `projectile.target_object_index` at `0x1E8`.
The `projectile_get` macro preserves the repository's established typed
object-access convention and named `_object_mask_projectile` contract.

An earlier Projectiles iterator wave explicitly excluded this private runtime
field because it had not yet authenticated the composite datum owner. It did
not compile a candidate. This wave supplies that missing typed layout evidence
before spending its single production candidate.

## One-shot and policy boundary

The typed layout, accessor, prototype, and natural two-line setter were frozen
before one normal Ninja compile. The candidate was strict immediately. No
alternate spelling, body retry, compiler flag, adjudication, comparator
exception, or object manipulation was used.

The retained implementation is readable C89-compatible C with one parameter
per line and an explicit terminal `return;`. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw address or
offset access, pointer/integer reconstruction, representation pun, undefined
behavior, synthetic anchor, private calling convention, or byte forcing.

## Validation

- January `projectiles.obj` SHA-256:
  `0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.
- Final rebuilt `projectiles.obj` SHA-256:
  `73c641dd5e07b1da5ba10cecc77745a21ce3089035eefec94d79ddcac9648930`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,718/8,246 functions, and
  653,479/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 32 bytes.
- Semantic audit: 470 units, 4,863 functions evaluated, 4,736 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,746 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `fb5905b34061a58a67bcce135efd2cfade3062360e68545d4e734dafa02ea5bf`.
- `build/semantic_report.json` SHA-256:
  `412b5ed808a19ee59775687882a791a465f0a31414355792f732e898a98c9a80`.
- `git diff --check`: clean apart from Git's line-ending notices.
