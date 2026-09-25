# LEDGER r2w2_object_lights (round 2 wave 2) - ADMISSION packet 01 + 02 + 03_NO_ALT

Unit: source/objects/object_lights. Tree 05255584 (HEAD 434f0151 = 05255584 + lane ledger only), clean.
Claimed 2026-09-24 as r2w2_object_lights (claim.py exit 0).
Read: WORKER_BRIEF_R2 0-9; round-1 owner_queue/objlights_render_debug APPLY_ORDER/RULING/MEASURED/LEDGER;
canonical ae12a3c1 reconciliation doc (leaf_map allow_incomplete_unit precedent); 05255584 commit message.

## S0 setup
- Tools copied into tools/ (each read before use): round-1 tgate.py (shadow-tree gate; EXTRA_CFLAGS variant),
  objcmp.py, semsnap.py, verify_semantic_entry.py, mkproj.py (output path re-pointed to this slug), splitcmp.py;
  with_split.py (from r2w2_rasterizer_xbox_debug; open() redirect of build/split + one build/base obj) at slug root.
- tree/source = cp -r of source/ at 434f0151 (clean). cfg_ctl/config and cfg_OL/config = cp -r of config/.

## S1 packet re-application at 05255584
- git apply --check (repo root) 01, 02, 03_NO_ALT: all CLEAN. object_lights.c, object_lights_rendering.h, objects.h
  untouched since cdc8ebd3 (0 commits); symbols.json (8 commits) and semantic_data_matches.json (3) moved but the
  patched rows / the tail context (units entry) are unchanged.
- Applied with `git apply --directory=<slug>/tree` (01) and `--directory=<slug>/cfg_OL` (02, 03_NO_ALT). diff -r vs
  the real tree: exactly the patch hunks (object_lights.c 9 hunks, object_lights_rendering.h -2 lines, symbols.json
  4 rows, semantic_data_matches.json +1 entry). git status unchanged (only the pre-existing untracked research dir).
- Reference census (tools/refcensus.py): _should_render_lights/_light_attenuation/_cluster_get_first_light/
  _cluster_get_next_light are DEFINED only by object_lights (split and base) and referenced by NO other object.
  Source grep: no other .c/.h uses any of the four (only the header prototype that 01 removes).
- cachebeta publics: none of the four is public (lights_initialize/render_diffuse/render_specular are, as controls).
  pdb_storage production: 71 split symbols, 4 disagreements = exactly these four.

## S2 split emulation (build/tools/csplit.exe -i cachebeta.exe -p <cfg>/config -o <split>)
- split_ctl (verbatim config copy) vs build/split: 833/833 identical -> build/split current, emulation faithful.
- split_OL (02 applied) vs split_ctl: exactly 1 object differs = source/objects/object_lights.obj.

## S3 gate / audit
- Control: gate.py production vs build/split 43/43 EXACT; production recompile objcmp vs build/base IDENTICAL.
- Candidate (tgate shadow tree, SPLIT_ROOT=split_OL): 43/43 EXACT (gate_cand_vs_splitOL.txt). Only warning = the
  pre-existing C4133 (production line 853 -> 855, +2 prototype lines).
- Candidate vs unmodified build/split: 43/43 (storage is not part of section_infos_equal); production vs split_OL 43/43.
- objcmp candidate vs production: every section IDENTICAL; ONLY the 4 symbol storage rows differ (3 vs 2).
- object_audit candidate vs split_OL: PASS, 71 January symbols, 0 differ (all 65 January sections ok).
  Controls: production vs split_OL FAIL(4) (storage 3/2); candidate vs build/split FAIL(4) (2/3)
  => 01 and 02 must land together (plus csplit regen).

## S4 /Od attestation of the should_render_lights() calls (halo_cache_symbols.exe, data only)
- /Od should_render_lights = 0x78dab0 (68 B): reads [lights_game_globals 0xded68c]->render_lights, calls
  game_engine_allow_dynamic_lighting (0x59b3a0), returns a boolean via a local. Only callers: 0x78c6eb in
  0x78c6c0 and 0x78c97b in 0x78c950.
- 0x78c6c0 = lights_render_diffuse: call diffuse_lights_begin (0x825320) -> call should_render_lights -> je skip ->
  short loop counter over [0xded6a0] -> ... -> diffuse_lights_end (0x825340). 0x78c950 = lights_render_specular:
  same with 0x825a30 / 0x825a50. Same statement order as our source.
- January double-attestation: January object_lights DEFINES _should_render_lights (32 B, non-public) while no
  relocation anywhere names it; lights_render_diffuse/_specular reference _game_engine_allow_dynamic_lighting
  directly (inlined at both sites).
- LAB strip test (lab/tree_nocall: 01 with the two call sites reverted to the expanded expression):
  42/43 + _should_render_lights UNWRITTEN => VC7 emits the static only because the source USES it; the calls are
  load-bearing for a January-owned section, and the expanded expression was a hand-expanded helper.

