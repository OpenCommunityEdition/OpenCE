# `flags.obj` counter-initializer probe — held (2026-09-23)

Base: `5019c1861a8062e8c0f61a7a84c4e83753b4fbb3`. This is research only: no production source, config, or park change is admitted, and no exact bytes are credited.

The previously documented `_flag_render_proper` residual is a 2,112-byte function with 677/677 aligned instructions and 84 ordered relocations. January stores the zero-valued `x` register into `vertex_index` after `csmemset`; our current source stores an immediate zero before it. A one-hunk probe reproduced January exactly:

```diff
- for (x = 0, vertex_index = 0; x < definition->width; x++)
+ for (x = 0, vertex_index = x * definition->height; x < definition->width; x++)
```

`python tools/campaign/gate.py source/objects/widgets/flags --all --forbid-emitted-symbol _point_from_line3d` reported 15 exact functions, `_flag_update` as the sole residual, and no forbidden emission. The simpler `vertex_index = x` initializer also gates exact. Both forms are equivalent to zero because the preceding comma operand assigns `x = 0`. The multiplication has a recognizable flattened-grid meaning (`flag_datum_get_vertex` uses `x * height + y`), but **at this site it has no independent semantic purpose or first-party source attestation**. It is therefore a suspect code-generation lever under the house rule against fake matching. Do not integrate it merely for 2,104 meaningful / 2,112 padded exact bytes.

Controls that give the multiplication a meaningful role do **not** close the function:

- `for (x = 0; ...; x++) { vertex_index = x * definition->height; for (y = 0; ...; y++, vertex_index++) ... }` remains residual at 2,112 padded bytes.
- Removing the counter and addressing `vertices[x * definition->height + y]` remains residual at 2,080 padded bytes.
- Earlier source-shape controls and the allocator/schedule analysis are in `flags_obj_opus5_150k_w2_20260914.md` and `flags_obj_opus5_next150_tierB_20260915.md`.

The original `flags.c` is restored and content-identical to the base. Reopen only with first-party source/local evidence that January initialized the flattened index from `x`, or an independently established VC7 source law that explains the delayed register-copy store without a redundant expression. Preserve the exact hunk above as a probe, not a landing.
