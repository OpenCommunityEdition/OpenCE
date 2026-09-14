# `collision_usage.obj` Opus5 150K house-clean lane, wave w2 park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/physics/collision_usage.c`. Target `build/split/source/physics/collision_usage.obj`
  (January 2002 Xbox debug build; XDK 3911 `CL.Exe` 13.00.9254; flags `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/150k-house-clean-20260914`, worker `w2:collision_usage_collisions`. Only `collision_usage.c` and `collisions.c`
  were in scope. **No source, header, config or park file was changed.** All experiments ran on scratch copies under
  `scratch/workers/collision_usage_collisions/`. Worker notes: `scratch/workers/collision_usage_collisions.md`.
- Read first: the five earlier ledgers (`codex_checkpoint`, `jonas_start_time_wrapper_20260827`,
  `jonas_timing_display_wave_20260829`, `jonas_renderer_one_shot_20260829`, `small_family_completion_20260902`), both
  `config/parked.json` entries, the 100K handoff, `git log --all -- source/physics/collision_usage.c` and
  `tools/campaign/branch_sweep.py` (9 blobs; none holds an exact renderer or current-user helper).

## Baseline and final (real file identical)

Real-file gate with `--forbid-emitted-symbol _point_from_line3d` (guard passed): **12 exact / 2 residual / 0 unwritten**.
Final real-file gate: identical rows. `scratch/parkcheck.py`: 2 parks, 0 drift. Owner census against `build/base` and
the target: no candidate-only or base-only owners. `tools/fake_match_scan.py`: 0 leads. `git diff --stat` empty.

| owner | target padded / relocs / sha16 | real file | outcome |
| --- | --- | --- | --- |
| `_collision_log_render` | 1584 / 79 / `dc4c9ded347bbcf3` | 1536 / 76 / `f586255e6af97eac` | **park-reopen proposed (scratch strict EXACT)** |
| `_collision_log_get_current_user` | 256 / 23 / `fc370d7e130011908` | 256 / 23 / `0f1289b9c2652df5` | not landed; park kept |

## `_collision_log_render`: park reopen proposed

Candidate: `scratch/workers/collision_usage_collisions/collision_usage_park_reopen_render.c` (the current real file with
only the renderer body replaced). Whole-TU gate: **13 exact / 1 residual**; the only row change is the renderer
residual -> EXACT (1584 padded, 79 relocations, normalized SHA-256 `dc4c9ded…a213`, identical to January). The guard
passes, the other park (`_collision_log_get_current_user`) does not drift, the owner census is clean and the fake scan
has no leads.

### New January evidence (relocation order and disassembly of the target object)

1. **Wrong CRT symbol.** All four formatted appends relocate to `__snprintf`, i.e. the XDK CRT `_snprintf` that exact
   `ai_debug.c` also calls. The parked body called the undeclared `snprintf` (implicit declaration, `_snprintf` symbol).
2. **Store/call order.** `_csstrlen` is called *before* the inlined `collision_log_format_usage` at all four append sites
   (0x248, 0x2f8, 0x42a, 0x4dd), with the length held in EBX/ESI across the format call. The length is therefore
   computed first: `short length = (short)csstrlen(temporary_string);` as a block-start initialized declaration.
3. **Aggregation topology.** Inside the `reset_upon_next_use` test January copies the period record and adds
   `collision_usage_buffer[p].function[f].usage_by_user[u].calls / .elapsed_time` into `total_usage.usage_by_period[p]`;
   these are the `+24`, `+32` and `+36` relocations the park recorded. The four `total_all_periods` accumulations
   (per user and overall) run unconditionally after the test, reading `overall_usage[u].usage_by_period[p]`
   (`[eax]`, `[eax+8]`, `[eax+0xc]`, loop pointers stepping by 0x10). A non-reset period contributes zero because
   `overall_usage` is cleared by the preceding `memset`, so the logic is the same "totals are sums of the per-period
   rows" computation.
4. **Position local.** January loads `rasterizer_globals+16` as a dword, adds -30 and homes it in a 32-bit slot whose
   uses are all 16-bit (the `bounds.y0` store and the `+= bounds.y0 - cursor.y` update, both dword reloads with dead
   upper bits). The local is `short debug_string_position`. `frame_bounds.x0` is read directly from
   `rasterizer_globals+14` at the draw site. There is no `rectangle2d` copy, which removes the extra `+12` relocation.
5. **Bounds stores.** January stores x0, y0, x1, y1. The source order `y0, x0, x1, y1` is the spelling of the accepted
   exact sibling `_ai_profile_draw_string` (`source/ai/ai_profile.c:428-431`), which uses the same terminal-font
   `interface_set_bitmap_text_draw_mode` / `draw_string_set_color` / `rasterizer_draw_string` sequence.

### Shapes (4 of 5)

| shape | change | result |
| --- | --- | --- |
| r1 | items 1-4, `y1` before `x1` | 1584 / 79; one store-order difference (`[ebp-0x20]` vs `[ebp-0x1e]`) |
| r2 | r1 + `bounds.y1 = bounds.x1 = SHORT_MAX;` (spelling in `xbox_texture_cache.c:961`) | EXACT |
| r3 | r1 + `bounds.x1 = SHORT_MAX; bounds.y1 = SHORT_MAX;` (ai_profile spelling) | EXACT |
| r4 | r3 + `_text_style_plain` for the style argument | compile failure: `text_group.h` is not in this TU's include set (not added) |
| r5 | r3 with `char temporary_buffer[256];` declared before `short length = …` | **EXACT (chosen candidate)** |

### Why the park criterion is met

The park asked for evidence that explains the frame and the aggregation lifetimes together. The evidence above is
January-side: relocation identities and order, call order and the 16-bit use of the position slot. It explains both
boundaries recorded in the park (`+12` versus `+16`, and the direct `+24/+32/+36` materialisations). It also corrects a
wrong relocation target (`snprintf`) that the park had not recorded. The earlier one-shot "do not retry" list rejected
isolated tweaks (removing casts, scalarising `frame_bounds`, expanding the pointer). This candidate is one complete
shape derived from the target's order, not a tweak sweep.

House rules: typed fields and globals, `NUMBEROF`, `SHORT_MAX`, `match_assert` with January line 342, declare+initialize,
explicit `return;`. There is no cast trick, pragma, volatile/register steering, raw offset or new owner.

## `_collision_log_get_current_user`: not landed (park kept)

January's two `NONE` exits are 32-bit: `mov eax,0xffffffff` at +0xb7 and the shared `or eax,-1` at +0xee. This is
exactly what the `long` body emits. The successful return is 16-bit `mov ax,si`, where we emit `movsx eax,si`. Both
callers consume only AX and are exact under either prototype.

| shape | result |
| --- | --- |
| s1 `short` return, same body | 240 bytes: the two NONE exits merge into `or ax,0xffff` |
| s2 `short` + single-exit result local | 256 bytes, but both exact callers (`end_time`, `usage`) regress (register convention) |
| s3 `short` with nested enable block | identical to s1 |
| lab `lab/r1.c`-`r3.c`, 13 variants at collision_usage flags | `long` always sign-extends, with or without prototypes and in either caller order. `short`/`unsigned short`, a long intermediate, a direct global return and nested or inverted tests always merge into a 16-bit constant. No return-value narrowing for statics. |

Classification unchanged (return-width / tail-merge boundary, unclassified). **Reopen criterion (unchanged):**
authoritative prototype or local evidence, or a same-compiler function with 32-bit constant returns and a 16-bit value
return in one body.

## Proposals

- **park-reopen** `_collision_log_render` from `scratch/workers/collision_usage_collisions/collision_usage_park_reopen_render.c`.
  Needed: remove the `config/parked.json` entry and replace the body in a dedicated commit. The body uses only existing
  includes and owners, so no header or config change is needed besides the park entry.
