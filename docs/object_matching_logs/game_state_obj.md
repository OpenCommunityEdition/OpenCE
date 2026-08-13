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
