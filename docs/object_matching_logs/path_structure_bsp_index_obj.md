# path_structure_bsp.obj — structure_surface_index_from_point

This isolated lane reconstructs only `structure_surface_index_from_point` from
the January 14, 2002 target. The remaining five object functions are intentionally
absent and are owned by the parent atomic-object lane.

## Target evidence

- Padded size: `0x60` (96 bytes); meaningful size: 82 bytes.
- Relocations: one `REL32` call to `_structure_test_line2d` at function offset `0x28`.
- The target has three distinct returns: immediate `NONE`, fallback to
  `known_surface_index`, and the recovered surface index. Multiple returns are
  retained because they are required by the January control flow, despite the
  project's general preference for a single return.
- The stack local is a `0x1c`-byte `path_collision_result`; its point begins at
  offset `+0x04`, and its surface index is at `+0x10`.

## Experiments

### E1 — HCEA topology with January-authoritative types

- Shape: guard return for `known_surface_index == NONE`, one line-test call,
  copy the resulting 2D point, then fallback/result returns.
- Source provenance: HCEA supplied the control-flow hypothesis; January bytes
  independently proved every parameter, stack offset, call argument, and return.
- Result: size and relocation count matched (`0x60`, 1), but the call relocation
  landed at `+0x2f` instead of `+0x28`. VC7 placed the immediate `NONE` return
  before the body, adding a seven-byte early epilogue that January places after
  the body. Rejected.

### E2 — positive body guard, trailing `NONE` return

- Shape: wrap the line-test body in `if (known_surface_index != NONE)`, keep the
  fallback return within it, and put the `NONE` return after the guarded body.
- Prediction: this preserves the same three semantic exits while putting the
  immediate-`NONE` materialization after the body, as required by the target CFG.
- Result: **strict exact**. Target/candidate padded size `0x60`; relocation count
  `1/1`; call relocation address/type/destination/addend equal at `+0x28`; both
  normalized SHA-256 values are
  `29cfb8955fc45cfc7972505dddb757d4374ca33735024c18aa7f5baac71b8f49`.

## Verification

```text
ninja build/base/source/ai/path_structure_bsp.obj
python -m tools.coff_compare build/split/source/ai/path_structure_bsp.obj \
  build/base/source/ai/path_structure_bsp.obj \
  _structure_surface_index_from_point
```

The hardened comparator returned `all_equal: true`. No data sections are owned
by this function-only lane. Whole-object admission remains the parent lane's
responsibility after the other five functions and shared data are reconstructed.
