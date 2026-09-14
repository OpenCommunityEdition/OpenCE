# `rasterizer_swizzle.obj`: opus5 150K wave 2 park re-investigation (2026-09-14)

## Baseline

Lane base `12f7375d4`. Real-file gate with the `_point_from_line3d` guard: `exact 12 / residual 1 / unwritten 0`.
The only non-exact row is the parked `_rasterizer_xbox_bitmap_swizzle`: 800 padded bytes and 47 relocations on both
sides. The real body's normalized SHA-256 starts `1155ecd5`, equal to the park base. The target's starts `4e705de6`.

## Outcome: park-reopen proposed (scratch only, real file untouched)

The candidate is `scratch/workers/connected_geometry_rasterizer_geometry_etc/rasterizer_swizzle_reopen_candidate.c`:
the current real file plus this one body. Its whole-TU gate is **13 exact / 0 residual**.
`_rasterizer_xbox_bitmap_swizzle` is strict EXACT at 800/47, with SHA `4e705de69eb0ee60...`.

### Evidence

January's cube-map preheader runs in this order:

1. Home the widened switch value `(long)bytes_per_pixel` to `[ebp-0x1c]`.
2. Divide `pixel_data_size` by 6 using the `0x2aaaaaab` magic multiply.
3. Home `face_size` to `[ebp-0x20]` and the strength-reduced `buffer - pixels` delta to `[ebp-0xc]`.

Both homed values are loop invariants. VC7 hoists them in loop-body order, and `switch (bytes_per_pixel)` comes first
in the body. So the division must come from inside the loop body, after the switch.

The parked body instead computed `long face_size = pixel_data_size/6;` as a block initializer before the loop. VC7
schedules that ahead of the hoisted switch value. That is the "backend schedule" the park describes. This refutes the
park's premise that no source lever exists.

### Source change (2 insertions, 3 deletions)

```c
destination += pixel_data_size/NUMBER_OF_FACES_PER_CUBE;
source += pixel_data_size/NUMBER_OF_FACES_PER_CUBE;
```

The `face_size` local is removed. The literal `6` becomes the cseries owner constant, which the sibling
`rasterizer_xbox_bitmap_rebuild_hardware_format` already uses (`mipmap_size /= NUMBER_OF_FACES_PER_CUBE`). The change
adds no named steering temporary, dead code, cast, or prohibited construct.

### Shapes

| Shape | Result |
| --- | --- |
| X1: division inline in both pointer increments | **exact** |
| X2: indexed `buffer + face_index*face_size` arguments | 15 differing instructions, rejected |

### Admission checks on the candidate

- All 12 baseline-exact rows stay exact.
- The point guard passes.
- `scratch/parkcheck.py`: this function is the unit's only park; its DRIFT is the reopen itself.
- `relocdiff`: 47/47 relocations, 0 differing rows.
- Owner census against `build/base` and `build/split`: no candidate-only owner, none lost.
- `tools/fake_match_scan.py`: 0 leads.
- No header or config change is needed.

## Orchestrator action

Remove the `_rasterizer_xbox_bitmap_swizzle` entry from `config/parked.json`, then apply the candidate body in a
dedicated commit.

## Reusable law (proposed)

**Loop-invariant hoist order.** VC7 emits hoisted invariants in the preheader in the order they occur in the loop body.
A named invariant computed before the loop is emitted ahead of them. If January homes a body-internal invariant (such
as a switch expression) before an arithmetic invariant, the arithmetic was written inside the loop.
