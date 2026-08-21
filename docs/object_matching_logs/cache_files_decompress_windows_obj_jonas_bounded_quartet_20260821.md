# `cache_files_decompress_windows.obj` Jonas bounded quartet (2026-08-21)

## Result and fixed boundary

This Jonas/Codex ledger records one bounded, fail-closed quartet from
authoritative integration commit
`c7d4f56170f43f3ae38a061368692756eff77d70`. The isolated branch is
`jonas/cache-decompress-windows-quartet-20260821`. All four requested public
bodies were emitted together in one natural code-producing compile. Two were
strict immediately and are retained; the two misses were removed wholesale
without a source-shape retry or code-generation tune.

The final `cache_files_decompress_windows.obj` advances from 0/46 to 2/46
strict functions and remains `NonMatching`. The gain is 67/8,154 meaningful
code bytes, 80 padded bytes, and six relocations. It receives zero data
credit.

## Immutable target and source provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `8bd52d0a46a573022330820b6b073d6af6a053d2b669d6a4df0a4c67f204a55f`.
- Baseline translation-unit blob:
  `37bcd7f12404de464957d3385cc7bc2eecbbf94f`, 6,983 payload bytes,
  SHA-256
  `515ab6640829235258023ff534b113787d9f77a2c648ff5a83ffa6d74aeb8601`.
