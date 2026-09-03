# Opus network/cache reconciliation (2026-09-02)

## Scope and provenance

- Admission base: `36ec431656b1674265c0986938590e106994b894`.
- Donor only: Opus commit `bb0d690a3`.
- Audited implementation units:
  - `source/networking/network_game_manager.c`
  - `source/networking/network_client_message_handler.c`
  - `source/cache/cache_files_windows.c`
- The only out-of-unit implementation edit is the declaration-only prerequisite in
  `source/cache/cache_files.c`: its incorrect local `void cache_file_read(...)`
  declaration and duplicate Windows-cache declarations were removed so the true
  owner header supplies the target-proved `short cache_file_read(...)` interface.
  The unit remains 28/28 strict exact.

The donor was not merged wholesale. Each target section was compiled in an
isolated worktree and checked against the January split object. Public names and
RVAs were checked against the local PDB evidence; private handler names are also
spelled by the target's diagnostic strings. HaloCEA independently supplies the
named `network_game_dump`, async cache-read, cache-request, and surrounding cache
behavior. Repository and Marathon conventions were used for types, ownership,
declaration placement, and formatting, not as a substitute for January behavior.

## Accepted progress

| Unit | Before E/R/U | After E/R/U | Exact function delta | Exact padded-byte delta |
|---|---:|---:|---:|---:|
| `network_game_manager` | 13/0/6 | 19/0/0 | +6 | +1,728 |
| `network_client_message_handler` | 0/0/17 | 17/0/0 | +17 | +4,224 |
| `cache_files_windows` | 7/0/43 | 48/2/0 | +41 | +4,896 |
| **Total** | **20/0/66** | **84/2/0** | **+64** | **+10,848** |

The networking objects are fully strict exact. The cache object has all 50
target functions implemented: 48 are strict exact and the remaining two are
honest fuzzy parks recorded in `config/parked.json`.

The accepted cache reconstruction also recovers the TU-private
`cache_file_globals` owner. Candidate and target `.bss` are both 12,412 bytes,
have no relocations, and share normalized SHA-256
`1a572b5284f04cf7f05e01ce72c659ad00bb2732ba8ab1bf09813e61de399ff9`.
The semantic static name is supported by January's own
`cache_file_globals.*` assertion strings and by HaloCEA's independently named
cache global; no `bss_004cdff8` alias remains.

Owner-interface repairs accepted with the implementation:

- networking public APIs now live in the corresponding manager, handler, UI,
  or transport header rather than in consumer `.c` files;
- cache-file, cache-copy, and texture-cache APIs now live in their closest
  owner headers;
- `enum cache_copy_status` and the structure-BSP rasterizer lock constant use
  their semantic owner headers;
- `global_frame_parameters` is an `extern` declaration instead of a tentative
  definition emitted by every `rasterizer.h` consumer;
- Win32/Xbox handles and constants use `HANDLE` and
  `INVALID_HANDLE_VALUE`; scenario and message switch cases use their enums;
- asynchronous completion storage and request observations are volatile for
  the real cross-thread callback boundary.

## Fuzzy parks

### `_cached_map_file_read_header`

- target/candidate padded size: 400/400;
- relocations: 26/26 with identical identities;
- objdiff similarity: 98.70079%;
- target SHA-256: `1a3412aeb8f52881b0a0317f1671f1bef803f138ec9d594be08e349072fe034a`;
- candidate SHA-256: `5abb084355a19b12c72870099def7db0523ce8f3e2ff464cb2f75c69e2ce7bfe`.

The retained typed implementation agrees with January and the independent
HaloCEA routine. The residual is VC7 register choice plus a redundant volatile
completion reload/control-flow schedule. Making the completion local
non-volatile worsened the authentic initialization schedule and was rejected.

### `_cache_files_open_cache_files`

- target/candidate padded size: 752/752;
- relocations: 47/47 with identical identities;
- objdiff similarity: 83.333336%;
- target SHA-256: `f1c88310c585856cc88840f5dfa06dfa579231bca2e1f57c06cf7d00803c7a3f`;
- candidate SHA-256: `5ac13a55c15d04794a7e301ed502b8962f97c1466484b36d13aacec0ff3648bc`.

The complete open/create/resize/header-validation loop and every call/string
relocation are present. The remaining difference is broad VC7 cold-block and
register scheduling. Goto reshaping, fake aliases, volatile/register forcing,
and other byte-directed source distortions were not admitted.

## Rejected or corrected donor material

- All `code_<address>` private names and the `bss_<address>` cache global were
  rejected and replaced with target-supported subsystem names; private entries
  are marked static in `config/symbols.json`.
- Consumer-local public prototypes were rejected. Declarations were moved to
  their real owner headers, including the `short` return type of
  `cache_file_read`.
- The cache-local duplicate rasterizer enum was rejected in favor of the real
  enum constant in `rasterizer.h`.
- Non-volatile async completion declarations were corrected. No inert load,
  barrier, pragma, forced-inline marker, raw address/offset access, fake linkage
  anchor, undefined behavior, or handwritten assembly was accepted.
- A non-volatile experiment for the 400-byte header reader remained residual
  and moved initialization away from January's schedule, so it was discarded.

One disclosed object-level caveat remains: the authentic stock `<xtl.h>` include
naturally emits three linker-discardable selectany D3D lookup-table COMDATs that
are absent from the linked January target split. No synthetic partial-XDK header
or fake suppression was introduced merely to hide them. The five target D3D
resource wrapper functions themselves are strict exact.

## Verification

- Focused gates:
  - `network_game_manager`: 19 exact, 0 residual, 0 unwritten;
  - `network_client_message_handler`: 17 exact, 0 residual, 0 unwritten;
  - `cache_files_windows`: 48 exact, 2 residual, 0 unwritten;
  - `cache_files`: 28 exact, 0 residual, 0 unwritten;
  - `units`: 189 exact, 0 residual, 0 unwritten.
- Full baseline and candidate builds: `ninja halobetacache_build libcmt_build`
  compiled all 572 objects successfully.
- Rename-stable whole-tree snapshots: 5,403 exact before, 5,467 exact after;
  **64 gained / 10,848 bytes / 0 regressions** across 8,245 target functions.
- Header blast: the full build recompiles every direct/transitive consumer of
  the nine changed headers; the stable sweep found no lost exact section.
- Fake-match scan: 3 scoped implementation files, 0 review leads.
- Park manifest: 153 active, 0 stale, 0 invalid.
- Tool tests: 261 passed.
- `git diff --check`: clean.
