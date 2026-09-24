# source/bitmaps/bitmap_group

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\production_storage_only.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\semantic_data_entry.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bitmap_group\\LEDGER.md"
]

## production_changes
All files are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bitmap_group\. Nothing is strictly exact, so these changes carry no function credit.
(1) production.patch replaces source/bitmaps/bitmap_group.c with cand.c. `git apply --check` is clean at 931ed8dc. It makes these changes:
  - _bitmap_group_add_bitmap initialises the descriptor in struct order (signature, width, height, depth, type, format, flags, ...), the same order January's exact bitmap_2d_new uses.
  - It uses the chained assignment `registration_point.x = registration_point.y = 0`.
  - It returns once, through `short bitmap_index = NONE; boolean valid = TRUE;` (the single-exit law).
  - The cube-map case is nested: `else if (type == cube) { if (width == height) SET(pow2); else { msg; valid = FALSE; } } else SET(pow2);`.
  - postprocess_bitmap and delete_bitmap become `static`.
  - The file adds `#include "bitmaps/bitmap_group_internal.h"`, its own header.
  The function stays PARKED. Re-baseline the config/parked.json entry for _bitmap_group_add_bitmap to: base size 1008, relocation_count 59, normalized_sha256 89e4adcee46d218320f022e26a8c07ce50713ec52857c7a53bcf9b631573eb47. Take objdiff_percent from the rebuilt report.
(2) config/symbols.json (full edited copy): lines 1224 and 1225 each gain `, "static": true` for _postprocess_bitmap and _delete_bitmap. The copy was edited in place and is byte-identical elsewhere. Both functions are absent from the cachebeta publics, so January made them file-static. csplit must regenerate bitmap_group. Nothing outside this file references either name.
(3) semantic_data_entry.json: append it to config/semantic_data_matches.json. It is a single-section entry on _global_bitmap_reference with allow_incomplete_unit. Measurements: size 1424, relocation_count 152, normalized_sha256 df5c6d69f4e13ea6df9398e2b5cc5c4455410b077b5ba6763930c4dd748505cb. It credits +1,424 data bytes: unmatched data is 7868 - 6444 = 1424, the whole section.
Alternative minimal packet: production_storage_only.patch adds only the static fixes and the own-header include, and leaves the parked body alone. Its body hash drifts to 2e0bbd84c45f240758cc76a2d4df8c78be689cfe9bc21af6a52e017f218e7ca0 (992/59), so the park needs a re-baseline.

## evidence
Baseline: _bitmap_group_add_bitmap residual [size 992!=1008, sha]; five siblings EXACT.
Candidate: gate [sha] only, 1008/1008 bytes, 59/59 relocations, 346/346 instructions, 4 instructions differ (cand.gate.txt, cand.alndiff.txt). Siblings still EXACT. fake_match_scan: 0 leads.

Key findings:
(a) The 'zero-pseudo pin' that three earlier ledgers called unreachable is a source defect. L5b used January's emitted store order as its source order. Struct order, the idiom of bitmaps.c's bitmap_2d_new, removes our surplus zero web. The result is January's `test eax,eax`, both `xor edx,edx`, the frame, and the parameter-home spills (+0xc/+0x10), with the size now exact (labS1).
(b) The chained x=y=0 assignment gives January's y-before-x store order (labS2). Precedents: editor_flying_camera.c:699, items.c:671, flags.c:347.
(c) The cold-block layout: the /FAsc listing shows C1 emits && and || with every jump going to a single target. January's cube test jumps to two targets (type!=cube to success, w!=h to msg) and falls into success. That is the signature of a nested if whose duplicate success arm was tail-merged. Nine single-arm spellings were refuted (S3, C1, L2, F1, E2, E3, T4, D1, I1); labD2 reproduces the layout exactly.
(d) The success-block reload order is a TU name-count tie (oracle N=1..13). The genuine own-header include lands in that window.
(e) cachebeta publics lack postprocess_bitmap and delete_bitmap. Making them static keeps the code exact and the .data resolved-equal, checked with the edited symbols.json.
(f) .data: datasec/data_entry show January and ours resolved-equal (1424 bytes, 152 relocations, sha df5c6d69..., same flags and storage). build/report.json scores it 83.3998% under objdiff 3.3.1; 111 of the relocations target ?$AA@ literals.

