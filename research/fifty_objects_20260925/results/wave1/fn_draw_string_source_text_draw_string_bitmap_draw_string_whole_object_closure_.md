# source/text/draw_string :: _bitmap_draw_string (whole-object closure)

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/final_full.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/bug_body_after_ownership.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/bug_body_only_vs_production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/cand2.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/symbols_json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/draw_string/LEDGER.md"
]

## production_changes
APPLY ONLY AFTER AN OWNER RULING that admits this BUG (original) body. The brief says to report it and not land it.

(1) Preferred: land the ownership packet first (next unit), then run `git apply scratch/w/draw_string/bug_body_after_ownership.patch` from the worktree root. Alternatively, `git apply scratch/w/draw_string/production.patch` does both at once; the result is byte-identical to scratch/w/draw_string/final_full.c (CRLF).

The new body of _bitmap_draw_string follows the /Od topology:
- the format switch sets `boolean valid_format` TRUE for the 5 listed formats and FALSE by default;
- `if (valid_format)` wraps the rest;
- inside, `if (!bounds)` runs set_rectangle2d(&adjusted_bounds, MAX(0,bounds->x0), MAX(0,bounds->y0), MIN(bitmap->width,bounds->x1), MIN(bitmap->height,bounds->y1)) and then `bounds=&adjusted_bounds`, under a /* BUG (original) */ comment giving the safe corrected alternative;
- the same clip is applied to `clip` via adjusted_clip;
- then draw_string(bitmap_draw_character, bounds, NULL, clip, 0, string).

(2) Retire the park: in config/parked.json, delete the entry `{ unit source/text/draw_string, function _bitmap_draw_string, class unclassified ... }` (lines ~781-799 in this worktree). Edit in place; do not re-serialise.

(3) After the ownership packet, csplit regen and a full ninja, set config/config.json line 424 `source/text/draw_string.c` status from NonMatching to Matching, but only if the integrator's admission audit agrees.

(4) Function-only fallback, if the ownership packet is not landed: `git apply scratch/w/draw_string/bug_body_only_vs_production.patch`, which equals cand2.c. It gates exact 28 / residual 0 against the CURRENT split with no split regen, but the object keeps its one pdb_storage disagreement.

## evidence
January (tinfo/dis_range):
- The code is: `mov [_draw_string_globals],esi`, then the switch jump table.
- `mov ebx,[ebp+0xc]; test ebx,ebx; jne 0x94`. The NULL arm reads [ebx], [2], [4] and [6] with no relocation (bounds==NULL is constant-propagated).
- The arm clips those values against bitmap[4]/[6] through MAX(0,..)/MIN(bitmap,..) into set_rectangle2d(&[ebp-8]), then does `lea ebx,[ebp-8]`.
- The layout proves the fields: rectangle2d is {y0@0, x0@2, y1@4, x1@6}, and the push order x0=MAX(0,[2]), y0=MAX(0,[0]), x1=MIN(width,[6]), y1=MIN(height,[4]) matches bounds->x0/y0/x1/y1 exactly.

NEW this run: the later first-party /Od build (halo_cache_symbols.exe fn 0x8b2090, dumped to scratch/w/draw_string/od_8b2090.txt) has the SAME inverted test `cmp [ebp+0xc],0; jne skip` followed by bounds-> reads. That is independent corroboration of the original bug.
- Its RTC descriptor names the locals adjusted_bounds [ebp-0x14] and adjusted_clip [ebp-0x24].
- A byte flag at [ebp-5] is set by the format switch and then tested.
- HCEX.pdb has no record for bitmap_draw_string.

Probes (full detail in LEDGER.md):
- cand1 (switch-case body with /Od names): gate exact 28 / residual 0.
- cand2 (/Od valid_format flag form): exact 28 / residual 0. The flag is byte-inert and attested by /Od.
- The production safe body stays 240/304 (objdiff 51.18).

Whole-object audit of final_full.obj against a simulated split (see the next unit), all passing:
- 49/49 January symbols strict-equal (coff_compare);
- object_audit PASS;
- surplus is the same 7 literal/real COMDATs production already emits, and provider_link PASS;
- surplus_identity: 0 code COMDATs;
- simulated pdb_storage: 0 disagreements;
- CL /Zs /W3: the same 13 warnings as production;
- fake_match_scan: 0 leads.

objdiff 3.3.1 scores the exact function at 87.83%. It joins the 5 already-accepted hidden_exact draw_string functions, which build/semantic_report.json accepts automatically. The .bss and .rdata sections stay at 100%.

## blockers
Policy, not bytes. The only exact form reproduces January's NULL `bounds` dereference, which is undefined behaviour. The worker brief requires owner approval for any authentic January bug, and the handoffs (opus5_100k consolidated, lane D manifest group J) list this as an owner ruling. No safe expression can produce absolute reads at 0..6 without a relocation.

## reopen_criteria
An owner ruling that admits a UB-preserving BUG (original) body. This run adds new support for such a ruling: the later /Od first-party build has the same inverted test, which is the independent lineage corroboration the playbook's original-bug gate asks for. Once admitted, apply the listed patches mechanically. No further compiler work is needed.

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
