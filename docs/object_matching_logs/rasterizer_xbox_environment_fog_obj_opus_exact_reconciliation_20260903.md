# `rasterizer_xbox_environment_fog.obj`: Opus exact-only reconciliation (2026-09-03)

## Outcome

This packet reconciles the truthful exact subset of the unfinished Opus donor
at `C:\halo-worktrees\opus-small-families-30k-20260902` onto canonical commit
`e0182f0973acff27168338c6ff87e4806459a1c7`. It admits seven newly exact
functions and 2,736 padded code bytes without losing any inherited exact
function:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_D3DDevice_SetRenderState` | 432 | 32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` |
| `_D3DDevice_SetTextureStageState` | 80 | 5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` |
| `_IDirect3DDevice8_SetRenderState@12` | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `_IDirect3DDevice8_SetTextureStageState@16` | 96 | 5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` |
| `__rasterizer_environment_fog_begin` | 1,168 | 111 | `06cf8198f76556bf91eb987b409a7ccd14495f2b68c2ddfa5cc6d9ce8703c81f` |
| `__rasterizer_environment_fog_draw` | 272 | 27 | `9bfcee7e2e2c55d0dcbc2fc58e40021506052eeef75b1b4f104f6f40a5e3e498` |
| `__rasterizer_environment_fog_screen_wind_get_vector` | 144 | 9 | `1585f3af7f9f1c4e2e1ec19c7705dcea5de956d573968678351ee7b2e36c4a9e` |

The object remains incomplete. No completion label, data credit, semantic
exception, comparator exception, or source-level byte steering is introduced.

## Reconciliation boundary

The gate was measured against freshly regenerated split targets so that the
four authenticated symbol aliases were compared under their canonical names:

| Tree state | Exact | Residual | Unwritten | Exact padded bytes |
|---|---:|---:|---:|---:|
| Canonical base `e0182f097` | 8 | 0 | 19 | 240 |
| Opus donor as found | 15 | 5 | 7 | not admitted as a unit |
| Retained exact-only packet | 15 | 0 | 12 | 2,976 |

The eight inherited exact functions remain exact: the fog end leaf, local
random boolean, screen initialization/disposal, model end, window begin/end,
and the vector4d setter. Their provenance and layout evidence remain in the
six earlier ledgers for this object.

Five donor bodies were deliberately excluded:

- `__rasterizer_environment_fog_screen_begin` had a 3,968-byte candidate for
  a 3,904-byte target and 240 candidate relocations for 235 target
  relocations;
- `__rasterizer_environment_fog_screen_draw` and
  `__rasterizer_environment_fog_screen_end` failed relocation identity;
- `_rasterizer_environment_fog_screen_model_begin` failed relocation
  identity;
- `_rasterizer_environment_fog_screen_model_submit` failed normalized bytes.

The donor-only `rasterizer_environment_fog_screen_active` helper is also
excluded. January has no independently owned function or symbol for it, and
the donor relied on compiler inlining rather than authenticated January
ownership. The excluded large bodies also contained provisional types and
representation-punning constructs that are not suitable for canonical source.

## XDK wrapper authentication

The four former `code + address` entries are ordinary out-of-line Xbox D3D
header COMDATs, not guessed Halo private functions. The stock header at
`xbox/include/D3D8.h` supplies the bodies and their exact public names:

- `D3DDevice_SetRenderState` begins at line 1517;
- `D3DDevice_SetTextureStageState` begins at line 1651;
- `IDirect3DDevice8_SetRenderState` is declared at line 1728;
- `IDirect3DDevice8_SetTextureStageState` is declared at line 1744.

Including `<xtl.h>` on the normal translation-unit path emits those same four
COMDATs. Direct hardened comparison reports `all_equal: true` for each one,
including padded bytes and relocation identity. The relocation sequences also
authenticate the bodies rather than merely their sizes:

- the generic render-state body references the simple/deferred state paths and
  the specialized `SetRenderState` handlers in stock-header switch order;
- the generic texture-stage body references Deferred, TexCoordIndex,
  BorderColor, ColorKeyColor, and BumpEnv handling in stock-header order;
- the two `IDirect3DDevice8` wrappers reproduce those bodies with the expected
  `this` argument and stdcall decorations.

Their target COMDAT ordering agrees with the stock header's declaration and
use topology. `config/symbols.json` therefore replaces the four address-form
labels with these authenticated XDK names.

## Halo source and type evidence