object_audit on cand.obj: only add_bitmap differs, plus 2 expected storage differences (2 vs 3) that go away when the split is regenerated from the static symbols.json. Surplus: .drectve and 3 folded literals; provider_link PASS; surplus_identity 0 code COMDATs.

There is no /Od copy of this function (halo_cache_symbols lacks it), and HCEX.pdb lacks it. All probes are listed in LEDGER.md.

## blockers
4 instructions in the loop body. This is a size- and relocation-neutral pair of choices about which operand becomes the destination of the add, and in what order its operands load:
- +0x270: January has `mov ecx,[edi+0x3c]; mov eax,[esi+0x18]; add eax,ecx`; ours swaps eax/ecx.
- +0x2a9: January has `mov edx,[edi+0x30]; mov ebx,[edi+0x3c]; add edx,ebx`; ours loads the address first.
In January the integer operand is the add destination at both `(byte*)group->pixel_data.address + int` sites.

Measured as inert (no effect on these rows):
- operand order, array form, integer-typed add, void*/char* casts
- shadow-header field types for address, base_address, pixels_offset and size, and `byte` as a macro
- assert macro form, base-variable ordinal, identifier interning order
- TU and local name-count oracles, definition order
- loop scoping and increment, space_between and pixels_end respellings, bitmap_index as the loop counter

Diagnostics show both rows are allocation choices driven by the loop body's IL. Dropping the space_between block flips site 1; an unnatural two-step base_address assignment flips site 2. No natural, house-rule-clean spelling does either. The methodology stop rule has fired.

The object is otherwise complete once packet items (2) and (3) land.

## reopen_criteria
Reopen on any of these:
- a first-party copy of bitmap_group_add_bitmap (a tool build, or HCEX_Release locals) that shows the loop-body statement or local shape;
- a decode of how C2 picks the destination of a commutative add (dbg32c at candidate +0x270), naming the IL property that keeps group->pixel_data.address as the non-destination operand;
- a natural loop-body respelling that flips both rows. Start from cand.c; the diagnostics diagE4 and baseaddr2 show which IL moves each row.

## task notes
`_bitmap_group_add_bitmap` is not exact, so no function credit. It went from 992 bytes with 118 differing instructions to 1008 bytes (matching January), 59 relocations and 4 differing instructions.

1. **Earlier "unreachable zero pin" claim is refuted.** Writing the descriptor stores in struct order, as bitmaps.c does, removes our extra zero register. LAW Z's rule that our compiler never adds a zero register January lacks still holds.
2. **Block layout lever.** A nested cube-map `if` with a duplicate success arm reproduces January's two-target branch. That is how a tail-merged duplicate arm looks in the output.
3. **Storage fix.** Two private functions, postprocess_bitmap and delete_bitmap, should be static because the cachebeta.pdb publics do not list them.
4. **Data credit available now.** The .data section is byte-identical under coff_compare; objdiff 3.3.1 undercounts it. One semantic_data_matches entry credits +1,424 data bytes, with no dependence on the function.

**Reviewer judgment calls:**
- Is the duplicated SET_FLAG(pow2) arm natural Bungie code? It is load-bearing for the block layout only, not a decoration, and no single-arm spelling reproduces January's branch shape (9 tried).
- The added own-header include lines up the TU name count for the reload order. The include is genuine, but its only effect here is on that tie.

No tracked files were edited. No ninja, configure or git-mutating commands were run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bitmap_group\LEDGER.md
