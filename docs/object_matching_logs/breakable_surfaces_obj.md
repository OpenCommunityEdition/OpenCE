# `breakable_surfaces.obj` exact-match strategy log

## Scope and provenance

- Source TU: `source/physics/breakable_surfaces.c`
- Target object: `source/physics/breakable_surfaces.obj`
- Target build: Halo January 2002 Xbox build
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254.1
- Flags: `/O2 /Oy- /Ow /QIfist /DDEBUG /Dxbox`
- Comparator: hardened COFF function and section comparison
- Status: rigorously fuzzy-parked at the current natural-source frontier; no
  strict matching credit has been granted

The rough October source at
`C:\Users\isabe\Downloads\halo-main (1)\halo-main\src\halo\physics\breakable_surfaces.c`
is a topology hint only. January machine code, relocations, and data ownership
remain authoritative.

## Current validated state

- Functions: `11/12` strict exact.
- Only residual: `_breakable_surface_effect`.
- Target/current padded size: `0xFC0/0xFC0` (`4032/4032`).
- Target/current relocations: `117/117`, with the same semantic destinations.
- Target/current frame: `0x1240/0x1240`.
- Target normalized SHA-256:
  `510486a7d0a1f2fc1dfb8f0c47990c57f83ce1030ef5e4db8dfc42943dc56bfa`
- Current normalized SHA-256:
  `678a8125d160643a9f1226240128c662bcbb85e7f968f8fd3ed0f0b411f77797`
- All 11 protected sibling functions remain strict exact.

The current source is the 2026-08-31 Claude reconstruction reconciled against
the current shared headers and September house rules. It supersedes the E09
traversal/audio scratch union checkpoint described below. The older experiment
ledger remains here because it is still useful negative evidence.

## Historical checkpoint controls (superseded)

E09 and E18 were legitimate intermediate evidence, but they are no longer the
production source shape. The final reconciliation recovered the later natural
block lifetimes and the same `rectangle2d` field meanings without retaining the
scratch union.

### E09: traversal/audio scratch union

January reuses one `0x58`-byte region for traversal scratch and the later
`sound_location`. The retained C89 union models the measured layout:

- traversal `s_offset`, `t_offset`, two projected points, and eight 2D
  vertices;
- audio view with `sound_location` at union offset `0x2C`;
- both views end at the same byte.

This is defined lifetime-based storage reuse, not aliasing or byte padding.

### E18: `rectangle2d` grid bounds

Replacing four unrelated short locals with `rectangle2d grid_bounds` moved
the candidate from `0xF90` to the exact target size `0xFC0` while preserving
`117/117` relocations and every exact sibling.

The native structure layout is exactly the order proved by January:

| Structure field | January meaning | January home |
|---|---|---:|
| `grid_bounds.y0` | `t_min` | `-0x5C` |
| `grid_bounds.x0` | `s_min` | `-0x5A` |
| `grid_bounds.y1` | `t_max` | `-0x58` |
| `grid_bounds.x1` | `s_max` | `-0x56` |

This is an evidence-backed type recovery and remains in production even
though the function is not yet exact.

## Current stack/codegen finding

E18 makes every large aggregate start exactly 16 bytes above the January
home:

| Region | Target | E18 |
|---|---:|---:|
| `surface_queue[1024]` | `-0x1240` | `-0x1230` |
| `surface_vertices3d[8]` | `-0x240` | `-0x230` |
| traversal/audio scratch | `-0x1E0` | `-0x1D0` |
| `new_particle_data` | `-0x188` | `-0x178` |

The missing 16 bytes are four January dword integer-conversion homes. For
each grid bound January emits:

```asm
call  _ceil/_floor
fstp  dword ptr [ebp-real_home]
add   esp, 8
fld   dword ptr [ebp-real_home]
fistp dword ptr [ebp-distinct_integer_home]
mov   r16, word ptr [ebp-distinct_integer_home]
mov   word ptr [ebp-grid_bound], r16
```

E18 instead emits `fistp qword` to one shared compiler temporary. The target
therefore rounds through single precision and owns four distinct dword
conversion homes; the present compiler forwards through every normal
readable-C spelling tested.

## Experiment matrix

Every reverted experiment preserved all 11 exact siblings unless explicitly
noted.

