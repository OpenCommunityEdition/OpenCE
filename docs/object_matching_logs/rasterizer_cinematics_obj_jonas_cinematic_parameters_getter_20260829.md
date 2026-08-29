# `rasterizer_cinematics.obj` cinematic-parameters getter wave

Date: 2026-08-29

## Outcome

`source/rasterizer/rasterizer_cinematics.obj` advances from 11/16 to 12/16
strict-exact functions. This wave contributes one function and 592 padded code
bytes:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_screen_effect_get_cinematic_parameters` | 592 | 39 | `afedfaac712b8db171012d119f7f488ec3bf7cd405cb0f3ad5ca9d36918767b4` |

The object remains `NonMatching`:

```text
UNWRITTEN   112  _rasterizer_screen_effect_set_convolution
UNWRITTEN   112  _rasterizer_screen_effect_set_filter
UNWRITTEN   288  _rasterizer_screen_effect_set_video
UNWRITTEN    48  _rasterizer_screen_effects_initialize_for_new_map
== exact 12  residual 0  unwritten 4  (of 16 listed)
```

No object-completion label, parked-function entry, semantic exception,
configuration rule, comparator behavior, or compiler flag was changed.

## Recovery evidence

The typed parameter and state layouts were recovered from the authenticated
HCEA material in the local research cache:

- `src/headers/rasterizer_screen_effect_parameters.h`;
- `src/headers/cinematic_screen_effect_globals.h`;
- `src/blam/rasterizer/rasterizer_screen_effect_get_cinematic_parameters.c`;
- the convolution, filter, and video setter sources beside that getter.

The HCEA source supplied semantic field names and ranges; January COFF bytes,
relocations, control flow, and constants remained the acceptance authority.
The translation unit now models the complete typed `0x38` parameter block and
the live interpolation fields in the `0x78` state, with compile-time size and
offset assertions.

The January getter reads two independently timed ramps. Its `PIN` expansion
evaluates the unclamped expression three times: VC7 inlines the first time read
to `game_time_get` and calls the existing `code_0016d140` helper for the next
two. The readable macro expressions reproduce that sequence for both ramps.
The three typed interpolation calls, black-to-green default tint, epsilon
shutdowns, multiple-window fatal path, and final state return then reproduce
all 195 target instructions.

One ordinary result local preserves the input pointer used by the no-control
return. In the controlled path the parameter variable becomes the current
typed global-state pointer. This natural lifetime separation lets VC7 reuse the
dead parameter home for the filter fraction while retaining one four-byte
local for the convolution fraction, exactly matching the target frame without
an alias, cast, storage directive, or representation trick.

The target object's `.rdata` identified the original fatal diagnostic as:

```text
### FATAL_ERROR screen effects can't use convolution when main_get_window_count>1
maybe you forgot to turn off the cinematic screen effect?
```

Restoring that complete message closed the final relocation-identity mismatch.
A census of 913 distinct valid local `rasterizer_cinematics.obj` builds found no
pre-existing exact getter or stronger admissible object to harvest.

The prior Jonas records for the leaf wave, setter ABI repair, public-setter
pivot, near-clip getter, start snapshot, and time/allocator pair were reviewed
before retention. The later HCEA material and all local compiled-tree evidence
were used read-only.

## Source policy

All retained code is readable typed C. It introduces no assembly, `volatile`,
`register`, pragma, barrier, intrinsic, attribute, raw address, raw offset,
representation punning, incompatible pointer cast, undefined behavior, byte
forcing, object patching, one-use function-pointer alias, compiler-flag change,
or comparison exception. The declaration uses one parameter per line and the
function has explicit returns on both paths.

The two close but non-exact setter experiments were removed before retention;
the final object has no residual functions.

## Validation

- Forced translation-unit build: pass.
- `python tools/campaign/gate.py source/rasterizer/rasterizer_cinematics --all`:
  12/16 strict exact, zero residual, four unwritten.
- Full `halobetacache_build`, `semantic_progress`, and `progress`: pass.
- Semantic audit: 470 units, 4,850 functions evaluated, 4,733 accepted exact,
  and zero unit errors.
- Tooling suite: 205/205 tests pass.
- Strict campaign board: 276/619 complete objects,
  652,071/1,922,669 padded code bytes, and 4,705/8,246 functions. Parked and
  assembly-implemented functions are not counted.
- `git diff --check`: pass.

Reproducibility identities before commit:

- retained source SHA-256:
  `5AE1DD8866A43F5EDEDE02B02A02FCA04EC029F7FD67E10FFE1F205D0141976A`;
- rebuilt object SHA-256:
  `BBDCF6E020826E519D449F70E230DFE693664B2382A64A3F0C79AC02EEA31B2B`;
- January target object SHA-256:
  `257B38976236EB2FB5913F4BDA88186C6B4DE1D498C5677EDDED99B5B7E6400`.