- Retained translation-unit blob:
  `0a50ca43de8db5c9201e476a1a62f10eb301c241`, 9,335 payload bytes,
  SHA-256
  `21b32e0a089aaaa1c004786185022aa308e424005bb49ac78ee71300591ccea6`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` has SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural flags remain `/O2 /Oy- /DDEBUG /Dxbox` with the repository's
  normal XDK 3911 include paths.

The raw candidate-object digest is phase-specific because VC7 writes a COFF
timestamp. The normalized function hashes below are the reproducible
admission identities.

## Clean HCEA donors

The semantic reference is the clean local HCEA tree at commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every cited file is reachable
from that tree and was read with `git cat-file`; the payload hashes are over
the exact Git-blob bytes.

| HCEA path / role | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `src/cache_copy_buffer_size.c` | `0cda2fadffe3ff1ba14872b7df128562c864b89d` | 456 | `3953548eda525a08bba6e9b91e5d390509b8b98e5824477deb9f1e28e53fe562` |
| `src/cache_copy_set_priority.c` | `1482c7813294cb20c5a60fd7c3ffff7371f49eb9` | 632 | `df628dcad1e11b22f4fa359e4139f82e3151f89ca7cbb8d7b38b4707e8bf04cf` |
| `src/cache_copy_compressed_file_complete.c` | `3cffd1d5536d13cb25c022ad1f6c6e54b093e270` | 284 | `e775729daf2e73932cd95f241f74edd7f3c06bcbef1a01c194c6e3ad987a225c` |
| `src/cache_copy_queue_end.c` | `57e1879ec3c0457f31f373d8453eebc42f09d2db` | 864 | `3b76455f08085e433b3b834d215126635f04c5f311c358953c9ce26eace4cf04` |
| `src/headers/simple_decompressor_definition.h` | `348e807c8186aced4ab0acfed26d1cd8f9642c44` | 4,207 | `3156036779df055bf207fcb5fbe37204d3b4a565832869c1ce26d8c299b25205` |

HCEA supplies readable behavior and public signatures. January PC COFF is
authoritative for field offsets, call decoration, bytes, padding, and
relocations.

## Typed PC layout and ABI

The HCEA layout is not copied blindly: it has `src_name[261]`, then alignment,
and its later fields are four bytes beyond the PC fields. The retained TU-local
PC prefix starts with `char src_name[260]`, the full named 0x800-byte cache
header, `flags`, and the real zlib `z_stream`; it then names the zlib buffer,
allocation, event, thread, and read/write-buffer fields through `blocking`.
It contains no catch-all state padding or offset-based access.

The local cache-header definition is authenticated by canonical
`source/cache/cache_files.c` blob
`c2719b499e63517f04a4ff826bdc67b0b1e855ba` (payload SHA-256
`7e397391419dfc6a661da575dc3d23655815f488393217dcb787101c86647691`).
The actual zlib type comes from `source/memory/zlib/zlib.h` blob
`49f56b43bc6c5bc89b103c4760a23330d6525086` (payload SHA-256
`7a13097001709cf69d7f1b7649fb6e93672164d92672b6f7b25fd43d3f220e49`).
Compile-time checks fix the cache header at 0x800 and the PC offsets:

| Field | Offset |
| --- | ---: |
| `zlib_stream` | `0x908` |
| `copy_stop_event` | `0x950` |
| `copy_complete_event` | `0x954` |
| `copy_thread` | `0x95C` |
| `blocking` | `0x988` |

The C identifier is `data_00316838`, so VC7 emits the target spelling
`_data_00316838`. It is declared only as an external pointer. The Win32
boundaries use local typed `__stdcall` declarations, producing
`_WaitForSingleObject@8` and `_SetEvent@4`; the exported functions remain
ordinary cdecl.

## One-shot quartet result

Before the code-producing compile, the exact natural command was exercised
once with `/Zs`. That parse-only prerequisite emitted no object. The next and
only candidate compile contained all four bodies. Its raw phase-specific
object SHA-256 was
`23b8016f0bbb249d75f65fe218da280e8d57875786b053982c6f569d32dd91f5`.

| Candidate | January RVA | Target meaningful / padded | Target relocs | Target normalized SHA-256 | First candidate result |
| --- | ---: | ---: | ---: | --- | --- |
| `_cache_copy_buffer_size` | `0x001A9C30` | `64 / 64` | 3 | `c30795d7bce37d74e9283fe4c389fb5d6b45f3252ebadb7609af05c847444cac` | rejected: 48 padded, 2 relocs, SHA `12a8662e2840a89ccf6c4fe267ed94174b996c2a39c5cceaa0a656fefcc8dd7e` |
| `_cache_copy_set_priority` | `0x001A9C70` | `54 / 64` | 3 | `4ca90f235dfbde68ac869a5d01438674a5c9dbaf2bb9544b2362ed628ae82943` | rejected: 80 padded, 5 relocs, SHA `c4f03df23e6c69566f7233e8e5bb646d5e4785c2e67ee105d4b5081938ad6fde` |
| `_cache_copy_compressed_file_complete` | `0x001A9CB0` | `25 / 32` | 2 | `0d2aff1d92c0e2d15e18bfa62eae9b3f6d97c107e7c80706ad7b17938fe89941` | **strict exact, retained** |
| `_cache_copy_queue_end` | `0x001A9FB0` | `42 / 48` | 4 | `fe7ed35596a552716ef2ab29339bdbee8465c21e10bfe02979e4604fa72fa092` | **strict exact, retained** |

The two rejects and their `SetThreadPriority` support declaration were removed
without retry. A later policy audit removed `volatile` from the unused layout
field `flags`; this was a support-only, layout-neutral correction. A preserved
A/B object comparison proves both retained code COMDATs and all six
relocations are identical before and after that cleanup.

## Scope, ownership, and exclusions

The target owns 5,945 ordinary non-code bytes: 3,265 `.bss`, eight `.data`,
and 2,672 `.rdata` bytes. The final candidate defines only its two code
COMDATs plus compiler directive/debug sections. It defines no `.bss`, `.data`,
COMMON symbol, runtime `.rdata`, or aggregate owner. `_data_00316838` remains
undefined at value zero, so all target non-code storage remains unclaimed.

`cache_copy_begin`, `cache_copy_get_status`, `cache_copy_end`,
`cache_copy_initialize`, `acquire_read_request`, both rejected candidates,
and every anonymous/private helper are absent from the final candidate. No
shared header, configuration, semantic exception, parked record, completion
label, storage owner, or other source file changes.

The retained implementation is readable typed defined C with explicit
terminal returns. It contains no assembly, raw address or byte-offset access,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined behavior, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, synthetic anchor, or byte-forcing expression.

## Pre-commit validation

- Complete 569-action `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened target comparison: both retained functions have exact
  normalized bytes, padded extents, relocation addresses/types/destinations,
  and addends. Both rejected symbols are absent from the final object.
- Semantic audit: 470 units, 4,128 functions evaluated, 3,988 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,049 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,036/11,060 exact functions,
  480,676/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 complete objects, 3,869/7,574 exact functions,
  467,762/1,770,166 meaningful code bytes, and 1,829,896/3,923,451 matched
  data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The pre-wave TU gate identifies exactly the retained pair as `NEWLY_EXACT`
  and reports `changed_nonexact: []`. It fail-closes on the expected new
  symbol set and compiler debug sections; no adjudication was added. Runtime
  non-code and external ownership were checked directly and are unchanged.
- `git diff --check`, deleted-path, source policy, protected-five, and frozen
  scope checks pass. Only the translation unit and this new Jonas-owned ledger
  are tracked changes.

The clean committed-state snapshot, forced object deletion/rebuild, regression
check, and final direct COFF replay are recorded in an additive follow-up
after the implementation commit. Nothing is pushed or history-rewritten.