## S5 header consumers (claimed source/render/render + render_objects as r2w2_object_lights for the window; released)
- object_lights_rendering.h is included only by object_lights.c, render.c, render_objects.c (no header includes it).
- Shadow-tree compile with 01's header: render 13/13, render_objects 22/22; objcmp vs build/base IDENTICAL (both).

## S6 audits (with_split.py redirect -> split_OL; candidate object redirected for build/base where the tool reads it)
- pdb_storage: candidate 71 split symbols / 0 disagreements (production 4 = the four helpers).
- HCEX.pdb (2011, corroboration only): cluster_get_first_light / cluster_get_next_light `static function: true`
  ("static long cluster_get_first_light(long *, short)"); should_render_lights / light_attenuation have no HCEX
  symbol (fully inlined there); lights_render_diffuse (control) is not static.
- cachebeta publics neighbourhood: 0x128A10 (_should_render_lights, file_offset 1214992) sits between public
  _sample_diffuse_texture 0x128920 and _lights_initialize 0x128A30 with no public row -> file-static.
- surplus_identity: 12 candidate-only code COMDATs, 11 IDENTICAL; _object_get_bounding_sphere DIFFERENT vs
  action_vehicle (held Q1; unchanged from production: 144/144, sha equal, 9/9 relocs, only DIR32 @37/@76 name
  "c:\halo\source\objects\objects.h" vs "..\objects\objects.h"). _distance3d IDENTICAL to action_vehicle (no
  distance3d-class conflict).
- provider_link: SELECTED-PROVIDER LINK PASS, 33 surplus names, both orders. --baseline=production: no new surplus.
- fake_match_scan (candidate .c/.h and production): 0 leads. /W3 and /W4 warning census identical to production
  (only the pre-existing C4133, line 853 -> 855).
- gate.py --source cand.c (production header on the include path, extern prototype + static definition) also 43/43
  and objcmp IDENTICAL to the shadow-tree object; the packet removes that prototype, so no extern-then-static
  construct lands.

## S7 data
- object_audit: .data sym:_lights_section 1541/1541 ok (flags c0400040 eq); .bss/.rdata all ok.
- Symbols in .data: _lights_section static @0, four public _object_light_* @1528/1532/1536/1540 (target == cand).
- Single relocation: target 'symbol' import of ??_C@_0O@JDONOBDH@render_lights?$AA@ (hs selects it) vs ours
  'defined-noncode' (the '$'-literal class); semsnap (section_info_resolved with cfg_OL symbols): size 1541,
  relocs 1, sha 527b43a1..., resolved infos EQUAL (also with production symbols/objects).
- build/report.json (09:24): object_lights data 1104/2645, .data 99.7411% -> unmatched 1541 == the entry size.
- Mini objdiff 3.3.1 (sha1 3130e428) proj_cand (cand vs split_OL) and proj_prod: code 12294/12294, 43/43,
  data 1104/2645 (identical). verify_semantic_entry (tools.semantic_progress.apply_semantic_data_matches):
  03_NO_ALT entry, unit incomplete -> +1541, data 2645/2645. Negative control (same entry minus the opt-in) ->
  SemanticProgressError "not marked complete" (fail-closed).
- tools/audit_object_admission.py: production 9/0/2/0; with 03_NO_ALT (+cfg_OL symbols) 10/0/2/0 - object_lights
  joins the review queue (audit-coff-ownership-before-admission) like render_debug/rasterizer_xbox/game_engine.
  Precedent check: ae12a3c1 added a rejection entry for leaf_map because it DOWNGRADED a Matching label; the
  allow_incomplete_unit entries for objects (84414a1a) and editor_flying_camera (5019c186, NonMatching at the
  time) carry no rejection entry -> a rejection entry is not a condition of the data-entry precedent, and adding
  one would pre-empt the open owner question Q1. Not added.

## S8 packaging
- patches/ regenerated at HEAD with tools/mkpatch.py (LF, git index lines from `git diff --no-index` of LF blobs):
  01 (2553c63c..9e77ec23, c863833f..527cc0b3), 02 (f9abb17d..e6ea96b2), 03_NO_ALT (77814b3d..3f1a813e).
  Hunk bodies identical to round-1 patches. git apply --check at repo root: each alone and the set in one call
  CLEAN. Applied one at a time into verify/ copies: post-images byte-equal to tree/ and cfg_OL/.
- git status throughout: only the pre-existing untracked research/fifty_objects_r2_20260924/.

## S9 close
- MANIFEST.md written (precedent section P1 cachebeta storage / P2 leaf_map allow_incomplete_unit, each condition
  shown holding). Verdict: ADMISSION_FIX_PROPOSED (zero code credit, +1,541 data; object stays NonMatching on Q1).
- Trimmed slug: removed split_ctl (== build/split), verify/ (post-image check done), lab/tree_nocall (kept its .c as
  lab/object_lights_nocall_striptest.c). Kept tree/, cfg_OL/, split_OL/ as the measured evidence.
- Claim released.
