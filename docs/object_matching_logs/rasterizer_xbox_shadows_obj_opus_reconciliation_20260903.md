# `rasterizer_xbox_shadows.obj` Opus reconciliation (2026-09-03)

## Outcome

This packet reconstructs every previously unwritten text function in
`source/rasterizer/xbox/rasterizer_xbox_shadows.c`.  The focused gate moves
from 3 exact / 0 residual / 15 unwritten functions to 15 exact / 3 residual /
0 unwritten functions.  That is a strict gain of 12 exact functions and 2,304
padded text bytes, with no inherited exact function lost.  The remaining 3,104
target bytes are represented by complete, typed, high-similarity C and are
parked as instruction-scheduling residuals rather than claimed exact.

The Opus aggregate at `bb0d690a3` was used as a lead and bounded-probe record,
not as matching authority.  January's COFF, its relocation topology, the XDK
3911 headers, January strings/globals/call sites, and independently named
HaloCEA behavior were checked before retaining source.  The later aggregate
tip `a74ae992f` does not change this translation unit.

## Exact closures

The newly exact functions are:

- `_D3DDevice_SetRenderState` (432 bytes)
- `_D3DDevice_SetTextureStageState` (80 bytes)
- `_IDirect3DDevice8_SetRenderState@12` (544 bytes)
- `_IDirect3DDevice8_SetTextureStageState@16` (96 bytes)
- `_IDirect3DDevice8_SetVertexShaderConstant@16` (16 bytes)
- `_IDirect3DDevice8_SetVertexData2f@16` (32 bytes)
- `_IDirect3DDevice8_SetVertexData2s@16` (16 bytes)
- `_IDirect3DDevice8_Begin@8` (16 bytes)
- `_IDirect3DDevice8_End@4` (16 bytes)
- `__rasterizer_environment_shadow_model_begin` (160 bytes)
- `__rasterizer_environment_shadow_end` (128 bytes)
- `_rasterizer_shadow_convolve` (768 bytes)

The nine D3D symbols are the stock XDK inline bodies naturally emitted by
including `<xtl.h>`; they are not handwritten byte emission or synthetic
wrappers.  Their names, decorated forms, section contents, and relocations
agree with the XDK declarations and adjacent January translation units.
`_rasterizer_shadow_convolve` is a private function: its name is supported by
the corresponding HaloCEA file/call site and its uniquely exact 768-byte,
46-relocation January section.  `config/symbols.json` therefore records it as
static instead of retaining an address-derived label.

The three inherited 16-byte functions remain exact:
`__rasterizer_environment_shadows_begin`,
`__rasterizer_environment_shadow_model_end`, and
`__rasterizer_environment_shadows_end`.

## Parked functions

Each residual has complete behavior, identical padded size, and identical
relocation count/type/destination topology.  The normalized hashes and
reopening conditions are recorded in `config/parked.json`.

| Function | Target/candidate bytes | Relocations | Objdiff | Fixed point |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_environment_shadow_begin` | 944 / 944 | 73 / 73 | 98.32% | independent constant-row stores and commutative x87 dot-product scheduling |
| `__rasterizer_environment_shadow_model_draw` | 688 / 688 | 48 / 48 | 97.95% | two dependency-independent instruction transpositions |
| `__rasterizer_environment_shadow_draw` | 1,472 / 1,472 | 146 / 146 | 95.55% | equivalent x87 operand/term order in three commutative dot products |

The donor's bounded probes covered six ordinary-C sum spellings plus natural
declaration, initialization, constant-store, and include-order variants.  They
returned to these schedules or regressed.  The retained source contains no
volatile/register forcing, optimizer pragma, fake dependency, raw byte/offset
access, fabricated branch, forced inline, or assembly.  The functions are
parked until authoritative January source/local records or a natural
compatible-compiler donor explains the remaining backend choices.

## Ownership and ABI repairs

Address-derived globals were replaced with subsystem-style private names:
`shadow_restored` and `rasterizer_environment_shadows_globals`.  The typed
global state and the private convolution function are both `static`.

Prototypes now live with their owners rather than inside a consumer C file:

- the shadow implementation entrypoints, model-skinning setter, and static
  triangle draw call are declared by `rasterizer/xbox/rasterizer_xbox.h`;
- static-vertex statistics are declared by
  `rasterizer/rasterizer_frame_statistics.h`;
- the public shadow facade in `rasterizer/rasterizer.h` now matches January's
  callers and implementation, including the boolean begin result, shadow color
  and output-radius parameters, and typed triangle/vertex buffers.

The corresponding changes in `rasterizer.c` are source/API repairs only.  A
focused strict comparison preserves all 131/131 exact functions in
`rasterizer.obj`; its text symbol set and section bytes are unchanged.  Adding
the owner header also emits no unintended text section or COMDAT, including no
`point_from_line3d` COMDAT.

## Data-layout boundary

January's shadow BSS owner is 74 bytes.  The old incomplete source model was 72
bytes, while the complete naturally aligned typed structure is 76 bytes.  This
packet retains the honest 76-byte C representation: it does **not** claim exact
data/BSS ownership, and it does not add packing, a raw overlay, or an artificial
tail solely to manufacture a match.  That two-byte layout question remains
explicit reconstruction debt.

Narrow typed structural views remain local to this translation unit where the
canonical owner headers still expose the relevant renderer structures only
partially.  Moving those definitions into broadly included headers would be a
separate ownership migration with compiler definition-position risk; no such
unrelated header blast is hidden in this packet.

## Verification

- `python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_shadows`
  reports 15 exact / 3 residual / 0 unwritten.
- `python tools/campaign/gate.py source/rasterizer/rasterizer` reports 131/131
  exact.
- All direct and transitive consumers of the three changed headers were gated
  against the canonical report.  None lost an exact function.  The gate parser
  cannot address the spaced `saved games/game_state` key, so that unit was
  compiled directly and compared as 24 exact / 0 residual / 0 unwritten.
- `python tools/fake_match_scan.py
  source/rasterizer/xbox/rasterizer_xbox_shadows.c
  source/rasterizer/rasterizer.c --format text` reports zero review leads.
- Manual scans find no `point_from_line3d`, assembly, optimization barrier,
  optimizer pragma, raw byte emission, or address-derived C identifier in the
  retained shadows implementation.
- Both changed JSON files parse successfully, and `git diff --check` reports no
  whitespace error.

The isolated worktree cannot generate a full `objdiff` report without building
every base object.  The parked measurements were therefore rechecked directly
from the focused candidate/target COFF sections; canonical integration should
rerun the full report, parked validator, stable diff, and project test suite.
