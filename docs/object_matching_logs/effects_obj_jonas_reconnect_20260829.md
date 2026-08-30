# `effects.obj`: structure-BSP reconnect recovery (2026-08-29)

## Result

`_effects_reconnect_to_structure_bsp` is strict-exact on its first and only
production candidate compile. The function contributes 192 padded code bytes
and eleven relocations. `source/effects/effects.obj` advances from 7/41 to
8/41 strict functions and from 352/11,616 to 544/11,616 padded code bytes.
The object remains `NonMatching`; no data, object-completion, semantic
exception, or parked-function credit is claimed.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_effects_reconnect_to_structure_bsp` | 192 | 11 | `b8ea42c0bc9bc00c3be22e787caf9e27748bfceb623b8bcdc5ba39403d3ae18f` |

Every padded byte and every relocation address, type, destination, owner, and
addend agrees with January. All seven inherited exact functions in the object
remain strict.

## Authority and reconstruction

The January COFF and disassembly are the byte and ABI authority. They show a
`data_next_index` traversal of the effect pool. Each live effect is resolved
with `datum_get`; effects attached to an object are skipped. For an unattached
effect, the first location handle is resolved and advanced. A first-person
node is passed through the remaining location-chain helper in third-person
mode. A surviving marker's typed matrix position refreshes the effect's
scenario location; a missing marker deletes the effect.

Independent semantic and layout evidence comes from:

- `C:/Users/isabe/Documents/Codex/reference/halocea/src/effects_reconnect_to_structure_bsp.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/effect_location_get_next_instance.c`;
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/effect_datum.h`; and
- `C:/Users/isabe/Documents/Codex/reference/halocea/src/headers/effect_location_datum.h`.

The donor fixes the natural loop, unattached-object test, first location
selection, reconnect/delete behavior, helper signature, and helper name.
January additionally proves that VC7 specialized the helper's
camera-mode-zero prefix into the caller while retaining one call for a
first-person node.

The production source uses typed local effect and effect-location records.
Compile-time assertions authenticate the effect's location at `0x10`, object
index at `0x3C`, first location handle at `0x5C`, and total `0xFC` size, plus
the location datum's matrix at `0x08` and total `0x3C` size. Runtime code uses
only those named fields, `struct location`, `real_matrix4x3`, and public typed
APIs.

The HCEA-authenticated helper name
`effect_location_get_next_instance` replaces the target split's prior
address-derived `_code_0008b310` label in `config/symbols.json`. Its January
address, helper body, recursive calls, and reconnect relocation independently
agree with the donor identity. This label correction changes no target byte,
address, section extent, or relocation addend; it lets the retained source use
the real typed name without a raw-address alias.

## Exact relocation schedule

The eleven zero-addend relocations are:

- `+0x05 DIR32 _effect_data`, `+0x0E REL32 _data_next_index`;
- `+0x23 DIR32 _effect_data`, `+0x2A REL32 _datum_get`;
- `+0x48 DIR32 _effect_location_data`, `+0x4F REL32 _datum_get`;
- `+0x72 REL32 _effect_location_get_next_instance`;
- `+0x86 REL32 _scenario_location_from_point`;
- `+0x91 REL32 _effect_delete`;
- `+0x99 DIR32 _effect_data`, `+0xA0 REL32 _data_next_index`.

## One-shot and policy boundary

The complete typed candidate was frozen from the HCEA body, January
instructions, field accesses, and relocations before one normal Ninja compile.
It was strict immediately. No alternate body, spelling sweep, source-shape
retry, flag change, comparator exception, object manipulation, or tuning pass
was used. `_effect_delete` and every other remaining Effects body were not
touched.

The retained source is readable C89 with declarations before statements, one
parameter per line, and an explicit terminal return. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw runtime
address or offset, pointer/integer reconstruction, cast or union pun,
inactive-union access, undefined behavior, synthetic anchor, or byte-forcing
expression.

## Validation

- Canonical starting commit: `6a53dc195b6426b8de3aecd29a770baf62372c24`.
- Original January split-object SHA-256:
  `b9a46dba146ee46c39600e5bf5b181e620e971c75644be2f2ff3d1475ff362b2`.
- Renamed-symbol January split-object SHA-256:
  `e36f91756ba500987dbecd7a16972a88e769d03b2efd11120ff4e5328256d8f6`.
- Clean committed-state replay object SHA-256:
  `cef06950eba1e26b11287057cb743de995690c1770d11478fe3749e00987c2c5`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  progress generation: pass.
- Strict campaign board: 276/619 whole objects, 4,718/8,246 functions, and
  653,639/1,922,669 padded code bytes.
- Semantic audit: 470 units, 4,863 functions evaluated, 4,736 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,746 accepted exact, and zero unit
  errors.
- Aggregate progress: 382/833 objects, 4,707/11,060 functions,
  597,183/2,198,102 meaningful code bytes, and
  1,856,018/4,176,062 matched data bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `4f4fdd7bbdfdd48a3859b2305a24e669685ee6a43a3ebbfcc425cb5b5ce2b916`.
- `build/semantic_report.json` SHA-256:
  `f78fbd99cfb944afba81c6aa40c4e47cf713b2a4e843f28fe2c32d54157b7ba`.
- `git diff --check`: clean apart from Git's checkout line-ending notices.

No push is performed.