The three admitted Halo functions are ordinary C reconstructions. January's
instructions, relocations, assertions, and string ownership establish their
control flow and types. Relevant cross-build HCEA sources corroborate the fog
begin and draw semantics: signed plane distance, pinned atmospheric and planar
density, the atmosphere-dominant flag, the two density textures, render-state
setup, pixel-shader constants, vertex-shader permutation selection, draw call,
and environment-fog statistics updates.

HCEA type evidence additionally authenticates:

- the fog debug-option fields at offsets 2, 4, 0x1C, and 0x1D;
- the January fog block at window-parameter offset 0x1E8;
- `_fog_definition_atmosphere_dominant_bit` as bit index 1;
- the `real_alpha_intensity_to_pixel32(real, real)` signature.

The latter prototype now lives in its owning `interface/hud_draw.h` rather
than as an ad hoc declaration in the consumer. The window, fog, statistics,
pixel-shader, and screen-wind views are kept translation-unit-local because
the canonical shared headers do not yet own equivalent complete definitions.
Every accessed offset and required aggregate size has an explicit compile-time
check or direct January relocation proof.

The archived Claude assertion synopsis independently confirms the target
function sizes and the wind function's `wind_vector` and bounded
`window_index` assertions, as well as the fog-begin distance assertions and
warning string. The PDB symbol inventory confirms the named Halo functions
and globals but contains no local-variable or line record that would justify
richer private names. Marathon and the searched Claude worktrees provide no
competing January implementation.

## BSS owner naming and boundary

The prior source defined the real fog-screen owner under an address-derived C
identifier and hid it behind a macro alias. This packet removes that workaround:
the actual production identifier and the csplit symbol-map entry are now
`rasterizer_environment_fog_screen_globals`. The variable is file-private.
Every January reference to the 661-byte range belongs to this translation
unit, the PDB has no exported/public name for the owner, no other source module
declares or references it, and the adjacent Xbox environment and profile
global aggregates follow the same `static ..._globals` ownership convention.
The symbol-map entry is therefore marked `static: true`, making csplit retain
the same file-private COFF storage class as the source. A scan of every
regenerated split object finds this symbol only in
`rasterizer_xbox_environment_fog.obj`; both the split target and rebuilt
candidate encode it with COFF storage class 3 (`STATIC`).

January relocation identity proves that this is the real storage owner rather
than an invented replacement. The wind function targets owner addend 332,
which is `windows` at 0x120 plus the selected 0x4C-byte window record and its
wind field at 0x2C. Fog begin targets the warning counter at owner addend
0x280. Earlier object ledgers independently prove the same owner's fields at
592, 596, and 644. The admitted code accesses named typed fields only; it does
not use a raw address, raw offset, macro alias, or pointer reconstruction.

The modeled prefix remains 648 bytes although January's BSS contribution is
661 bytes. The unknown 13-byte tail is not guessed, fake-packed, or awarded
data-matching credit.

## Verification

A full 572-target baseline build and a full 572-target candidate build were
captured in separate worktrees, each against a split regenerated from its own
symbol configuration. Stable-verdict comparison evaluated 8,245 functions:

- baseline: 5,681 exact;
- candidate: 5,688 exact;
- gained: exactly the seven functions above, totaling 2,736 padded bytes;
- lost or regressed: zero.

The owner-header prototype include blast covered all 13 unrelated direct
`hud_draw.h` consumers. Every object retained identical exact, residual, and
unwritten counts. The final object-local gate reports 15 exact, zero residual,
and 12 unwritten functions.

Additional gates:

- full Ninja candidate build: pass, 572 targets;
- hardened direct comparison of all 15 exact functions: `all_equal: true`;
- tooling tests: 261/261 pass;
- semantic report: 473 units, 6,019 functions, zero unit errors, 5,714
  accepted-exact functions;
- parked validation: 176 active, zero stale, zero invalid;
- object-admission audit: zero contradictions or revocations introduced;
- fake-match scan over the changed source/header: zero review leads;
- `git diff --check`: pass.

## House-rule audit

All admitted Halo bodies use canonical `real`, vector, color, and integer
types, one parameter per line, `void` on its own line for no-argument
functions, and an explicit final `return;`. Flags use `TEST_FLAG`; D3D state
uses named XDK constants; the fog flag uses its authenticated enum constant.
The code does not add inline assembly, forced inlining, `register`, `volatile`,
pragmas, intrinsics, raw addresses, manual pointer arithmetic, type punning,
synthetic anchors, undefined behavior, or nonsensical byte-matching forms.
