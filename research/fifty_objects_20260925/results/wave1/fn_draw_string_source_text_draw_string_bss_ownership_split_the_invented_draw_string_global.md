# source/text/draw_string (.bss ownership: split the invented _draw_string_globals into January's 4 file statics)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/final_ownership.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/ownership.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/symbols_json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/gen3.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/simcmp.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/object_audit_sim.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/simsplit1/source/text/draw_string.obj"
]

## production_changes
Landable now at zero code credit, independent of the owner ruling. Apply both parts in one commit, then run a csplit-only regen and a full ninja.

(a) From the worktree root, run `git apply scratch/w/draw_string/ownership.patch`. The result is byte-identical to scratch/w/draw_string/final_ownership.c. The patch:
- removes the `#define draw_character_software_globals/font_drawing_globals draw_string_globals` aliases and `struct draw_string_globals`;
- adds `struct font_drawing_globals`, using HCEX field names (string_list_index, current_font_index, current_flags, current_style, current_justification, current_color, tab_stop_count, tab_stops[MAXIMUM_NUMBER_OF_TAB_STOPS], highlight_start_index, highlight_stop_index, initial_indent, paragraph_indent) and `struct rasterizer_dynamic_screen_geometry_parameters multitexture_params`;
- adds `#include "rasterizer/rasterizer.h"`;
- adds four file statics in this declaration order, each with `= { 0 }`: anonymous-struct draw_character_software_globals {bitmap, encoding_shift}, text_pick_globals {pick_point, best_pick_string_index, best_pick_distance, last_string_index}, text_bounds_globals {bounds, last_font_header}, and `static struct font_drawing_globals font_drawing_globals`;
- rewrites every field reference to match;
- updates the header symbol-list comment.

(b) Run `git apply scratch/w/draw_string/symbols_json.patch`. It replaces config/symbols.json line 23271 `{ "file_offset": 4987144, "flags": 0, "name": "_draw_string_globals" },` IN PLACE with four lines:
- 4987144 `_draw_character_software_globals`
- 4987152 `_text_pick_globals`
- 4987164 `_text_bounds_globals`
- 4987176 `_font_drawing_globals`

Each line carries `"static": true`. Do not re-serialise the file.

Also required:
- relocs.json rows 82245-82255 and 82475 target file_offset 4987144 and keep working unchanged; the simulated csplit already exercised them.
- contribs.json needs no change.
- No semantic_matches, semantic_data_matches or symbol_ownership entry names _draw_string_globals.

## evidence
Name evidence:
- The January assert literals name two different globals: `draw_character_software_globals.bitmap->width` and `font_drawing_globals.tab_stop_count`.
- cachebeta.pdb has no public anywhere in 0x4C18xx-0x4C1Axx, and no other split object references the symbol, so January's storage was file-static. The current state is split 2 / ours 2 / truth 3, which is the object's only pdb_storage disagreement.
- HCEX.pdb (DIA2Dump -g/-sym/-type) lists four File Static symbols: text_pick_globals, text_bounds_globals and draw_character_software_globals (unnamed-tag structs), plus font_drawing_globals.
- The font_drawing_globals UDT gives the field names above and puts multitexture_params (rasterizer_dynamic_screen_geometry_parameters) at +0x4C. The repo's size check is 0x8C, which equals January's unreferenced tail at +0x6C..+0xF8.
- The /Od build places the same 4 objects in reverse order, while that 0x8C tail stays glued to font_drawing, so the tail belongs to font_drawing.

Layout law:
- With uninitialised statics, VC7 orders by name hash: pick 0, bounds 0x0C, dcs 0x18, font 0x20 (probe 3). That is not January's order.
- With `= { 0 }` in declaration order dcs, pick, bounds, font, VC7 places them at 0, 8, 0x14, 0x20, which is January's exact layout (probe 4, VC7 .bss law).

Verification (simulated split):
- build/tools/csplit.exe was run with a scratch copy of config into scratch/w/draw_string/simsplit*.
- With the unmodified config, the output is byte-identical to build/split.
- With the patched symbols.json, only draw_string.obj changes across the whole split (diff -rq). It defines the 4 statics (storage 3) at 0/8/0x14/0x20.
- final_ownership.obj against the new split: 27/28 functions strict-equal. The only exception is _bitmap_draw_string, which keeps the same safe-body residual as today.
- Simulated pdb_storage: 0 disagreements. object_audit fails only on that function.
- objdiff 3.3.1 gives scores identical to production (.bss 248 at 100%, .rdata at 100%, same function percentages).
- /W3: the same 13 warnings. fake_match_scan: 0 leads.
- All patches were re-applied outside the repo and reproduce the files byte-for-byte.
- `git status` shows no tracked file modified.

Note: until the split is regenerated, gate.py against the OLD split shows 23 reloc-identity rows. That is expected. Land source and symbols.json together.

## blockers
None for landing. It earns no code credit by itself, but it removes the object's only storage-truth defect, so the draw_string object is admission-clean apart from the owner-gated _bitmap_draw_string body.

## reopen_criteria
n/a. After landing, confirm with a full ninja plus `python scratch/tools/pdb_storage.py source/text/draw_string` (expect 0 disagreements) and gate.py (27 exact plus the unchanged _bitmap_draw_string residual).

## task notes
This slug directory did not exist at resume, so the run started fresh; nothing was lost.

**_bitmap_draw_string**
- It is exact only with January's NULL-bounds original bug.
- Two forms were measured exact: switch-case, and the /Od-attested valid_format flag.
- New in this run: the later /Od build (0x8b2090) has the same inverted test, which corroborates the bug from an independent build. It also names the locals adjusted_bounds and adjusted_clip.
- It remains owner-gated.

**Complete-object candidate**
- final_full.c passes every audit step against a simulated regenerated split.
- It needs the owner ruling plus the ownership packet.

**Ownership packet**
- It is independently landable now at zero credit.
- It fixes the object's pre-existing storage defect: one invented external 248-byte aggregate replaced by the four HCEX-attested file statics.

**Tools and safety**
- csplit was run only into scratch directories, with a copied config.
- `git status` shows no tracked file modified.
- A `git apply` test inside a repo subdirectory was a no-op (paths outside the subdirectory are ignored). Verification was redone outside the repo.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/LEDGER.md
