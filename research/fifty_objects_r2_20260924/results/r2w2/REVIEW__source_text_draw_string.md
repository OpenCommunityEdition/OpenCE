# REVIEW source/text/draw_string

approve: True

amended_patch: 

## per_object
null

## checks
Everything below was run at tree 434f0151, which is clean. The only untracked path is research/fifty_objects_r2_20260924/, and it is not mine. All work is in scratch/w/review_r2w2_draw_string/adv2/ and the review is appended to scratch/w/review_r2w2_draw_string/REVIEW.md.

1. Packet identity: the packet sha256 is 1b93d16a. It is byte-identical to the prior reviewer's PACKET_draw_string_AMENDED.patch.

2. Apply check: `git apply --check` returns 0 for the combined packet, 01 alone and 02 alone.

3. Round trip: I applied the packet to CRLF copies of the working-tree files.
   - draw_string.c becomes blob 984f239a and symbols.json becomes blob 0f677d81, both with consistent CRLF.
   - These files are byte-equal to the candidate I compiled and to the config I used for the emulation.

4. My own csplit emulation (emu2.py): I copied config and applied 02 as an exact one-line replacement.
   - Unmodified control: 833/833 split files are byte-identical to build/split.
   - Patched: only source/text/draw_string.obj differs.
   - The new split's .bss is 248 bytes with flags c0400080. The four names are storage 3 at offsets 0, 8, 0x14 and 0x20.

5. Strict per-function gate (coff_compare.section_infos_equal, fncmp.py):
   - Candidate vs new split: 27/1. Production vs old split: 27/1.
   - The EXACT rows are identical on both the target side and our side (size, sha, relocation count).
   - Cross pairs give 5/23, so both halves must land together.
   - The residual is `_bitmap_draw_string` at 304/240, r9/9, sha 4e8747/ad3d0d. This equals the parked.json measurements, so the park stays valid.

6. .bss relocations: the multiset of (symbol, addend) references in the new split equals the candidate's. Every addend falls on an HCEX field offset.

7. object_audit (a copy that takes the target path from an env var):
   - Candidate vs new split: FAIL(1), `_bitmap_draw_string` only.
   - 49 January symbols, 0 differ. .bss plus 17 .rdata sections are ok.
   - Production vs old split: the same single failure, 46 symbols.

8. pdb_storage (reimplemented against the new split): 1 disagreement goes to 0. None of the five names is a cachebeta public.

9. Surplus:
   - 0 code COMDATs.
   - Candidate-only surplus is identical to production's.
   - provider_link: 7/7 PASS in both orders; `--baseline=prod` finds no new surplus.

10. Cross-references: in build/split, `_draw_string_globals` is defined only in draw_string.obj and nothing references it. A split + base scan and a grep of source/tools/config/docs find no other object or source file that references it or the four new names. No semantic-match, rejection or other config entry names them.

11. objdiff 3.3.1 (sha1 3130e428) mini report: production/old split equals candidate/new split at 92.155975, 3253/7208, 22 functions, data 1056/1056, with identical per-function and per-section rows.

12. /W3: 13 diagnostics in both, identical. fake_match_scan: 0 leads.

13. HCEX: I re-ran DIA2Dump -sym for all four names.
    - All four are DataKind 0x5 File Static in the draw_string compiland (the three `<unnamed-tag>` plus font_drawing_globals).
    - The font_drawing_globals type has Length 0xD8.
    - The member names and types match the packet, including short encoding_shift and multitexture_params at +0x4C.

14. January .rdata literals name `draw_character_software_globals.bitmap->width/height` and `font_drawing_globals.tab_stop_count`.

15. /Od build (read as data only, linear disassembly of the draw_string code range):
    - The statics sit at font 0xf457d8, bounds 0xf458b0, pick 0xf458bc, dcs 0xf458c8. That is exactly the reverse of the packet's declaration order.
    - The only access to encoding_shift is `movsx ecx, word ptr [0xf458cc]`, a short load.

16. Precedents: I read the original a595bbc2 commit and ruling doc, the rejection entry it removed, a9e23674, 7979cf8f, and the 2026-09-15 owner hold. Every condition holds (see issues).

17. My own ablation: I removed the initializer from none, dcs, font or pick in turn and recorded the .bss order and the gate result for each (see issues).

18. Lab only, not landed: 01 plus the round-1 bug body (lab_full_bug_body.c) gives 28/0 and object_audit PASS against the new split.

pytest was not run, because the live config is not patched. The integrator should run it after landing.

## issues
No blocking issues. Approve landing scratch/w/r2w2_draw_string/PACKET_draw_string.patch as-is. It is a zero-credit ownership packet: the unit stays NonMatching and no park, semantic or rejection edits are needed.

**Precedent audit (not stretched)**
- a595bbc2 (models) conditions, each checked here:
  - Authentic names and types: stronger than in models. All four names are HCEX File Static, two also appear in January asserts, and the /Od build shows the declaration order. Models had only one HCEX-confirmed name.
  - Same .bss size and flags, in January's order.
  - No aggregate facade, padding, steering name, custom section or directive. The packet removes the invented aggregate, its two alias #defines, the three pad members and the byte blob.
  - Ablation proves the initializers are the lever.
  - Zero regressions, parks valid, fake scan 0.
- Models never touched symbols.json. The one-row-to-four static split is covered by:
  - a9e23674: the identical construct class (an invented aggregate plus #define alias replaced by HCEA file statics, initialized in declaration order, with "static": true rows).
  - 7979cf8f: rows edited in place to first-party names.
- The 2026-09-15 owner hold was about invented names. Its reopen criterion (authenticated names) is met.
- The only difference from the precedents is that the object stays NonMatching, because of the owner-held NULL-bounds January-bug body of `_bitmap_draw_string`. The packet claims no credit and changes no status, so this does not stretch the ruling.

**Correction of the earlier review record**
- The first reviewer's ledger said all four `= { 0 }` initializers are required. That is wrong.
- My ablation:
  - None initialized: order pick 0, bounds 0xc, dcs 0x18, font 0x20 (hash order).
  - Only dcs uninitialized: January's order. The dcs initializer is inert, as the worker disclosed.
  - Font uninitialized, or pick uninitialized: wrong order.
- Every ablation stays 27/1 on functions. The initializers affect only symbol offsets, which is exactly the models situation.
- The inert dcs initializer carries no bytes, so it is not a strip-test fake. Keeping it gives uniform spelling across the four siblings.

**Optional cosmetic nit (not amended)**
- The packet deletes the `/* ---------- macros */` section header, which becomes empty once the alias macros go.
- Every sibling text/*.c file keeps an empty macros section; 231 of the 328 macros sections board-wide are empty.
- This is comment-only and byte-inert, and it does not warrant a third packet variant.

**Integrator notes**
- Apply both halves in one commit, then do a csplit-only regen; only draw_string.obj changes.
- Then run a full ninja and check that pdb_storage shows 0 disagreements and gate shows 27/1 with the unchanged `_bitmap_draw_string` residual.
- Run pytest.
- `_bitmap_draw_string` stays owner-held in the January-bug class.