| ID | Source or compiler experiment | Result | Decision |
|---|---|---|---|
| E01 | Move `position` and `position_2d_test` inward | byte-neutral | reverted |
| E02 | Move both vertex arrays into the `while` scope | byte-neutral | reverted |
| E03 | Declare the 2D vertex array later | byte-neutral | reverted |
| E04 | Remove the duplicate queue-read shadow | byte-neutral | reverted |
| E05 | Hoist random offsets to function scope | byte-neutral | reverted |
| E06 | One named `real` between ceil/floor and cast | byte-neutral; store forwarded | reverted |
| E07 | Group grid bounds and loop indices | byte-neutral | reverted |
| E08 | True sibling traversal/audio scopes | frame `0x1228`; proved lifetime reuse | superseded by E09 |
| E09 | Meaningful `0x58` C89 scratch union | size `0xF90`, frame `0x122C` | retained |
| E10 | Remove explicit `long` from the grid casts | byte-neutral | reverted |
| E11 | Static grid-index helper | caller byte-neutral; unwanted helper COMDAT | reverted |
| E12 | Named `real` plus four named long homes | optimizer eliminated homes | reverted |
| E13 | Aggregate containing rounding/conversion fields | optimizer eliminated homes | reverted |
| E14 | Address escape / compiler-barrier shape | did not produce target chain | reverted |
| E15 | Conversion-home array plus barrier | did not produce target chain | reverted |
| E16 | Pointer helper plus barrier | did not produce target chain | reverted |
| E17 | `__forceinline` float ceil/floor wrappers | caller byte-neutral; two unwanted COMDATs | reverted |
| E18 | Native `rectangle2d grid_bounds` | exact `0xFC0`, `117/117`; correct bound layout | retained |
| E19 | `real_rectangle2d` staging plus E18 | byte-identical to E18 | reverted |
| E20 | Four named long homes plus E18 | byte-identical to E18 | reverted |
| E21 | Historical function-scope bounds plus `long` loop indices | regressed to `0xF70`; hash `f32f3d8f...` | reverted |
| E22 | Move only `rectangle2d grid_bounds` to function scope | byte-identical to E18 | reverted |
| E23 | Change only the two grid loop indices to `long` | same `0xF70` regression as E21 | reverted |
| A01 | Compile E18 as C++ (`/TP`) | function byte-neutral; decorated symbols | analysis only |
| A02 | `/TP` with explicit `ceilf`/`floorf` | unwanted helpers; caller byte-neutral | analysis only |
| A03 | Remove `/Ow` | byte-neutral | analysis only |
| A04 | Enable `/Op` | `4320` bytes, `114` relocs; broad regression | rejected |
| A05 | Minimal direct/named/four-value float-to-short fixtures | every form collapsed to shared `fistp qword`; no target motif | analysis only |
| A06 | Four legal union-backed long/short conversion objects | also collapsed to shared `fistp qword` | analysis only |

## Corpus and compiler audits

- A relocation-aware scan of all 833 January split objects found the complete
  float-home conversion motif only eight times: four here and four in
  nonexact `_build_structure_lens_flares`.
- A scan of 467 rebuilt base objects found zero occurrences.
- `_build_structure_lens_flares` is not a reconstructed-C donor: target/base
  is `4336/4000` bytes and `156/157` relocations.
- All locally available compatible compiler sets are byte-identical
  VC7 13.00.9254.1 binaries. No alternate compatible compiler/QFE is locally
  available.
- Upstream history commit
  `95e6477328dd7742175074de5eab0a20c3e7bde0` provided a real source-shape
  lead: function-scope bounds and `long` grid indices. E21-E23 showed that
  bound scope is neutral and the historical `long` index type causes the
  same `0xF70` regression, so it is not the January topology.
- The reproducible `round_probe.c` fixture proves that direct, named-real,
  named-long, four-output, and four-longs-first forms all lower through one
  shared `fistp qword` home under the campaign compiler.

Reproducible evidence is under
`research/breakable_surfaces_closeout/`, especially
`STACK_SLOT_LIFETIME_MAP.md` and `donor_census/REPORT.md`.

## Do not repeat

- E01-E17 and E19-E23, including cosmetic variations with equivalent
  lifetimes.
- C++ mode, `ceilf` wrappers, `/Ow` removal, or `/Op`.
- Adding guessed padding merely to force the missing 16 frame bytes.
- Treating `_build_structure_lens_flares` as an exact donor.
- Inline assembly, `volatile` byte forcing, undefined behavior, object-byte
  patches, or compiler-flag changes.

