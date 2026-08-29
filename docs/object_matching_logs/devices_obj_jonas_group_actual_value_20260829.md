# `devices.obj`: actual group-value propagation (2026-08-29)

## Result

`_device_group_set_actual_value` is newly strict-exact on its first and only
production candidate compile. The wave adds one function and 224 padded code
bytes. `source/devices/devices.obj` advances from 13/30 to 14/30 strict
functions and from 784/4,688 to 1,008/4,688 strict padded bytes. The object
remains `NonMatching`; no object-completion, data, semantic exception, park,
or compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_device_group_set_actual_value` | 224 | 7 | `e4a3615b74d495f8f8adcc7b1ee47324b0bb215b73b5fd0ad48e3f8e16df2f35` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with the January target. The seven relocations are
the zero and one real constants, `_device_groups_data`, `_datum_get`,
`_object_iterator_new`, and two calls to `_object_iterator_next`.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show an inclusive clamp to the unit interval, one typed group lookup and
write, and a device-object iteration. Matching power and position groups each
set the named position-changed flag, copy the actual value into the relevant
device scalar, and clear its velocity.

The independently reconstructed HCEA source at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/device_group_set_actual_value.c`
confirms that behavior. Its foreign backing-storage view was not imported.
The production implementation uses the repository's typed `datum_get`,
`object_iterator_new`, and `object_iterator_next` contracts, the verified
`device_group_datum` and `device_datum` structures, and named fields and flag.

## One-shot and rejected sibling

The propagation body's natural typed form was frozen before one normal Ninja
compile and was strict immediately. No alternate spelling, body retry,
compiler flag, adjudication, comparator exception, or object manipulation was
used.

The same research wave tested one separately frozen natural reconstruction of
`_device_can_change_position`. It preserved all five target relocation
identities but compiled to 144 padded bytes against the target's 128 because
the compiler retained an extra `change_allowed` lifetime. That body was
removed wholesale without declaration or branch-shape tuning and receives no
credit. Reopen only with new source evidence for the January lifetime and
control-flow shape.

The retained propagation code is C89-compatible readable C with one parameter
per line and an explicit terminal `return;`. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw address or
offset, pointer/integer reconstruction, representation pun, undefined
behavior, private ABI, synthetic anchor, or byte-forcing expression.

## Validation

- January `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Final rebuilt `devices.obj` SHA-256:
  `5652643385836311ef6df4907ea3f34a55b95088c55630bad44157d4963b3d52`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,716/8,246 functions, and
  653,319/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 224 bytes.
- Semantic audit: 470 units, 4,861 functions evaluated, 4,734 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,744 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `bb8365a584be3b03df37a3b67facab6cf626117195c976e4af76a7894d19ea49`.
- `build/semantic_report.json` SHA-256:
  `22f84023ba320c388dc2b6b34cc43818611ead5b525ee52a461b282432b073fc`.
- `git diff --check`: clean apart from Git's line-ending notices.
