# `rasterizer_cinematics.obj` screen-effect closeout (2026-08-31)

## Outcome

This closeout replaces the last four unwritten January owners in
`source/rasterizer/rasterizer_cinematics.c` with coherent typed C and preserves
all twelve previously exact siblings.  The compatibility gate, which maps the
two newly recovered source names back to the pre-regeneration split names,
reports:

```text
residual    112  _rasterizer_screen_effect_set_convolution  [size 96!=112, sha]
residual    112  _rasterizer_screen_effect_set_filter  [sha]
residual    288  _rasterizer_screen_effect_set_video  [sha]
residual     48  _rasterizer_screen_effects_initialize_for_new_map  [sha]
== exact 12  residual 4  unwritten 0  (of 16 listed)
```

All four residuals are ordinary-source reconstructions and are recorded in
`config/parked.json`.  They add 560 January padded bytes of semantically
implemented code to the board without claiming strict byte equality.

| Function | Target padded bytes / relocs | Candidate padded bytes / relocs | Objdiff |
| --- | ---: | ---: | ---: |
| `_rasterizer_screen_effects_initialize_for_new_map` | 48 / 3 | 48 / 3 | 59.933334% |
| `_rasterizer_screen_effect_set_convolution` | 112 / 4 | 96 / 4 | 95.15151% |
| `_rasterizer_screen_effect_set_filter` | 112 / 4 | 112 / 4 | 81.53846% |
| `_rasterizer_screen_effect_set_video` | 288 / 20 | 288 / 20 | 95.2381% |

## Evidence boundary

The reconstruction used the following independent evidence:

- January split-object code, relocations, strings, assertion line 225, and the
  `_error_silent` call path in
  `build/split/source/rasterizer/rasterizer_cinematics.obj`;
- the authenticated HCEA tree at commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, especially
  `src/blam/rasterizer/rasterizer_screen_effects_initialize_for_new_map.c`,
  `rasterizer_screen_effect_set_convolution.c`,
  `rasterizer_screen_effect_set_filter.c`,
  `rasterizer_screen_effect_set_video.c`, and
  `rasterizer_screen_effects_time.c`;
- the HCEA HaloScript definition/evaluator records, which independently prove
  `(short, short, real, real, real)` for convolution and `(short, real)` for
  video rather than the stale word-typed local declarations in `hs.c`;
- the January PDB module record for `rasterizer_cinematics.obj`, which confirms
  VC7 13.00.9254 but exposes no local-variable or line records for these
  bodies; and
- the open Marathon 2/Infinity tree at commit
  `ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`.  Its `marathon2/fades.c` and
  `fades.h` do not supply a direct donor, but corroborate the contemporary
  Bungie style of typed effect globals and explicit initialization.

The complete prior object ledgers were also read.  The 2026-08-30 census had
already screened 1,577 source copies and 2,332 target-named objects, so this
wave did not repeat broad donor searches after confirming that no more advanced
compiled candidate existed.

## Reconstructed semantics and measured fixed points

### Map reset

The retained body clears the complete 0x78-byte state and restores all four
script values to `1.0f`.  Target and candidate have the same padded size,
relocation identities, and stores.  January schedules `add esp, 0xC` after the
first three stores; VC7 schedules it before all four.  Separate assignments,
chained assignments, and refreshed-pointer forms compile to the same retained
schedule.

### Convolution

The retained body disables video, stores the two signed-short controls and the
radius endpoints, and constructs the interval from game ticks in the canonical
`real` type.  The streams are identical through the time calculation.  January
uses `fld st(0)` followed by `fstp` for the first endpoint; VC7 uses `fst` and
keeps the value live for the duration addition.  Nested/chained assignments,
in-place mutation, and a call through the recovered time helper all preserve
VC7's 96-byte result.

### Filter

The retained body disables video, records both interpolation ranges, computes
the time interval, stores the additive flag, and clears both mask flags.
Target and candidate have the same padded envelope and relocation identities.
Only independent endpoint/video-clear stores, the trailing byte flags, and the
x87 first-endpoint store are scheduled differently.

### Video

The retained body contains the full null-global assertion, both missing-bitmap
checks, complete parameter/interpolation reset, typed `bitmap_group_get` and
`TAG_BLOCK_GET_ELEMENT` lookups, and the attested silent error.  Typed bitmap
pointer locals recover the exact 288-byte/20-relocation envelope from the older
272/16 and 368/33 candidates.  Target and candidate have 84 aligned
instructions; the only differences are two independent `add esp, 0xC`
placements after `csmemset` and the first bitmap lookup.

## Names, declarations, and ownership

- HCEA supplies the private helper name `rasterizer_screen_effects_time`; the
  recovered correspondence marks it private, so the definition is now
  `static real rasterizer_screen_effects_time(void)`.  `config/symbols.json`
  names file offset 1495360 `_rasterizer_screen_effects_time`.
- The address placeholder `_bss_004662f4` is removed.  The owning TU now
  defines
  `struct rasterizer_cinematic_screen_effect_state *cinematic_screen_effect_globals = NULL`,
  and file offset 4612852 is named `_cinematic_screen_effect_globals`.
- All public functions are declared in
  `source/rasterizer/rasterizer_cinematics.h`, and the owning C includes that
  header so the definitions are type-checked.
- `global_rasterizer_data` is declared in its owning associated header,
  `source/rasterizer/common/rasterizer_common.h`, rather than locally in the
  cinematics TU.
- The candidate has exactly the same sixteen text owners as January and no
  helper COMDAT.  Defining the recovered cinematic global also restores the
  target's four-byte mutable-data owner.  Candidate-only scalar and assertion
  string COMDATs are ordinary duplicate emissions of target-external,
  same-named constants/strings; no candidate-only mutable data exists.

The two symbol renames deliberately made the checked-in pre-regeneration split
stale.  Canonical integration regenerated the split and completed the full
Ninja, semantic, strict-gate, and parked-manifest checks.  The final object
remains at 12 exact / 4 measured residual / 0 unwritten, with all sixteen
function bodies and their intended names accounted for.

Canonical integration now has `source/hs/hs.c` include the owning
`rasterizer_cinematics.h` header and removes its historical local declarations,
including the incorrect word-typed convolution/video arguments.  The focused
`hs.obj` regression gate remains at 445 exact / 3 pre-existing residual / 0
unwritten.

## Source policy

No assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
fake dependency, manual address/offset access, representation punning,
undefined behavior, dead branch, synthetic helper, byte forcing, object
patching, compiler-flag change, or comparator exception is retained.  The
source uses named structures, `real`, `boolean`, `TICKS_PER_SECOND`, bitmap/tag
macros, the correct error enum, and owning headers.  Reopen a parked body only
for authoritative January source or a natural same-compiler lifetime/schedule
donor described in its manifest entry.
