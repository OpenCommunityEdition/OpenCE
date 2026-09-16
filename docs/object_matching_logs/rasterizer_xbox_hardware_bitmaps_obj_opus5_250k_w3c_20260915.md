# `rasterizer_xbox_hardware_bitmaps.obj` — Opus5 250K lane, wave w3c (2026-09-15)

Lane `opus/250k-house-clean-20260915`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Scope: `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c` only.

## Result

**No source change landed.** `git diff --stat` for the file is empty.

| gate (real file, `--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 18 | 1 | 0 |
| final | 18 | 1 | 0 |

Guard: pass. `scratch/parkcheck.py`: 1 park, drift 0. `tools/fake_match_scan.py`: 0 review leads.

## `_rasterizer_bitmap_new` (400 B, parked) — skipped, matches a recorded negative result

Target 400 padded / 26 relocs / `041cef84e2a13eea…`; park base 400 / 26 / `719469460357f06c…`; objdiff 99.96377.

Re-verified at this HEAD: `tools.campaign.alndiff` reports **zero instruction differences** (150 against 150; the only rows
printed are relocation-label spellings, which are an artifact of comparing a `build/base` object against the split target).
The single normalized byte is the `jne` displacement at `+0x15A`, which selects between two byte-identical
`bitmap->hardware_format = NULL; return TRUE` epilogues — January takes the first (`+0x171`), current VC7 takes the second
(`+0x17E`). Emitted behaviour is identical.

### Law detector run

This is exactly laws_w3 **A43 (EPILOGUE / CROSS-JUMP SURVIVOR)**, which is recorded as a **negative result**: "No source
lever moves which duplicate tail survives" — demonstrated for `_virtual_keyboard_select` (a dedicated lab: the source-later
arm always keeps the merged tail), `_weapon_can_be_fired`, `_actor_path_refresh`, `_actor_action_handle_vehicle_entry` and
`_render_debug_add_cache_entry`. The object's own ledger independently records more than 50 ordinary-C probes over shared
versus split returns, branch polarity, `goto` topologies, explicit success materialisation, duplicate and single cleanup
blocks, and declaration/lifetime changes, all of which preserved the one-byte choice or regressed earlier exact code.

No other w2/w3 law fires: there is no size, relocation, frame or instruction-selection delta to attach a detector to.
**No shape was spent** — spending one would repeat both the object's own do-not-repeat list and a lane-level negative.

**Reopen criterion** (unchanged from the park): original January source/local/PDB provenance, an authenticated cross-build
donor with the same block layout, or an ordinary-C VC7 control that changes only this branch-threading decision while
preserving the exact prefix, size, relocation schedule, exact siblings and data ownership.

## Evidence used

January split COFF bytes and relocations; `tools.campaign.alndiff`, `tools/campaign/gate.py`, `scratch/parkcheck.py`,
`tools/fake_match_scan.py`; `config/parked.json`; `docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj.md` and
`…_obj_upload_helpers_blueprint_20260830.md`; `scratch/w3/laws_w3.md` (A43). No supplied binary was executed.
Worker notes: `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc.md`.
