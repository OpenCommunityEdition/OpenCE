# `saved_game_files.obj` Jonas mutex-pair wave

## Result

This bounded wave starts from authoritative integration commit
`a60070d88533c9dcb1100f35dc3fa5080039284c`. The two HCEA-backed public
mutex wrappers were emitted together in exactly one natural candidate compile.
`_saved_game_files_release_mutex` is independently strict and is retained.
`_saved_game_files_take_mutex` missed and was removed immediately, without a
spelling, declaration, control-flow, or compiler-control retry. Its public
declaration remains corrected from `void` to `boolean`, because January callers
consume `AL` and establish that ABI independently of the rejected body.

`saved_game_files.obj` advances from 3/46 to 4/46 exact functions and remains
`NonMatching`.

| Retained function | January RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_saved_game_files_release_mutex` | `0x001B2600` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| **Wave gain** | | **13** | **16** | **2** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends. The object now reports
37/12,068 meaningful code bytes and four exact functions. The target's
remaining 42 functions and all 6,156 non-code bytes remain outside this wave's
credit.

## Provenance and committed source identities

- Baseline source/header blobs:
  `1554bbe133220cb81839e06cfc44da2dbea6af98` and
  `a2987f70ca4042934e8d68319d6bde7165f4888c`.
- Committed `saved_game_files.c` Git blob:
  `385312669fdcf6e9e9e2af13a799a33530e39c51`; raw Git-blob payload is
  12,350 bytes with SHA-256
  `e25de25f5719c23848d7d6dbc4ece9fd817754c7c1156c2262294aa7ce6c8a9a`.
- Committed `saved_game_files.h` Git blob:
  `5218f5d3aad875cd217e87e908f4e958ae952022`; raw Git-blob payload is 476
  bytes with SHA-256
  `b6c3c76a608cc82506f146eb512dd6b83dcb08e183c01f39722f0b4c40b0c065`.
  Post-commit `git cat-file blob HEAD:path` verifies both identities; they are
  raw Git payload hashes, not CRLF checkout hashes.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `bd000ef1323c25d9bba631cd0c1a0ef9c6f3bbca7bcc5a634fc9ec13bc8fedf4`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its tree-reachable exact-name
  take/release body blobs are
  `1a032f593d7725874527fd079420f7e07b8c8d48` and
  `500f12a9374ce7c291059b8b47f74a3a528cdffd`; its typed saved-game globals
  layout blob is `b83c097fdbaf9bc662cfddfbfb56ee99f99750c1`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The authenticated public signatures are:

```c
boolean saved_game_files_take_mutex(void);
void saved_game_files_release_mutex(void);
```

The retained implementation uses the typed helper contract:

```c
void release_mutex(
	struct mutex_reference *mutex_reference);
```

The existing `saved_game_files_globals` layout remains 0x11C bytes;
`general_mutex` is a typed `struct mutex_reference *` at offset 0x10C and
`memory_units_dirty` remains at offset 0x117. The existing compile-time layout
checks continue to pass. The corresponding take helper is independently
authenticated as returning `boolean` and taking the same mutex pointer plus an
`unsigned long` timeout, but its local declaration is absent from final source
because the rejected take body was removed.

## ABI and one-shot adjudication

January PC callers `_code_001b1dd0`, `_code_001b2040`, `_code_001b0e30`, and
`_code_001b10b0` execute `test al, al` immediately after calling
`_saved_game_files_take_mutex`. The marketing caller also calls it but ignores
the return. This proves the `boolean` return ABI and makes the former shared
header declaration `void saved_game_files_take_mutex(void)` incorrect.

The repository-wide header-consumer census finds exactly the owner TU and
`source/interface/marketing_and_strategic_business_development.c`. Both see a
`boolean` definition before the header. A controlled exact-base A/B compile of
the sole non-owner proves the header repair has no runtime effect:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_xbox_demos_available` | 96 | 7 | `a9509d28dc1cb8eb7cfc7f1220f5e9f83fb9de7b6d9a50191a0f8f2012677ad7` |
| `_IDirect3DDevice8_PersistDisplay@4` | 16 | 1 | `6d46436cb945dbb1b016f222c634a74f3596ac3dee003ade6338c25801aa8edb` |
| `_clean_up_for_image_launch` | 96 | 12 | `ba014659d5ab1cfea5bd8dd44f21c18e0da88887df6c57fb750adb94b45785f4` |
| `_xbox_demos_launch` | 112 | 7 | `45d6c5a8993df7f98dc602302e5a0759dd974c2e971961bb1f2ba51c076c836a` |
| `_xbox_dashboard_launch` | 128 | 4 | `b10206c4f8cfa04e67f4cb695c92728b1d4a0f7010de3ae56e7983f47bf4201f` |

