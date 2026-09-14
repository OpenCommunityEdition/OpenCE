# `rasterizer_xbox_hardware_geometry.obj` — Opus5 150K wave w2 (2026-09-14)

## Baseline and outcome

The unmodified real file gates at **15 exact / 2 residual / 0 unwritten**, and the guard passed. Both residuals are parked (`rasterizer_xbox_hardware_geometry_obj.md`, which includes a do-not-repeat sweep list). The real file is unchanged and nothing was landed.

| Function | Target | Real file |
| --- | --- | --- |
| `_rasterizer_vertex_buffer_new` | 368 / 20 / 5442bf32f8b49c2a | 368 / 20 / bf6b5feafdc7750b |
| `_rasterizer_triangle_buffer_new` | 352 / 24 / e90e066bbde0d04c | 352 / 24 / b0fdebbf1336f833 |

## Evidence gathered

- **No callers.** No January object references either constructor (xref census), so no caller-side ABI evidence exists.
- **No Unlock.** The XDK `D3DVertexBuffer_Unlock` and `D3DIndexBuffer_Unlock` are empty `static __forceinline` bodies. January emits Lock and Release wrapper COMDATs but no Unlock COMDAT, so the original source never called Unlock. An empty-inline Unlock as a natural schedule anchor is therefore rejected.
- **Common tail pattern.** In both tails, January reloads the address-taken D3D buffer local into the register freed by the first field store (EDX for the vertex constructor, ECX for the triangle constructor). Our build reloads it after the last store. This is an allocation/scheduling tie on a local whose address was taken.
- **Static linkage does not apply.** The window/cinematics static-linkage lever has no target here: `global_d3d_device` and `rasterizer_globals` have genuine PDB publics.

## Shapes (scratch only; bounded because the ledger forbids repeating the sweeps)

| Shape | Result |
| --- | --- |
| Vertex constructor without the barrier, five evidence orders (vt1–vt5) | all 368 / 20, sha differs |
| `short vertex_type` (HCEA `int16_t`) with the base body | identical to base |
| Triangle constructor with both `_ReadWriteBarrier` calls removed (tb1) | 336 / 24; the failure-block layout changes, a structural loss |

## House-rule debt (for the orchestrator)

Both parked bodies carry `_ReadWriteBarrier()` and `#pragma intrinsic(_ReadWriteBarrier)`, which the current brief prohibits. The triangle body also jumps with a `goto` into an if block.

The barrier-free vertex body `scratch/workers/rasterizer_xbox_rasterizer_xbox_hardware_geometry_etc/hg_vt3.c` keeps the same 368/20 envelope (sha `2ec337c00675316c`). A park re-baseline to it would remove the barrier at no measurable cost. The triangle body has no barrier-free equivalent yet.

## Reopen criteria

- Unchanged from the object ledger: original locals or source, or a same-compiler donor with the same post-copy live ranges.
- In addition, any barrier-free triangle CFG that reaches 352/24 would make a re-baseline proposable.
