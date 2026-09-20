# `game_engine.obj` exact closeout (2026-09-19)

## Result

`source/game/game_engine.obj` is **180/180 strict exact** and
**33,760/33,760 padded code bytes exact**. This pass closes the four inherited
residuals for **+2,912 strict padded bytes**, without losing any of the 176
inherited exact functions:

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_drawline` | 112 | strict exact |
| `_default_starting_location_rate_function` | 176 | strict exact |
| `_game_engine_get_starting_location_rating` | 96 | strict exact |
| `_game_engine_post_rasterize_post_game` | 2,528 | strict exact |

The whole object is admissible as exact. No fuzzy or equal-size-only credit is
used in this result.

## First-party evidence that closed the object

The decisive donor is the same-revision symbol executable:

`C:\Users\isabe\Downloads\haloleak2024\halo1\h1_tags\halo_cache_symbols.exe`

The executable was compiled with runtime stack checking. Its function
epilogues pass `_RTC_CheckStackVars` a frame descriptor. Each descriptor has a
count and an array of `{ ebp_offset, size, name }` records, preserving genuine
source local names, extents and scopes even without the matching PDB. Literal
xrefs locate the function; the nearest real prologue and the epilogue's
descriptor reference make the extraction independent of guessed addresses.

The reusable decoder is `scratch/pe_rtc_frame_descriptor.py`. Other relevant
read-only evidence is:

- `scratch/h1_tags_halo_cache_game_engine_drawline.disasm.txt`
- `scratch/h1_tags_halo_cache_game_engine_drawline_rtc_locals.txt`
- `scratch/h1_tags_halo_cache_game_engine_default_rate.disasm.txt`
- `scratch/h1_tags_halo_cache_game_engine_postgame.disasm.txt`
- `scratch/h1_tags_halo_cache_game_engine_postgame_rtc_locals.txt`
- `scratch/ghidra_halo_cache_game_engine_decomp.c`
- `scratch/oct2276_vs_jan2342_game_engine_normalized.txt`
- `scratch/hcea_game_engine_postgame_lines.txt`
- `scratch/hcea_game_engine_postgame_sym.txt`

The January object remained the byte and ownership authority. The debug build,
HCEA records and October body were used only to recover source topology.

### `_drawline`

The debug body at `0x0059A8F0` materializes genuine `line_height = 18` and
`line_spacing = 8` dword locals. Restoring those named scalars, together with
the authenticated `(string, row_index, justification)` parameter order and
`short` justification type, reproduces January's two independent row-height
expressions. This closes the 112-byte function without volatile, fake aliases,
manual dependencies or assembly.

### Starting-location rating pair

The debug body at `0x0059A750` preserves the original wrapper topology:

- `player = player_get(player_index)`;
- `rating = 1.0f`;
- successive guarded multiplications by distance, friendly and engine-specific
  ratings;
- the public order `(player_index, starting_location)`.

Restoring this topology gives the private helper January's EBX/EDI lifetime and
also restores the public caller's hidden private-call contract. Both functions
close together, for 272 padded bytes.

### Postgame renderer

The `/RTC` descriptor at `0x005A56BC` authenticates sixteen aggregate locals,
including `buffer`, `custom_buffer`, `tab_stops`, `statistic_buffer`, the three
colors, `rect`, `team_tab_stops`, `team_order`, `team_string`, `fields`,
`custom_score`, `team_colors`, `bounds` and `options_color`.

Two additional facts come directly from the same-revision unoptimized body at
`0x005A4870`:

1. the HUD `font_index` is assigned after the six main tab stops and before the
   inlined color helper, then passed to `draw_string_set_draw_mode`;
2. the player rows are a zero-based indexed loop over
   `statistic_buffer[entry_index]`, with the display row computed as
   `entry_index + 8`.

The previous fuzzy source instead reloaded the font through `hud_globals` after
the color helper and hand-carried an entry pointer plus a decrementing count.
Replacing only those two source topologies makes the 2,528-byte January and
candidate streams both 752 instructions with no normalized instruction or
relocation difference. The indexed loop is also the natural C source visible
in the debug body; it is not a synthetic dependency or register-allocation
device.

## House-rule review

- No inline assembly, volatile, barriers, register forcing, pragmas, compiler
  flag changes, undefined behavior or raw-byte emission was introduced.
- No fake local, fake call, manual dependency, address-derived name or
  coincidental/nonsensical logic is present.
- The recovered private helper name and parameter order are authenticated by
  symbol/cross-build evidence.
- Typed object/tag access remains through the project's owning accessors.
- Every edited function retains an explicit return where required by the
  current house style.
- No header or central config file was changed.
- `_point_from_line3d` is not emitted by this translation unit.

`python tools/fake_match_scan.py source/game/game_engine.c` reports zero review
leads.

## Verification

Focused gate artifact: `scratch/game_engine_full_exact_20260919.obj`.

```text
== emitted-symbol guard passed (1 forbidden names checked)
== exact 180  residual 0  unwritten 0  (of 180 listed)
```

Additional checks:

- `tools/campaign/alndiff.py` reports no normalized difference for the final
  postgame function: 752 target instructions and 752 candidate instructions.
- The pre-postgame candidate and final candidate both contain 342 sections and
  853 symbols. Of the 342 sections, 341 are byte/relocation identical; the
  only changed non-debug section is the intended 2,528-byte postgame `.text`
  owner. Data, BSS, literals, COMDATs and every sibling owner are unchanged.
- All 180 target function sections pass strict bytes, padding, relocation type,
  destination, addend and ownership comparison.
- `git diff --check -- source/game/game_engine.c` passes.
- No source/header outside `source/game/game_engine.c` is part of this object
  closeout packet.

The scratch progression is retained for reproducibility:

- `scratch/game_engine_three_exact_landed.obj` — 179/180 checkpoint;
- `scratch/game_engine_postgame_font_index_authenticated.obj` — exact source
  ordering with only the inherited hand-written loop topology remaining;
- `scratch/game_engine_postgame_debug_indexed_loop.obj` — final exact renderer;
- `scratch/game_engine_full_exact_20260919.obj` — final 180/180 full-object gate.
