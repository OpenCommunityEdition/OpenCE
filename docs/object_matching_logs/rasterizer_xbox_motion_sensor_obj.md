# `rasterizer_xbox_motion_sensor.obj` matching log

## Scope

- Source: `source/rasterizer/xbox/rasterizer_xbox_motion_sensor.c`
- January target: `build/split/source/rasterizer/xbox/rasterizer_xbox_motion_sensor.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Goal: strict whole-object equality under the hardened comparator.

## Acceptance gate

The object is not complete unless all of the following hold:

1. Every function has identical normalized bytes.
2. Relocation count, address, type, destination, and addend are identical.
3. Owned `.rdata`, `.data`, and `.bss` bytes, size, linkage, and ownership are identical.
4. A whole-TU regression sweep reports no previously exact sibling regression.
5. The consolidated build, semantic audit, parked-ledger validation, and tooling tests pass.

No inline assembly, volatile byte forcing, undefined behavior, byte patches, or compiler-flag changes are permitted.

## Baseline

The translation unit is an empty skeleton except for comments. The January object contains:

- ten XDK/D3D inline-wrapper instantiations;
- three Halo motion-sensor drawing functions;
- one four-byte floating-point constant;
- one one-byte BSS flag.

The current shape atlas proves that nine wrapper bodies have exact donors elsewhere in the tree.
Prior controlled compiler research also proves that ordinary calls through the stock `<xtl.h>`
definitions cause XDK 3911 CL to emit the required out-of-line wrappers. The wrappers must still
be regenerated and verified in this translation unit; shape similarity alone grants no credit.

## Strategy

1. Recover the three Halo functions from January disassembly and relocation evidence.
2. Use normal XDK calls and stock headers so the wrapper definitions are compiler-generated.
3. Rename the anonymous target wrapper symbols only after their API identities and decorated
   calling conventions are proven by the emitted candidate and relocation topology.
4. Rebuild and gate the entire translation unit after each coherent source-shape change.
5. Record every rejected source family below so later sessions do not repeat it.

## Experiments and findings

### E0 — untouched baseline

- Result: the base object contains none of the thirteen target functions.
- Classification: greenfield reconstruction, not a near-match or compiler tie.

### E1 — reconstruct the Halo functions and instantiate the XDK wrappers

- Ordinary calls through stock `<xtl.h>` emitted all ten D3D wrapper bodies.
- Every wrapper passed the hardened comparator, including exact sizes,
  relocation counts, addresses, types, resolved destinations, and normalized
  hashes.
- The target keeps unique `_code_*` names for these per-object copies while
  stock headers emit repeated SDK names. They are admitted through explicit
  target/base aliases in `config/semantic_matches.json`; renaming the image
  symbols themselves was rejected because duplicate global names make the
  address ledger ambiguous.
- The first Halo reconstruction exposed wrong texture-address and cull-state
  values in `begin`, a local-stack topology mismatch in `draw`, and structural
  differences in `end`.

### E2 — correct Xbox state encodings and the draw local topology

- The first two passes use `D3DTADDRESS_BORDER`; the post-composite pass uses
  `D3DTADDRESS_CLAMP`.
- The January code passes the raw Xbox cull-mode encoding `0x901`. Replacing it
  with the superficially equivalent desktop-style enum changes the generated
  state value.
- Modeling the draw position as one `real_point2d` aggregate, rather than two
  unrelated scalar locals, reproduced the original stack-slot allocation.
- Result: `begin` became exact at 640 bytes and 45 relocations; `draw` became
  exact at 384 bytes and 26 relocations.

### E3 — recover the end-pass dataflow and control flow

- Corrected the sweep texture-coordinate order to
  `(high, low)`, `(low, low)`, `(low, high)`, `(high, high)`.
- Preserved the target's two source returns: one after the successful composite
  and one after the conditional render-target restore. This is an
  evidence-backed exception to the single-return preference.
- Used 16-bit width and height intermediates, matching the target's partial
  register operations and signed conversions.
- A temporary `_ReadWriteBarrier()` between pixel-shader stores was rejected:
  it prevented beneficial argument hoisting and widened the difference.
- A `real_point2d` aggregate for the sweep bounds was rejected: it changed the
  prologue's local allocation and made the first divergence earlier.
- Result: 1,920-byte size and 142-relocation topology matched, with only the
  render-target load two bytes short.

### E4 — recover the narrow render-target parameter

- The target sequence `xor reg, reg; mov word ptr [global], reg16` proves a
  zero-extended 16-bit value at the call boundary.
- Correcting `rasterizer_set_target`'s first parameter from `long` to `word`
  emitted that sequence and made the rest of the routine fall into place
  without further changes.
- Result: `end` is exact at 1,920 bytes and 142 relocations.

## Final strict measurements

| Group | Functions | Padded code bytes | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| Stock XDK/D3D wrappers | 10/10 | 1,296/1,296 | 80/80 | Exact |
| Motion-sensor begin | 1/1 | 640/640 | 45/45 | Exact |
| Motion-sensor draw | 1/1 | 384/384 | 26/26 | Exact |
| Motion-sensor end | 1/1 | 1,920/1,920 | 142/142 | Exact |

Owned non-code sections also pass strict comparison:

- assertion path `.rdata`: 63/63 bytes, zero relocations;
- `__real@bd000000`: 4/4 bytes, zero relocations;
- `_bss_00465e27`: 1/1 byte, zero relocations and identical external ownership.

The additional XDK constant tables and pooled constants in the rebuilt COFF are
select-any COMDAT instantiations, not unique object-owned data. Their consuming
relocations resolve to the same destinations as the target's external forms.

## Plain-English behavior

The object prepares the Xbox GPU for the motion sensor, draws each radar blip
as a small colored textured square, draws the rotating sweep and its mask, and
then composites the finished sensor into the active player's screen area. Most
of the apparent complexity came from Xbox D3D state wrappers and exact
compiler scheduling, not from complicated gameplay logic.

## Do not repeat

- Do not synthesize wrapper machine code.
- Do not use `__declspec(naked)` or inline assembly for D3D thunks.
- Do not accept shape-atlas hashes without destination-aware relocation comparison.
- Do not mark the object `Matching` while any Halo function or owned data section remains unresolved.

## Current disposition

Strict-complete and admitted:

- 13/13 functions and all 4,143 target code bytes are exact;
- all 69 uniquely owned data bytes are exact;
- the consolidated `halobetacache_build` completed successfully;
- the semantic audit accepted all 13 functions and reported zero unit errors;
- project progress now records this object as complete;
- all 168 tooling tests pass;
- the shared rasterizer enum cleanup left `rasterizer.obj` at 131/131 exact
  functions, 3,294/3,294 code bytes, and 612/612 data bytes.

The pre-edit regression manifest had no exact functions or owned data to
protect because this was a greenfield skeleton. The gate therefore reported
the new functions, symbols, and data as intentional additions rather than
silently treating them as regressions. Their admission comes from strict
target comparison and the whole-project semantic audit. A post-admission
manifest freezes this completed state for future regression checks.
