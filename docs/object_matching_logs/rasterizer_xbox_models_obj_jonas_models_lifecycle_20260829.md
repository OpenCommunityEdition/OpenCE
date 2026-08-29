# `rasterizer_xbox_models.obj` models lifecycle recovery (2026-08-29)

## Scope and result

This bounded wave starts from pushed canonical commit
`8d4cb15d17c6e01b183f20e3d250b51a7e8cca8e` and recovers the inner models
begin/end pair in ordinary typed C. Both functions are strict exact on the
first and only candidate compile:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_models_begin` | 64/64 | 5/5 | `07b01cbf590115ce95600bfa78a698a26b4aaf963097567aa313834b67130706` |
| `__rasterizer_models_end` | 48/48 | 4/4 | `44fb2a9355a1188219606ef37ba4f2406eead7284927b1804fa869d3e1871998` |
| **Wave gain** | **112** | **9** | |

The object advances from 1/14 to 3/14 strict-exact functions, with zero
residuals and eleven unwritten functions. It remains incomplete and receives
no data or semantic-exception credit.

## Evidence and typed ownership

January's complete COMDATs and ordered relocation maps are authoritative.
The clean HCEA oracle at commit
`c168af2e747d3095d9a29418ae401f3a39544863` independently authenticates the
inner begin semantics and typed field names:

- `src/_rasterizer_models_begin.c`, Git blob
  `a7480672ff74d48ef3d3d49d95d48478d133afc7`, SHA-256
  `05e840a918579f57e8d662603978c484851b743e0bfbf5ba3e00733162426b18`;
- `src/_rasterizer_models_end.c`, Git blob
  `be1159bd3bd39fb338dab8a2fc3eade7de6eb458`, SHA-256
  `d2435ceb8d6f5d6ab5950505f33fd9b0f6665e3cc8db93d914aebf25ccf0c72d`.

HCEA identifies `rasterizer_debug_options.draw_models` at `+0x0C`, the
pixel-shader dirty flag, and the saved sky flag. Its later-build inner end is
empty, so that file is explicit version-negative evidence rather than a body
donor. January independently proves that the PC/Xbox beta pair profiles sky
as profile 1 and ordinary models as profile 2 on both entry and exit.

The retained source declares two translation-unit-local typed prefixes and
fixes `draw_models` at `+0x0C` and `sky` at `+0xBA` with compile-time
`offsetof` assertions. Both are declaration-only views of storage owned by
other objects. `data_0030cefb`, `bss_00465d68`, and
`rasterizer_debug_options` remain undefined externals; this wave does not
rehome or claim their storage.

January's exact begin topology is: test `draw_models`; set the dirty flag;
save `sky`; then call `rasterizer_profile_begin(1)` for sky or
`rasterizer_profile_begin(2)` otherwise. End tests the same option and saved
flag, then calls the corresponding `rasterizer_profile_end`. The natural
typed spelling reproduces all branch, cleanup, early-return, padding, and
relocation details without a retry.

## Policy and verification

The retained functions use one parameter per line, `void` on its own line in
the no-argument list, and explicit terminal returns. They contain no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, raw address or offset, representation pun, undefined behavior,
synthetic anchor, byte forcing, compiler-flag change, object patch, or
comparator exception.

- January object SHA-256:
  `994b40517a081b740bc93bb4f0b60617fd53b00657bdd175e8666a34e9eb1f15`.
- Final rebuilt object SHA-256:
  `b99af9f3e82f911ea4c7e8d75ceffb443a6db83b8949209be45b1b4a4c9cdfe7`.
- Baseline source Git blob:
  `c75fa455c889fc58517a0bd2ca70423fc4acd388`; retained source Git blob:
  `649dfcebfc3ddcdf278a47047270af0df4c740a1`; working-tree SHA-256:
  `0e6cb85e4fda81d8fe7fcd68b31b4f9e3cb3eee133ff662b32cd756ee6fa9a5a`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,792 functions,
  4,668 semantic exact, 4,678 accepted exact, and zero unit errors.
- Strict campaign board: 275/619 complete objects,
  637,287/1,922,669 padded code bytes, and 4,649/8,246 strict functions.
- Object admission: zero candidates, contradictions, and revocations.
- Parked ledger: 13 active, zero stale, and zero invalid.
- Tool suite: 205 passed; the optional pytest-cache warning is sandbox-only.
- `git diff --check`: pass.

Only `source/rasterizer/xbox/rasterizer_xbox_models.c` and this ledger are
retained. No protected Units, Vehicles, Matrix Math, AI, or game-engine
source; configuration; semantic ledger; parked entry; target object; or
unrelated translation unit was modified.
