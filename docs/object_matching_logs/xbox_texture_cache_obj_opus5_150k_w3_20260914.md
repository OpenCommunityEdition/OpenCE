# `xbox_texture_cache.obj` — opus5 150K lane, wave w3 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 22 | 3 | 0 |
| real file, final (unchanged) | 22 | 3 | 0 |

All three rows are parked. Nothing landed: `git diff --stat` is empty, park drift is 0 and the fake scan reports 0 leads.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `_texture_cache_steal_memory` | 224 | SKIPPED-EXHAUSTED |
| `_render_inverse_transform_screen_point` | 256 | OWNER-BLOCKED |
| `_texture_cache_debug_render` | 784 | OWNER-BLOCKED |

### `_texture_cache_steal_memory`

This function is on the laws_w2 do-not-repeat list (4 shapes, tie). An alndiff re-read confirms 84/84 instructions.
The only code difference is which dead callee-saved register receives the late `lea [ebx+0x104000]`: January uses
ESI after pushing `stolen_size`, ours uses EDI after pushing `remaining_page_count`. The rest is the known
`_bss_004d1198` label gap.

No law among A1-A22 predicts that choice, so no shape was spent.

### `_render_inverse_transform_screen_point` / `_texture_cache_debug_render`

w2 measured both exact only through `point_from_line3d(&world_vertices[4], global_zero_vector3d, 1.0f, ...)`. That
form emits the `_point_from_line3d` COMDAT, and the owner ruling on it is out of scope for this lane. No new
non-emitting spelling exists: hand expansion is prohibited manual inlining and keeps the wrong helper ABI (w2 r1-r10).

Reopen criterion: an owner ruling on `_point_from_line3d`, plus the w2 symbols.json proposal
(`_bss_004d1198` → `_texture_cache_debug_bitmaps`, static).

Worker notes: `scratch/workers/w3_cache_files_decompress_windows_xbox_texture_cache_etc.md`.
