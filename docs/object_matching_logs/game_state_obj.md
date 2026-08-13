# game_state.obj matching log

## Result

`source/saved games/game_state.obj` is strict-complete: all 24 functions,
all target-owned read-only data, the callback table, and the BSS aggregate
match the January 14, 2002 object under XDK 3911 CL 13.00.9254.1 with the
repository flags `/O2 /Oy- /DDEBUG /Dxbox`.

The final lift added the two private helpers, GPU arena allocation, persistent
storage restoration, and core-file loading. The existing 18 exact functions
were rechecked after every source change.

## January-authoritative recovery

- `code_001af4f0` is PDB-correlated with the later name
  `game_state_header_valid`. January gives it a private static convention:
  `header` in ESI, `halt_on_error` in BL, and the boolean result in AL.
- The validator is one ordered `if` / `else if` ladder over build number, map
  name, allocation checksum, player count, and cache-file checksum. In the
  nonfatal persistent-load path VC7 inlines and constant-folds it. In
  `game_state_load_core` it remains an out-of-line private-ABI call.
- `code_001af650` is the allocation logger. VC7 retains one cdecl copy and
  inlines it into both CPU and GPU arena allocators. The GPU flag selects the
  `"*"` suffix.
- The GPU allocator grows downward from `base + 0x345000`; CPU and GPU limits
  are `0x305000` and `0x40000` respectively.
- Persistent restore naturally inlines the one-entry before-load callback and
  `game_state_save`, while leaving the 13-entry after-load dispatcher as a
  call. Core loading makes the opposite contextual inlining choice for the
  after-load dispatcher. These are compiler consequences of the shared
  readable helpers, not hand-duplicated machine code.

The later HCEX decompilation was used only to suggest names and topology.
January disassembly, strings, relocations, sizes, and compiler output were the
acceptance authority.

## Preserved original defect

The build-number failure text is `"expected build #%d but got #%d"`, but the
arguments are string pointers. This original diagnostic-format bug is
preserved for exact reconstruction. A bug-fixed derivative should replace
both `%d` conversions with `%s`; doing that in this exact branch would change
the owned string and relocation identity.

## Strict function evidence

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `code_001af4f0` | 352 | 29 | `350352f1b90205366b540a4de2ceed508368e45f565b1501a1f1a734b903ac92` |
| `code_001af650` | 112 | 11 | `8f67474089a0c650702bf7901d9fe8c32d7b4bcd95c99766c25776c451b68e3b` |
| `game_state_malloc` | 272 | 29 | `947befa023fcb18727260717911e26a200633a81ce43001ccd20e14fefde8c4d` |
| `game_state_gpu_malloc` | 272 | 29 | `edb4f69ac452cdbc47ef929fef9ee43007045b9fb02fe4c7d9c3770ebdeba96e` |
| `game_state_try_and_load_from_persistent_storage` | 240 | 21 | `d3673bad9698327d0460e4b837d8f75949c6701c8dc381e7a044407a256e4aae` |
| `game_state_load_core` | 160 | 10 | `2424a9384c9f1edb3f17c0b3c72137e931068c7936bb749dc25e1e4af5eb30b6` |

The remaining 18 functions also pass `section_infos_equal`; the complete
object total is 24/24 functions and 2,624/2,624 padded code bytes.

## Data and ownership audit

- Callback `.data`: 60/60 bytes, 15/15 relocations, exact destinations and
  order. It contains one before-save, one before-load, and thirteen after-load
  callbacks.
- BSS: 32/32 bytes. The file pointer precedes the 28-byte game-state aggregate
  at the January offsets.
- All 18 target-owned string COMDAT sections are exact, including assertion
  text, logger format/path, and core-load status strings.
- XDK headers also instantiate three unrelated select-any D3D lookup tables in
  the rebuilt object. They are not target-owned sections and are benign header
  artifacts; no target data credit depends on them.
- The target csplit object exposes private symbols externally, whereas MSVC
  emits the two private helpers and callback sub-symbols as static. Strict
  function and relocation-destination equality plus the whole-section data
  checks prove the semantic ownership.

## Source shapes tried

The faithful shared-helper form was sufficient. Replacing the existing inline
logger in `game_state_malloc` with a call caused VC7 to inline the helper back
to the exact January instruction stream and simultaneously emitted the exact
out-of-line helper. No speculative barrier, pragma, compiler flag, assembly,
volatile qualifier, undefined behavior, or byte patch was needed.

## House-rule audit

- No-argument declarations put `void` on its own line.
- Every parameter is on its own line in newly added declarations/definitions.
- Callback types explicitly use `(void)`.
- Every added function has an explicit final return and one source-level
  return.
- No raw tag/object/datum getter casts were introduced.
- No alignment directives or non-default packing were introduced.

## Current campaign revalidation (2026-08-13)

This atomic lane began at authoritative tip `a8114eb7`, where
`game_state.obj` was still marked `NonMatching`. Before reconstructing the
remaining functions again, the lane audited all local refs and worktrees and
found the completed but unintegrated result at `a9c624c8`. That commit was
transplanted onto the current tip in an isolated branch and independently
rebuilt with XDK 3911 CL 13.00.9254.1 and the unchanged repository flags.

