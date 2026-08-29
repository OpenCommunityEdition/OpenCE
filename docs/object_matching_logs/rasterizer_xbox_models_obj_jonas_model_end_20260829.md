# `rasterizer_xbox_models.obj` model-end recovery (2026-08-29)

## Scope and result

This continuation starts from pushed canonical commit
`9117b690d72d268fb9b6c5de5d7310d54995b253` and recovers
`__rasterizer_model_end` in ordinary typed C:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_model_end` | 128/128 | 13/13 | `c29113a2e85ca6f9e080032a0bc8fe64df329e16b9152526526b5a140ed28184` |

The object advances from 3/14 to 4/14 strict-exact functions, adding 128
padded code bytes. The retained gate reports four exact, zero residual, and
ten unwritten functions. No object-completion, data, semantic-exception, or
parked credit is claimed.

## Evidence and typed layout

January's 128-byte COMDAT and 13 ordered relocations are authoritative. The
clean HCEA oracle at commit
`c168af2e747d3095d9a29418ae401f3a39544863` supplies the complete typed inner
body in `src/_rasterizer_model_end.c`, Git blob
`a5444a177c461445fc3d27f63ed2b3ab02fc2025`, SHA-256
`d66bfd0dc7728e0a6df802fcef4261ac88787e0f6b965cd4503312510fbf9eda`.
The previous lifecycle wave independently proved the `draw_models` and saved
sky fields before this function was attempted.

This wave extends the declaration-only private prefix with:

- `local_parameters` at `+0xB0`;
- the already proven saved sky flag at `+0xBA`;
- `environment_fog_screen` at `+0xBC`;
- `do_not_change_z_stencil_states` at `+0xBD`.

Every field has a compile-time `offsetof` assertion. A minimal typed
`rasterizer_model_begin_parameters` view names `geometry_flags` at offset
zero. The BSS remains an undefined external owned elsewhere; no raw address,
offset dereference, storage rehome, or data credit is introduced.

January and HCEA agree on the body: when model drawing is enabled, assert the
parameters pointer; end the environment-fog-screen model when active; restore
the reject stencil mode and zero frustum for first-person geometry unless the
caller requested state preservation; then clear the parameters pointer.
January independently fixes the assertion source line at 1491, the
first-person bit at 7, and the reject stencil mode at 2.

## Assertion-name correction

The initial natural typed spelling produced code that already matched all
128 padded bytes and all 13 relocation addresses, types, and nonliteral
destinations. Its only strict mismatch was the compiler-generated assertion
expression string: `bss_00465d68.parameters` instead of January's explicit
`local_parameters` literal.

The target string is direct source-spelling evidence, so the retained source
adds a typed macro alias `local_parameters` and uses it consistently. The
next compile changed only that assertion literal identity; the complete code
hash and relocation topology remained identical and the hardened comparator
became fully strict. No control-flow, declaration-order, scheduling, flag, or
optimizer experiment was performed.

## Verification and policy

- January object SHA-256:
  `994b40517a081b740bc93bb4f0b60617fd53b00657bdd175e8666a34e9eb1f15`.
- Final rebuilt object SHA-256:
  `e5f0c092901ed1baa27f545ff002b94041758bd23db0435395459657060e8ecd`.
- Retained source Git blob:
  `cc432ea5b61d59b7fa9371aaf51c76a57119ef55`; working-tree SHA-256:
  `707a1daf1169d609957f0bc24d9d59930b24cfcf7913ca7cdc9c26116ba77136`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,793 functions,
  4,669 semantic exact, 4,679 accepted exact, and zero unit errors.
- Strict campaign board: 275/619 complete objects,
  637,415/1,922,669 padded code bytes, and 4,650/8,246 strict functions.
- Object admission: zero candidates, contradictions, and revocations.
- Parked ledger: 13 active, zero stale, and zero invalid.
- Tool suite: 205 passed; the optional pytest-cache warning is sandbox-only.
- `git diff --check`: pass.

The retained body follows C89 declaration order, uses `void` on its own line
in the no-argument list, and ends with an explicit return. It contains no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, representation pun, undefined behavior, synthetic anchor, byte
forcing, compiler-flag change, object patch, or comparator exception.

Only `source/rasterizer/xbox/rasterizer_xbox_models.c` and this ledger are
changed. Protected Units, Vehicles, Matrix Math, AI, and game-engine sources
remain untouched.
