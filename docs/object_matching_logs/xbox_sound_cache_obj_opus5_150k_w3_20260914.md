# `xbox_sound_cache.obj` — opus5 150K lane, wave w3 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 16 | 2 | 0 |
| real file, final | 16 | 2 | 0 |

The real file changed by **two `#include` lines only**, with no code-generation change:
- the `--all` rows are identical row by row;
- the object owner census is identical;
- park drift is 0;
- the guard passes;
- the fake scan reports 0 leads.

No exact credit is claimed.

## House-rule fix: C4013 implicit declarations (law A7)

At HEAD, `CL /Zs /W3` reported:
- line 517: C4013 `'cache_file_read' undefined; assuming extern returning int`;
- line 526: C4013 `'system_milliseconds' undefined; assuming extern returning int`.

The genuine owner headers are:
- `cache/cache_files.h` (`short cache_file_read(long, long, long, void *, boolean *, boolean)`);
- `cseries/cseries_windows.h` (`unsigned long system_milliseconds(void)`).

The sibling `xbox_texture_cache.c` includes exactly these two headers. Both includes were added. The warnings are
gone, and `sound_cache_start_loading_sound` stays byte-identical:
- the `cache_file_read` result is discarded;
- `last_allocation_failure_time` is `unsigned long`, so the subtraction was already unsigned.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `_render_inverse_transform_screen_point` | 256 | OWNER-BLOCKED (`_point_from_line3d`; identical to the texture-cache copy) |
| `_sound_cache_debug_render` | 368 | OWNER-BLOCKED (helper ABI), plus one x87 k-term operand-order pair |

Laws A22 dot canonicalisation and younger operand are diagnostic only. An operand swap would be rejection pattern R3.

Reopen criterion: an owner ruling on the `_point_from_line3d` COMDAT, then an evidence-led explanation of the k-term
load order.

Worker notes: `scratch/workers/w3_cache_files_decompress_windows_xbox_texture_cache_etc.md`.
