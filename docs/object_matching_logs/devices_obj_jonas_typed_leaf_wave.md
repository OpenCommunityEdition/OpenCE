# `devices.obj` Jonas typed-leaf wave

This Jonas-owned ledger records one bounded typed-leaf recovery wave for
`source/devices/devices.obj`. The object advances from 1/30 to 4/30 strict
functions and remains `NonMatching`; no storage, initialized-data, or
completion claim is made. No pre-existing or Claude-authored Markdown file is
edited.

## Scope and provenance

- Authoritative integration base:
  `09879dccf6bab47ac42dc3476125ff6fd1f942e7`.
- Baseline `devices.c` blob:
  `4595411951f3a112513dcff3bc0b72d327b3f29f`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- The base already supplies the typed `device_datum` and `machine_datum`
  layouts, the `device_get` and `machine_try_and_get` accessors, the object
  masks, and the machine runtime-flag enum. January COFF disassembly is the
  sole byte and relocation authority and independently proves the retained
  accesses and branch behavior.
- Only `source/devices/devices.c` and this new ledger are changed. There is no
  shared-header, configuration, frozen-object, or existing-Markdown edit.

For `_device_set_never_appears_locked`, January proves the `NONE` guard, the
checked machine mask `0x80`, and bit two of the machine flags at `+0x1C4`.
`_device_one_sided_set` uses the same checked machine access without a separate
`NONE` guard and changes bit one. `_device_set_power` uses the checked device
mask `0x380`, sets bit two of the device flags at `+0x1A4`, stores the power at
`+0x1AC`, reads the signed power-group index at `+0x1A8`, and passes that group
and power to `device_group_set_desired_value`.

## Bounded experiment

The one natural candidate compile covered exactly six requested leaves. Three
were strict immediately. The other three were removed without a source-shape
or code-generation tuning pass:

- `_device_get_position` and `_device_get_power` each had the target's
  48-byte extent and two relocation identities, but the natural early-zero
  branch emitted the zero relocation before the object-access call while the
  target lays out the device return first.
- `_device_operates_automatically_set` had the target's 64-byte extent and
  checked-machine relocation, but VC7 selected the opposite equivalent branch
  layout for the inverse runtime flag.

No rejected definition or reference remains in the candidate object.

## Strict code result

The untouched baseline measured 1/30 exact functions and one meaningful code
byte. The retained source measures 4/30 and 191 meaningful bytes: a gain of
three functions, 190 meaningful bytes, 208 padded bytes, and four relocations.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_device_set_never_appears_locked` | 64 | 64 | 1 | `e180e3b5196b04ad2c72967cdc93bdc207a23cc33b12d406864567ff38d55e36` |
| `_device_one_sided_set` | 59 | 64 | 1 | `37b297855afc1da1f38223444572043ab773a86d1831a3c828a24175f687bfb6` |
| `_device_set_power` | 67 | 80 | 2 | `77bb40070eb2a2a67b2d36bc1bae62ffce1cf94b7a3babc9cd3eacd37972b7be` |

For all three retained functions, target and candidate have identical
normalized bytes, padded extent, relocation addresses/types/destinations, and
one unique external function owner with matching COFF type and storage class.
The pre-existing exact `_devices_dispose` remains exact at 1/16 bytes with no
relocations and normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.

## Data and source policy

The candidate emits only the four function COMDATs plus directive and debug
sections. It emits no `.bss`, `.data`, `.rdata`, or COMMON owner, and the
target's 208 non-code bytes remain entirely uncredited.

All retained bodies are readable, defined C with one parameter per line and
explicit terminal returns. They use typed device and machine fields and the
existing verified object accessors. There is no assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, raw address
reconstruction, pointer/integer cast, union pun, signed-overflow match,
synthetic anchor, or byte-forcing construct.

## Reproducibility and gates

- Final tracked source blob:
  `04613ddc1f1aa7fd2a06a616728870fd2214b70e`.
- Final source SHA-256:
  `284314c9c0633deed794562f96e9e6c3502048bfb3735221d94d4ba6fc1c7db3`.
- Complete Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,981 functions evaluated, 3,841 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,902 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,889/11,060 exact functions,
  471,979/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission reports zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` completion-label contradiction.
- Park validation reports three active, zero stale, and zero invalid entries.
- The complete tooling suite passes 179/179 tests.
- A clean committed regression snapshot/check and forced object replay preserve
  the four exact functions, their relocation identities, and the zero-runtime-
  data candidate shape.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
is performed.