In plain English, the object now reproduces all of the original save-state
machinery: its lifecycle wrappers, allocation arenas, checksum validation,
persistent-storage path, callback table, and global state storage. The later
HCEX material helped identify intent, but every accepted byte, call target,
string, and layout came from the January target object.

The hardened comparator reproduced all 24 code sections:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_dummy` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_game_state_call_before_save_procs` | 16 | 1 | `247599aa11966badf77d9bbec40b25aca3a5e0bc652cb153ca2a57eb58b7c953` |
| `_game_state_call_before_load_procs` | 16 | 1 | `247599aa11966badf77d9bbec40b25aca3a5e0bc652cb153ca2a57eb58b7c953` |
| `_game_state_call_after_load_procs` | 32 | 1 | `29564bc017611472ab8b9a3d80b764564006f2251ac98cffd5ad054321384c62` |
| `_game_state_dispose` | 16 | 2 | `9f760b763648bf9f93ffa324169a776403fafb81f229b53514562ce23ecc4cad` |
| `_game_state_initialize_for_new_map` | 176 | 20 | `1c42a262a8e5d490bc55bd146085ab04f97c11fcf80fb687dcadc803afd7ac1e` |
| `_game_state_dispose_from_old_map` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_game_state_save` | 32 | 5 | `c5e4b6fdde3e9149ba71a013848b6f9c1235b29fc4014c014c7507dbde060171` |
| `_game_state_revert` | 64 | 6 | `a33cc8fffd018e66187b004bf0d4980fdf8ae1c87458798d131092275ecf040d` |
| `_game_state_save_to_persistent_storage` | 64 | 5 | `8c480b60afb7fabfa0d3862a1504b16f747daf1e82fbc2ea7f9563025ad2076c` |
| `_game_state_test_persistent_storage` | 112 | 4 | `6f8621fad595fd42c8fb7f060435f8596a0f98b7e1779b7f5a63bc27047584fd` |
| `_game_state_save_core` | 80 | 6 | `69e6cca4da2fb002c0d3315cc6d372d8a9d0370399ae2895bebe1501defd05c4` |
| `_game_state_reverted` | 32 | 2 | `254f436194c6a97556cf901ee959502670f8d75399cd0c3e7884a99a7e888260` |
| `_code_001af4f0` | 352 | 29 | `350352f1b90205366b540a4de2ceed508368e45f565b1501a1f1a734b903ac92` |
| `_code_001af650` | 112 | 11 | `8f67474089a0c650702bf7901d9fe8c32d7b4bcd95c99766c25776c451b68e3b` |
| `_game_state_set_revert_time` | 32 | 3 | `40c3b5882654cf42e4a4e217fffd19b863d77593bd7fe9d0a38a0d0b6e4bc4ef` |
| `_game_state_malloc` | 272 | 29 | `947befa023fcb18727260717911e26a200633a81ce43001ccd20e14fefde8c4d` |
| `_game_state_gpu_malloc` | 272 | 29 | `edb4f69ac452cdbc47ef929fef9ee43007045b9fb02fe4c7d9c3770ebdeba96e` |
| `_game_state_data_new` | 64 | 4 | `20067ba894d44b980a03bca3828a2bffc909d54654f2577317c244226e5749be` |
| `_game_state_memory_pool_new` | 64 | 4 | `a8e473abdaee557ad3ec971ed365acb21c85f9c6efaa12f9d29add07f531e133` |
| `_game_state_lruv_cache_new` | 80 | 4 | `7380045f6dc1c73cf2fa48566e81bb86b3eb6faee84035a09e84e9292b6ed7f7` |
| `_game_state_try_and_load_from_persistent_storage` | 240 | 21 | `d3673bad9698327d0460e4b837d8f75949c6701c8dc381e7a044407a256e4aae` |
| `_game_state_load_core` | 160 | 10 | `2424a9384c9f1edb3f17c0b3c72137e931068c7936bb749dc25e1e4af5eb30b6` |
| `_game_state_initialize` | 80 | 8 | `7d5d8884e49f398ef22ce7499b8b4252a62938d8e8a3c1a0d0f8353dbc4b08f4` |

The target-owned callback `.data` is 60/60 bytes with 15/15 exact
relocations and normalized hash
`5dcc1b5872dd9ff1c234501f1fefda01f664164e1583c3e1bb3dbea47588ab31`.
The aggregate `.bss` is 32/32 bytes with normalized hash
`66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925`.
All 18 target-owned string COMDATs also pass strict comparison.

Because `game_state.h` is included widely, the lane built both the untouched
`a8114eb7` tree and the candidate tree from scratch. A targeted strict census
then checked all 29 direct C translation-unit consumers: all 691 functions
that were exact before remain exact, and all 404 previously exact owned data
sections remain exact. The regression tool's raw manifest also reported
debug-record and section-index changes caused by reformatting declarations;
those are not code/data regressions, and the target-authoritative census above
was used to distinguish them rather than waive them.

The full build, ordinary progress report, semantic progress gate, and all 173
tool unit tests pass. The current source still contains the pre-existing early
exit in `game_state_revert`; it is not newly introduced by this reconstruction
and matches January exactly. Every newly reconstructed function retains one
source-level return and follows the parameter, `void`, accessor, and explicit
final-return house rules.
