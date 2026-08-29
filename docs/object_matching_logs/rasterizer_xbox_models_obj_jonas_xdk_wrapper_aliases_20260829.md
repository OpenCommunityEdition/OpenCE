# `rasterizer_xbox_models.obj` XDK wrapper aliases (2026-08-29)

## Result

Five anonymous January owners in `rasterizer_xbox_models.obj` are now assigned
their evidence-backed stock-XDK identities. This is an ownership correction,
not a code-credit claim: the current model source does not naturally instantiate
these wrappers until the remaining model draw path is reconstructed, so the
object gate still reports them as unwritten.

The correction removes all five false anonymous names without changing C source,
compiler flags, matching status, semantic adjudications, or parked functions.

## Strict cross-object proof

The current branch regenerated the January split from `cachebeta.exe` with the
repository-pinned `csplit` v0.0.2 tool, rebuilt
`rasterizer_xbox_motion_sensor.obj` with XDK 3911, and compared each renamed
model section directly against the naturally emitted wrapper in the motion
sensor object using `tools/coff_compare.py`.

Every pair is strict semantic-COFF exact: padded bytes, normalized bytes,
relocation count, relocation address and type, symbolic destination, and addend
all agree.

| January offset | Correct owner | Bytes | Relocs | Normalized SHA-256 |
| ---: | --- | ---: | ---: | --- |
| `0x15A150` | `_D3DDevice_SetRenderState` | 432 | 32 | `0c9239db142a50a3fe1585fe634701a6d96f7cf60c6424feeb317433eae9e4d0` |
| `0x15A300` | `_D3DDevice_SetTextureStageState` | 80 | 5 | `b6cf386298aad881ad444eb1d4f3f732cfd75e40bc5626748da27fe3eac79469` |
| `0x15A740` | `_IDirect3DDevice8_SetRenderState@12` | 544 | 32 | `d391ce622a8d1be40209833f735c268b440f24a07bc06da8e1b306fc82ad8a0e` |
| `0x15A960` | `_IDirect3DDevice8_SetTextureStageState@16` | 96 | 5 | `89d1c696147a0aae6d137f03b3bf6d284aa9ef5c083a33905dd28caeb08775c3` |
| `0x15A9C0` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | 16 | 1 | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |

Together the identities cover 1,168 padded bytes and 75 ordered relocations.
The vertex-constant wrapper's single relocation is at section offset `+0x4` to
`_D3DDevice_SetVertexShaderConstant@12`; this resolves the otherwise common
16-byte wrapper shape unambiguously. The render-state and texture-stage-state
families have distinctive full relocation inventories and match the same stock
XDK definitions already proven in the Motion Sensor, Plasma Energy, and Detail
Objects closeouts.

## Gates and boundary

- regenerated model split: pass;
- direct five-function cross-object comparison: `all_equal: true`;
- model gate after renaming: 5 exact, 0 residual, 9 unwritten;
- global alias scan: no unambiguous candidate; two unrelated 16-byte
  `rasterizer_xbox.obj` owners remain ambiguous between the same three helpers;
- source diff: none;
- config diff: exactly five name replacements; file offsets and flags unchanged;
- `units.c`, `vehicles.c`, and `matrix_math.c`: untouched.

No assembly, `volatile`, `register`, pragma, intrinsic, barrier, attribute, raw
address or offset access, representation pun, undefined behavior, byte forcing,
dummy emission anchor, object patch, tool exception, or compiler change is
introduced. Future work should obtain these wrappers only through the natural
typed Direct3D calls in the real model draw implementation.