All five code sections and 31 ordered relocations are identical. All 11
runtime `.rdata`, `.data`, and `.bss` sections are byte/relocation identical,
and all 23 external defined, undefined, and COMMON symbol records are
identical. In particular `_clean_up_for_image_launch` preserves its relocation
to `_saved_game_files_take_mutex`. The raw A/B objects are different only
because `.debug$S` grows from 228 to 230 bytes, consistent with the intentional
return-type metadata repair: baseline SHA-256
`7c786e6bb4261b9bf7f6e6516cb0222471dd066923c4f8e5ff626cb0bff0c798`,
candidate SHA-256
`c988b632df22161684dab56c8b5384f38a275b9ee235ba6be5885fa724289940`.

Both requested bodies were compiled together once. The rejected body is:

| Rejected function | January RVA | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | ---: | --- | --- |
| `_saved_game_files_take_mutex` | `0x001B25E0` | `20 / 32 / 2` | `07436c97d42902601853f9d1a160b3ce8553c576612327a4a6c6de3fdd85dd5c` | `32 / 2`, SHA `b0ab8c4ba30254f1e0888e81188c92f96d6983e03af52aae890468cef3044301`; relocation schedule and identities are exact, but normalized instruction bytes differ. |

For take, the common relocations are `DIR32 _saved_game_files_globals+0x10C`
at `+0x01` and `REL32 _take_mutex` at `+0x0C`. For release, they are
`DIR32 _saved_game_files_globals+0x10C` at `+0x01` and
`REL32 _release_mutex` at `+0x07`. The rejected take COMDAT and its unused
local helper declaration are absent from the final object/source. Reopen it
only with independently preserved January source or a newly authenticated
ordinary-C contract; do not tune from the rejected object.

## Ownership, policy, scope, and validation

`saved_game_files_globals` remains an undefined external in the candidate
object (`section 0`, value zero). The target owns it in `.bss`; the hardened
comparison resolves the release relocation to that owner plus addend 0x10C.
This wave adds no `.bss`, `.data`, `.rdata`, COMMON symbol, or storage
definition and claims zero data bytes.

Only `source/saved games/saved_game_files.c`, its necessary public header, and
this new Jonas-owned ledger are changed. No configuration, semantic exception,
parked record, completion label, frozen-five source, pre-existing Markdown,
or other source file is edited; no tracked file is deleted.

The retained source is readable typed C with explicit returns. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, pointer or union pun, inactive-union access, undefined
overflow, volatile scheduling device, force-inline annotation, optimizer
pragma/barrier, synthetic anchor, or object-byte forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened final comparison: the retained release wrapper and inherited
  `_saved_game_file_get_type`,
  `_saved_game_files_notify_memory_units_changed`, and
  `_enumerate_memory_units_test` all pass. The rejected take symbol is absent
  from the final candidate COFF symbol table.
- Semantic audit: 470 units, 4,123 functions evaluated, 3,983 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,044 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,031/11,060 exact functions,
  480,401/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds one function and 13
  meaningful bytes with no data increase.
- `halobetacache`: 273/468 complete objects, 3,864/7,574 exact functions,
  467,487/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot followed by deletion and forced rebuilding
of `saved_game_files.obj` is the final same-path reproducibility proof. The
post-rebuild hardened comparison remains strict for all four accepted
functions, and the rejected take symbol remains absent. No push is performed.
