# `object_lights.obj` marker cluster (2026-08-29)

## Result

`source/objects/object_lights.obj` advances from 12/43 to 15/43 strict-exact
functions. The retained production source reconstructs the typed light-marker
state and admits three functions:

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_light_marker_begin` | `55 / 64` | 7 | `fa269afbcde38818559b3ce47e3fc6845d8655d8d7326ab05a5e263d4895202e` |
| `_light_mark` | `89 / 96` | 8 | `1175088cd02bd3341a0ff200c3356917ecd5b27958606b0c9d68ef92c07a5300` |
| `_light_marker_end` | `49 / 64` | 6 | `92a2e74acbf810eb1514c8d27df288d3a1a5b02827f5862c86d136c47f78499d` |

This adds 193 meaningful code bytes. The object remains `NonMatching`; no
completion label, semantic exception, parked entry, or data credit is added.

## Evidence and frozen candidate

The complete prior Object Lights ledger set and the campaign/tooling rules
were read before source work. The clean HCEA reconstruction supplies the four
marker API names, their typed semantics, and the complete `0x350`-byte lights
globals layout. The January COFF and recovered assertion COMDATs independently
pin the assertion spelling, source path, source lines `0x664`, `0x66F`,
`0x67F`, and `0x68E`, store order, function boundaries, relocations, and
symbol destinations. The final-image `_lights_globals` symbol and the next
recovered image symbol also bound the aggregate to exactly `0x350` bytes.

One coherent four-function candidate was frozen because all four functions
share the same marker state. That single source shape produced three exact
functions and one miss. `_light_unmarked` had the correct semantics, 96-byte
padded extent, and all eight relocation identities, but January materializes
the marker word, clears EAX, compares through ECX, and then emits `setne al`;
the natural expression compares directly against memory and returns the byte
result. Its normalized hashes were:

- target: `6aa93207648f0793874a395bb091edda96bd71c50fd33a1eb3ef9d34ce6ff498`;
- candidate: `b9e841295d6336199b380756ca3fff9f8cdc6718cecc38b8d744c755cf318c75`.

That miss was removed in full and its recovered symbol name was restored to
`_code_00129180`. It was not tuned, respelled, retried, parked, or credited.
The three independently exact functions were retained and replayed once after
the removal.

## Typed state and ownership boundary

The source now names the full `struct lights_globals`, including the 128 scene
point-light indices and eight typed queued lens-flare records, with a compile
time `sizeof == 0x350` guard. This is ordinary typed source needed by the exact
marker functions; the light datum access also uses the existing typed
`datum_get` API.

The candidate compiler places this aggregate in BSS. The January split object
references `_lights_globals` through the recovered final-image symbol but does
not establish an Object Lights data-section owner for the 848-byte span.
Accordingly this wave claims no matched data and adds no semantic-data
adjudication. The ordinary report continues to count all 2,645 target runtime
data bytes in this object as uncredited. This is a source reconstruction and
typed layout claim, not a data-ownership claim.

## Policy and regression audit

The retained functions use readable C89, one parameter per line, typed fields,
named constants, and explicit returns. They contain no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, raw offset or
address, pointer/integer reconstruction, type pun, inactive-union access,
undefined behavior, byte forcing, synthetic caller/anchor, object patch,
comparator exception, alternate compiler, or flag substitution.

Direct hardened comparison proves all 15 accepted Object Lights functions
strict exact after the miss was removed. The 12 inherited exact functions are
unchanged: `_texture_cache_bitmap_load`, `_lights_dispose`,
`_lights_initialize_for_new_map`, `_lights_dispose_from_old_map`,
`_lights_enable`, `_light_delete`, `_light_disconnect_from_map`,
`_lights_disconnect_from_structure_bsp`, `_light_attenuation`,
`_code_00129100`, `_code_00129120`, and
`_lights_reconnect_to_structure_bsp`.

## Validation

- January split object SHA-256:
  `17f5b66319a615e7dc22e010f67c32bba4ca6b000dc691647b0d1645c1a76a5b`.
- Final rebuilt object SHA-256:
  `1007cd4d2ce176f2dde8ade2314fdf00666dfdc59e887fd57219c761433a6448`.
- Full Ninja build, split, ordinary report, semantic report, and progress:
  pass.
- Semantic audit: 470 units, 4,879 functions evaluated, 4,752 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,762 accepted exact, and zero unit
  errors.
- Aggregate progress: 382/833 linked files, 4,723/11,060 functions, and
  598,668/2,198,102 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `b6c46c65f09b3f16de073df8f29b5169fb3becf62ae010c75f80a8f91edeca29`.
- `build/semantic_report.json` SHA-256:
  `e80afe0d609ac3e13218bfd6648d50bfbd54ee6d2dbabd603e7b71dca76ecd62`.
- `git diff --check`: clean apart from Git line-ending notices.

Units remains untouched and exact. No tool file changed in this wave.