## 2026-07-28 provenance audit: January PDB, HCEX PDB, and local source corpus

A second provenance pass found a strong source-semantics lead but no new
legal-C code-generation lever.

### January PDB negative result

`C:\Users\isabe\Downloads\Halo Combat Evolved (Jan 14, 2002 prototype)\cachebeta.pdb`
names `\halo\objects\halobetacache\breakable_surfaces.obj` as module stream
154. The complete module stream is only 132 bytes: one `S_OBJNAME` record,
one compiler record (`Microsoft (R) Optimizing Compiler`), and four zero tail
bytes. It contains no function, local, type, or line records, so it cannot
recover the grid-bound declarations, any rounded `real`, or the four
conversion homes. The raw compiler record must not be treated as proof of an
unrecorded precision flag.

### HCEX source-line and code-generation evidence

The later debug and release symbol files are:

- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb`
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX_Release.pdb`

They identify `breakable_surface_effect` as source
`d:\projects\code\hcex\sources\physics\breakable_surfaces.c`, source MD5
`CEACA3EE6F74879855CD2005D8B6C1AC`. Debug RVA/length/frame are
`0x00D0C1A0/0xE68/0x1360`; release RVA is `0x01743190` with the same
length/frame. Neither PDB contains injected source.

DIA line records map the four bounds to four consecutive source statements:
lines 423-426 are the two `ceil` assignments followed by the two `floor`
assignments; line 421 is the nonzero-density test and line 440 begins the
grid loop. There are no intervening source lines for a separate named-real
assignment/conversion pair. The surviving named-local records also contain
no rounded-real/grid aggregate or scalar grid-bound names, although
optimized-away locals cannot be ruled out from that absence alone.

PowerPC disassembly of the decoded debug image at flat RVA `0xD0C924` is
stronger semantic evidence. Each libm result is immediately rounded from
double to float with `frsp` (first at `0xD0C954`), then converted with
`fctiwz`/`stfd` and sign-extended to a short; the four results remain scalar
GPR values. This is consistent with four direct
`(short)(real)ceil/floor(...)` assignments and inconsistent with a required
separate named `real` statement. PowerPC cannot distinguish the optional
intermediate `(long)` because both `int` and `long` are 32-bit there.

### Source-corpus and macro negatives

A hash census of 747 local `breakable_surfaces.c` files found 11 unique
contents and no copy matching HCEX MD5
`CEACA3EE6F74879855CD2005D8B6C1AC`. Repository history contains
reconstruction commits only, not an authentic Bungie source blob. The
October file at
`C:\Users\isabe\Downloads\halo-main (1)\halo-main\src\halo\physics\breakable_surfaces.c`
is a decompiler reconstruction: it defines replacement `breakable_floor` and
`breakable_ceil` functions and has grid comments/order inconsistent with the
machine-code topology, so its named `float val` is not provenance.

The active type and macro audit found `typedef float real` in
`source/math/real_math.h` and ordinary ternary `PIN` in
`source/cseries/cseries.h`; no real-to-short rounding macro exists in the
recovered source. The closest source sibling,
`source/structures/structure_lens_flares.c`, spells all four bounds directly
as `(short)(real)ceil/floor(PIN(...))`, and its January target has the same
float-home conversion motif, but that function is itself nonexact and cannot
serve as a strict donor.

### Verdict and do-not-repeat expansion

The highest-likelihood original spelling is four direct scalar assignments
with an explicit `(real)` cast, probably without the reconstruction-only
`(long)`. That spelling, named-real forms, named-long homes,
aggregates/arrays, address escapes, helpers/wrappers, scope changes, C++
mode, and precision-flag probes have already been measured and do not make
the campaign VC7 emit January's chain. No genuinely untried legal-C
hypothesis now satisfies the reopen criterion. Do not repeat the
January-PDB private-record search, HCEX injected-source/source-hash search,
October-source audit, macro/type audit, or local source-hash census; reopen
only if an authentic source blob, a distinct compatible compiler/QFE, or a
strict donor artifact appears.

## Residual classification

Compiler-precision/provenance fixed point. Size, call set, relocation count,
recovered grid type, and large aggregate layout are strongly supported.
January has a repeated single-precision rounding and four-home conversion
topology that the current VC7 binary does not emit from any tested legal-C
shape. This is not strict exactness and receives no completion credit.

## Reopen/continuation criteria

Reopen only with evidence for one of:

