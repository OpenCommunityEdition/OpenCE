# `rasterizer_xbox_models.obj` model-begin recovery (2026-08-29)

## Scope and result

This continuation starts from pushed canonical commit
`391a1e19c6342e73077f05e98f34d760a6624a9c` and recovers
`__rasterizer_model_begin` in ordinary typed C:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_model_begin` | 448/448 | 45/45 | `927fbb492aafc335d58deeabbf8c95f52a4c9f6937255256b6e4383937dfe16b` |

The object advances from 4/14 to 5/14 strict-exact functions, adding 448
padded code bytes. The retained gate reports five exact, zero residual, and
nine unwritten functions. No object-completion, data, semantic-exception, or
parked credit is claimed.

## Evidence and typed reconstruction

January's complete 448-byte COMDAT and its 45 ordered relocations are the
authoritative oracle. The clean HCEA oracle at commit
`c168af2e747d3095d9a29418ae401f3a39544863` supplies a readable later-build
semantic donor in `src/_rasterizer_model_begin.c` (Git blob
`9e29b2ab996513281903f74dbcfbc59b450bf1f4`, SHA-256
`96cca95fd29a5cf3d6b324812c84e0a3458838caa71611f97807859821959cba`).
HCEA corroborates meaning and source-level topology only; January fixes the
retained body, fields, branches, constants, and source line.

The declaration-only views are expanded just far enough to name every field
the function uses. Compile-time `offsetof` assertions authenticate the model
parameters' skinning, lighting, and effect members; the private model globals'
parameters, effect type, and state flags; the window fog member; and the frame
statistics skinning counter. Access remains through typed fields. The private
BSS, window globals, and frame statistics remain undefined external owners;
there is no raw address, offset dereference, storage rehome, or data credit.

January and the donor establish the retained behavior:

- honor the model-drawing debug switch and assert the parameters pointer at
  January source line 587;
- configure first-person stencil and frustum state when the caller permits it;
- save the typed parameter pointer and per-model state;
- select active-camouflage or transparent-zbuffered effects, otherwise submit
  skinning and lighting and accumulate their frame-statistics work;
- derive the camera's signed distance to the planar-fog plane and set the
  typed planar-fog state from the documented geometry bits;
- begin environment-fog-screen handling for non-sky models; and
- increment the model counter when rasterizer statistics are enabled.

## Relocation-order correction

The initial natural typed compile already matched the target's 448-byte size,
all 45 relocation addresses and types, and the complete normalized instruction
hash. Its only residual was the identities of the six relocations in the
three-component camera/plane dot product: the source listed components in
`z, y, x` order, causing VC7 to emit `x, y, z`, while January requires emitted
`z, y, x`.

The final source uses the conventional donor-authenticated expression order
`x, y, z`. VC7's evaluation order then produces January's exact relocation
sequence without changing a single normalized instruction byte. The hardened
comparator consequently accepts all 45 relocation identities and the complete
function. This was a source-expression correction grounded by both the donor
and January, not a compiler-flag, scheduling, or byte-forcing experiment.

## Verification and policy

- January object SHA-256:
  `994b40517a081b740bc93bb4f0b60617fd53b00657bdd175e8666a34e9eb1f15`.
- Final rebuilt object SHA-256:
  `fca0cf15aadf819d274e1e2395980340d6bdee6c6c240616413d080120007ccb`.
- Retained source Git blob:
  `dfe5bbee5e480ec5268b14a25169911e814cf208`; working-tree SHA-256:
  `8193e687531ab28f9605bce7bbe2a2f06e9fb5e901f8859cb0d9a007856f11fe`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,794 functions,
  4,670 semantic exact, 4,680 accepted exact, and zero unit errors.
- Strict campaign board: 275/619 complete objects,
  637,863/1,922,669 padded code bytes, and 4,651/8,246 strict functions.
- Object admission: zero candidates, contradictions, and revocations.
- Parked ledger: 13 active, zero stale, and zero invalid.
- Naming-gap attestation: zero distinct candidates.
- Tool suite: 205 passed; the optional pytest-cache warning is sandbox-only.
- `git diff --check`: pass.

The retained body follows C89 declaration order, keeps each parameter on its
own line, and ends with an explicit return. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, optimizer barrier, attribute,
representation pun, undefined behavior, synthetic anchor, byte forcing,
compiler-flag change, object patch, or comparator exception.

Only `source/rasterizer/xbox/rasterizer_xbox_models.c` and this ledger are
changed. Protected Units, Vehicles, Matrix Math, AI, and game-engine sources
remain untouched.
