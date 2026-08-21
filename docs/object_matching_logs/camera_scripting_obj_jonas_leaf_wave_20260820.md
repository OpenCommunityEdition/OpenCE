# `camera_scripting.obj` Jonas leaf wave

This Jonas/Codex ledger records one bounded six-function recovery wave for
`source/camera/camera_scripting.obj`. The object advances from 0/13 to 6/13
strict functions and remains `NonMatching`. No source-owned data definition or
data credit is added, and every pre-existing or Claude-authored Markdown file
remains untouched.

## Scope and provenance

- Authoritative integration base:
  `09879dccf6bab47ac42dc3476125ff6fd1f942e7`.
- Baseline `camera_scripting.c` blob:
  `260edd237d60e1fbebe2baa9e2063ae06a826242`.
- Final source blob:
  `e483b46fb5590de28539e4b79460b67541fdb4e3`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `5be1b15bb8f628bd795b912bbea2f45b89cedd4b0f69b2ec99247a33ba408075`.
- HCEA semantic oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its authenticated anonymous
  camera-globals layout blob is
  `751d8b03e9cd57baabac4ee2923fc340cd323c4b`, and the enable-leaf blob is
  `1cf9cfc14b16fa0216f700413bf736fc1e40de39`.
- The six hash-pinned cross-build packets are
  `8a56f0f3f7938fb23a4bdd1a385c6f74ff6eca2ee23bc335e15e05a9f4794ae9`,
  `8ec8ec8940798daa7eb79c7b8938d1665fa5b1d98da1a62c2221bfe253672cf7`,
  `19b786b7b47af464f9e26031275b03388d6842cf024a4dcdc8bc18522a2fffa3`,
  `56cb32dd53b3d7f9d0dc09b67216b8e17eb2f5385e6d9e885562a22f8b3f5eb3`,
  `f29c55811c4ebf3bb583823ea17a6ed53c9e3bb8191bdf9fc00f1f2f986beafb`,
  and
  `390843a3f0cb4b7688373316643c54c04ab8824566cffb76fc6ea398218293ea`.
- HCEA supplies typed names, signatures, field meanings, and wrapper intent.
  January disassembly and relocations are the sole byte, call-order, and
  storage-offset authority.
- Only `source/camera/camera_scripting.c` and this new ledger are changed.
  There is no shared-header, configuration, frozen-object, or
  existing-Markdown edit.

## Bounded experiment and strict result

Exactly the six requested independent leaves were compiled as one natural
candidate. All six were strict on that first compile, so there was no
source-shape or code-generation tuning pass. The later complete-build and
post-commit rebuilds replay the retained source; they are not new candidates.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_scripted_camera_enable` | 20 | 32 | 2 | `691fd2268aea40f2a3e6c3ddb5b4a7eb49c3d15117d924804e0477d4825e7c9c` |
| `_scripted_camera_set_absolute` | 23 | 32 | 1 | `0e0e7c6b6817a5675c9be883f8667758a8c6c39bc7ab544325485026467c88d5` |
| `_scripted_camera_set_camera_point_absolute` | 35 | 48 | 1 | `cf4632da168837688f7cbddfbaccbeaf91af48256a5b8b37b72b1eec8e6ea947` |
| `_scripted_camera_next_camera_point` | 7 | 16 | 1 | `1f82cf78b0df84e5b0bf619acb9ebfc8c165ff2558679479ef137123faa560a2` |
| `_scripted_camera_object_relative_to` | 6 | 16 | 1 | `1e7169478a5de2d7d707c15b39f24ee5b545e536a2ea0f1383ec56294f9ee3bb` |
| `_scripted_camera_time` | 17 | 32 | 3 | `afdb1f624faa5446a5e842c3652d550675aeebcec53638cd45bb5fdf736f772a` |
| **Wave delta** | **108** | **176** | **9** | |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends for every retained function. The
object now measures 6/13 functions and 108/2,534 meaningful code bytes.
The first-person, dead-camera, animation, relative-point, main setter, and
update bodies were deliberately excluded from this leaf wave.

## Typed state and ABI

The source declares `data_002dcb60` only as an external typed view. Its named
fields are authenticated at offsets `+0x00` (`enabled`), `+0x01`
(`first_update`), `+0x02` (`mode`), `+0x04` (`camera_point_index`), `+0x08`
(`timer`), and `+0x34` (`relative_object_index`); all intervening bytes are
reserved. Direct COFF inspection proves `_data_002dcb60` remains an undefined
external symbol with value zero. The source emits no COMMON, `.bss`, or
`.data` owner. The compiler-generated `30.0f` COMDAT is required by the exact
time conversion but is not granted data credit. All 172 target non-code bytes
remain uncredited, including the 64-byte camera-global owner.

The two wrapper callees have typed declarations derived from the HCEA
signatures and January call shapes. Their transition arguments remain the
repository's established January `word` ABI; the wrappers pass `NONE` as the
long relative-object index. No first-person or dead-camera setter declaration
was added, and no shared header was needed.

All retained bodies are readable, defined C. No-argument functions use
explicit `void`, parameters appear one per line, and every void function has an
explicit terminal `return;`. There is no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, raw integer-address access,
pointer/integer reconstruction, union pun, signed-overflow match, synthetic
anchor, or byte/code-generation forcing.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all six retained functions pass.
- Semantic audit: 470 units, 3,984 functions evaluated, 3,844 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,905 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,892/11,060 exact functions,
  471,897/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
is performed.