1. an original October/January/HCEX source or debug topology that explains
   the intermediate `real` materialization;
2. a strict reconstructed-C donor for the complete conversion motif;
3. a genuinely new legal-C dataflow that predicts both the `fstp/fld`
   single-precision round trip and four distinct dword homes.

## Disposition

`NonMatching` / rigorously parked at 11/12 exact functions. E09 and E18 were
evidence-backed improvement checkpoints, not object completion. The newer
reconciliation below is authoritative. Do not mark the object complete or
grant credit from equal size or semantic plausibility.

## 2026-09-01 Claude-frontier and house-rule reconciliation

### Corpus audit

The closeout reviewed all 1,648 registered worktrees and reduced their
`breakable_surfaces.c` files to twelve distinct source variants. The unique
complete frontier was the history on `claude/breakable-surfaces-20260830`, in
particular commits `7fbe2a94d`, `a324c519b`, `39207d9ec`, `a5af3125d`,
`ba1109b49`, and `230dc0af0`. It was checked against the separate
`exact-breakable-surfaces-closeout` lane, the canonical/object log history,
`research/breakable_surfaces_closeout/`, the January target, the HCEX PDB and
PowerPC implementation, the HCEA donor, the October reconstruction, and the
local same-compiler donor census. No worktree contained a later credible
frontier.

The retained source recovers the complete surface traversal and projection,
particle-grid generation, outward/forward impulse shaping, particle tint and
alpha evaluation, and optional break sound. It also retains the evidence-backed
natural block scopes, `rectangle2d` grid bounds, `real_point3d` vertex lvalues,
semantic private helper names, grouped j/k plane-distance association, and the
target alpha multiplication order. The old duplicate TU-local conversion
helper was removed: all four conversions now use the authenticated shared
`fast_ftol` in `source/cseries/cseries.h`. That two-instruction x87 helper is
independently corroborated by the HCEX PDB and January's
`actor_combat.obj`, and is within the house rule allowing sparse assembly in a
math/helper primitive.

### Inline ownership

Calling the shared header `point_from_line3d` directly leaves a candidate-only
COMDAT even though January owns none in this object. The production source
therefore uses a transparent, coordinate-for-coordinate scalar expansion of
that header body. Its semantic local bindings model the inline function's
parameters, preserve the exact `0x1240` frame and `0xFC0` padded envelope, and
emit no `_point_from_line3d` symbol or COMDAT. Plain direct statements also
remove the COMDAT, but make VC7 collapse the function to `0xFB0`; that less
faithful schedule was rejected. No forced-inline/noinline attribute, pragma,
volatile access, barrier, assembly in the effect routine, or fake dependency
is retained.

### Final measurements and residual

- Strict gate: `11 exact / 1 residual / 0 unwritten`.
- `_breakable_surface_effect`: target/current padded size `4032/4032`.
- Frame: target/current `0x1240/0x1240`.
- Relocations: `117/117`; relocation types and symbolic destinations occur in
  the same sequence.
- External objdiff fuzzy similarity: `99.8042%`; the campaign report's
  validator metric is `91.437065%`.
- Target normalized SHA-256:
  `510486a7d0a1f2fc1dfb8f0c47990c57f83ce1030ef5e4db8dfc42943dc56bfa`.
- Candidate normalized SHA-256:
  `678a8125d160643a9f1226240128c662bcbb85e7f968f8fd3ed0f0b411f77797`.
- All eleven sibling functions and the object's owned data remain strict exact.

The aligned 1,156-target/1,155-candidate instruction diff reduces the genuine
code differences to two small x87 regions: commutative operand-role selection
in one `cross_product3d` expansion around target offsets `0x37B`-`0x392`, and
an equivalent `fxch`/load/pop stack schedule in the first scalar-expanded grid
offset around `0x97D`-`0x9B1`. More than seventy ordinary-C declaration, scope,
association, helper, operand-order, and compiler-shape probes either returned
to this fixed point or degraded broader code. Relocation-name differences shown
by the historical aligned-diff utility are annotation artifacts; the hardened
COFF comparison resolves their symbolic destinations.

### Final disposition

This is the most complete authentic, house-rule-compliant source currently
available, but it is not byte exact and earns no strict completion credit. It
is evidence-parked as `instruction-scheduling`. Reopen only for authoritative
January source/local records, an authenticated compatible compiler/QFE, or a
natural same-compiler donor that explains the remaining x87 operand and stack
choices.
