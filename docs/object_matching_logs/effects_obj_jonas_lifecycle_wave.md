# `effects.obj` Jonas lifecycle wave

This Jonas/Codex ledger records one bounded lifecycle recovery wave for
`source/effects/effects.obj`. The object advances from 1/41 to 4/41 strict
functions and remains `NonMatching`; no storage, initialized-data, or
completion claim is made. No pre-existing or Claude-authored Markdown file is
edited.

## Scope and provenance

- Authoritative integration base:
  `9097dbb7dad5ddac695af090e722c4c34df12926`.
- Baseline source blob: `148052352442f3393f63e806cec7fd1bf80d610a`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `b9a46dba146ee46c39600e5bf5b181e620e971c75644be2f2ff3d1475ff362b2`.
- HCEA applied-source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- HCEA supplies the typed names, signatures, and lifecycle behavior for the
  two `data_array *` pools. January labels and executable disassembly
  independently prove the call and store order at `0x0008AD10`, `0x0008AD30`,
  and `0x0008AD50`: effect pool first, then effect-location pool; map startup
  calls `data_make_valid`, old-map disposal calls `data_make_invalid`, and
  final disposal conditionally clears both pointers.
- Only `source/effects/effects.c` and this new ledger are changed. There is no
  shared-header, configuration, frozen-object, or existing-Markdown edit.

## Bounded experiment and strict result

The one natural, code-producing candidate compile covered exactly the three
requested lifecycle functions. All three were strict immediately, so there
was no source-shape or code-generation tuning pass. An earlier compiler
invocation stopped during parsing, before emitting a candidate object, because
`memory/data.h` relies on the cseries typedef context; adding the ordinary
`cseries/cseries.h` include closed that compile dependency without changing a
body.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_effects_initialize_for_new_map` | 27 | 32 | 4 | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` |
| `_effects_dispose_from_old_map` | 27 | 32 | 4 | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` |
| `_effects_dispose` | 31 | 32 | 4 | `57c2ce5afbda11a72425091cebf748417b9bccf805b372d1aa9ba35baf61818d` |

The wave gains three functions, 85 meaningful bytes, 96 padded bytes, and 12
relocations. The pre-existing exact no-op
`_effects_disconnect_from_structure_bsp` remains exact at 1/16 bytes with no
relocations and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.
The final object therefore measures 4/41 strict functions, 86 meaningful
bytes, 112 padded bytes, and 12 relocations.

For every retained function, target and candidate have identical normalized
bytes, padded extent, relocation addresses/types/destinations, and one unique
external function owner with matching COFF type and storage class.

## External state and source policy

`effect_data` and `effect_location_data` are declared only as typed
`extern struct data_array *` pointers. In the candidate COFF they are undefined
external symbols with value zero. The object emits no COMMON owner and no
`.bss`, `.data`, or `.rdata` section, so this wave claims zero data bytes. The
target's 2,205 non-code bytes remain entirely uncredited.

All retained bodies are readable, defined C. No-argument functions use
explicit `void`, each recovered void function has an explicit terminal
`return;`, and the pool operations use their typed declarations from
`memory/data.h`. There is no assembly, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, raw address reconstruction, pointer or
integer cast, union pun, signed-overflow match, synthetic anchor, or
byte-forcing construct.

## Validation

- Final source SHA-256:
  `dc3461e7af198365282fc0e37ca89bedaad79dbf40f2ff26fba83aaa7ea7f505`.
- Complete Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,972 functions evaluated, 3,832 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,893 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,880/11,060 exact functions,
  471,681/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
is performed.
