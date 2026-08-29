# `devices.obj`: runtime-group deletion (2026-08-29)

## Result

`_device_delete` is newly strict-exact on its first and only production
candidate compile. The wave adds one function and 144 padded code bytes.
`source/devices/devices.obj` advances from 12/30 to 13/30 strict functions
and from 640/4,688 to 784/4,688 strict padded bytes. The object remains
`NonMatching`; no object-completion, data, semantic exception, park, or
compiler-control claim is made.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_device_delete` | 144 | 9 | `c54809e0d5b1f1b8f471b192d26b0049b278e41f8c67db9a277e46d8f6f6f075` |

Every normalized byte, padded byte, relocation address, type, destination,
owner, and addend agrees with the January target.

## Authority and reconstruction

The January target COFF and disassembly are the byte and ABI authority. They
show one checked device lookup, followed in order by the signed power-group
and position-group indices. Each assigned group is obtained through the typed
data array, and is deleted only when bit two of its flags word is set.

The independently reconstructed HCEA source at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/device_delete.c`
confirms the two-group behavior and identifies bit two as the runtime-group
flag. Its foreign addresses and direct backing-storage view were not imported.
The production implementation instead uses the repository's existing typed
`device_get`, `datum_get`, and `datum_delete` contracts, the verified
`device_group_datum`, and named device fields.

## One-shot and rejected sibling

The deletion body's natural typed form was frozen before one normal Ninja
compile and was strict immediately. No alternate spelling, body retry,
compiler flag, adjudication, comparator exception, or object manipulation was
used.

The same research wave tested one separately frozen natural reconstruction of
`_device_effect_new`. It produced the target's 176-byte padded extent and all
nine relocation identities, but ordered the effect arm ahead of January's
fatal/sound/effect topology. Its candidate normalized SHA-256 was
`bdb2ac51c5215e4c832029732d898d5e52f111c49ba0ea0991e04ff197d976c0`
versus target
`fa087c5a822e926b4b191ea1927bd30c696c5577513d6d3b632c586044239a48`.
That body and its includes were removed wholesale without branch-shape tuning
and receive no credit. Reopen only with new source evidence for January's
control-flow topology.

The retained deletion code is C89-compatible readable C with one parameter
per line and an explicit terminal `return;`. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw address or
offset, pointer/integer reconstruction, representation pun, undefined
behavior, private ABI, synthetic anchor, or byte-forcing expression.

## Validation

- January `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Final rebuilt `devices.obj` SHA-256:
  `501e709d061ffacea6d25476a1039ee5e8af7e6bf9296d3d790eb10bf92496b6`.
- Complete Halo and libcmt builds, ordinary report, semantic report, and
  strict progress: pass.
- Strict campaign board: 276/619 whole objects, 4,713/8,246 functions, and
  652,775/1,922,669 padded code bytes. This wave changes only the latter two
  totals by one function and 144 bytes.
- Semantic audit: 470 units, 4,858 functions evaluated, 4,731 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,741 accepted exact, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `1119bf092c9d0c17dcbf29bea8a7ca1ebdfe3edaa77d568061cba0a9412ddad3`.
- `build/semantic_report.json` SHA-256:
  `59a9b1da9ea61bd485b9ea34506be213ea6c7f851f61c9678b7736afc3c923cb`.
- `git diff --check`: clean apart from Git's line-ending notices.
