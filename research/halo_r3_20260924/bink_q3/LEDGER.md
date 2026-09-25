# LEDGER r3w3_retail_bink_playback__bink_query_analog_controller_button

Unit source/bink/bink_playback, function _bink_query_analog_controller_buttons (January 80 B / 1 reloc / c88d131b).
Tree: HEAD 6c44a151 (= b78b76c0 + ledger-only commits), git status clean at start. Claimed with this label (exit 0).
No tracked edits; no ninja/configure/git mutation. Retail harness COPIED from scratch/w/r3w3_retail (sha1 prefixes
retail_gate 43a25787, labcc 4a52ce05, octw a9bc1460, rw 69efddf4, mkshim d626edf0) into ./retail/ with HERE re-pointed
at ./retail so every lab temp/obj lands under this slug. First-party images are read as DATA only.

## Step 0: read-first (no compile)
- Brief sections 0-9 read. Prior negatives (do NOT repeat): w2 q1 (inner `&& !button_pressed`), q9/q10 cond order,
  q4 comma init, q5 setcc; n3a q1 decl order, q2 gamepad hoist; n5 qA outer cond swap (loop rotation inverts), qB
  button_index hoist; round-1 (claude-fifty-objects-20260925) TU name-count 0..80 inert, L1/L2 semantic ==2 labs,
  L4/L5 operand signedness inert, P1/P2 defaultP context (getter inlined) neutral, G1/G2 extern __fastcall emulation
  (|| 96 B EBP frame; q3 == 2276P 0x14f180 file).
- Owner hold: commit 26601453 "the rejected _bink_query_analog_controller_buttons, whose edges share one block"
  (identical-fake-branch prohibition; laws_w3 A26 admits split arms only with TWO DISTINCT rejection blocks).
  Recorded reopen criterion: an unoptimized (/Od) witness of separate START/BACK arms - unsatisfiable on hand
  (/Od halo_cache_symbols and HCEX carry only bink stubs).
- r2 critic packet (bink_decompress_audio_frame naming) is held by canonical as an unattested ADDED call (brief s8).
- Cross-build census (r3w2): January body verbatim in Oct 2276betaP 0x1c6100, Sept cachebeta 0x1c4430, Aug 0x1aa310;
  TU-robust in sep (1 other fn changed) and aug (10 other fns changed): the allocation is stable under TU drift.
- r3w3_retail candgate.jsonl: production (|| form) retail 96 B, NOT found; q3 (L3_q3.c) debug EXACT, retail 80 B
  found verbatim at 2276P VA 0x15f180.

## Step 1: debug + retail re-gate at HEAD (own harness copy)
- prod.c = source/bink/bink_playback.c (HEAD); q3.c = prod.c with ONLY the START/BACK `||` split into
  `if (start==1) {p=TRUE;} else if (back==1) {p=TRUE;}` (CRLF preserved; diff = 2 lines -> 5 lines).
- DEBUG gate.py --all: prod 23/1/0 (`_bink_query_analog_controller_buttons [size 96!=80, sha]`); q3 24/0/0.
- RETAIL (retail_gate.py, calibrated config, /Gr FPO no-DEBUG shim):
  prod: 96 B, OCT=none (not in 2276P), SEP=short-image (Sept-25 retail cache.exe map body
        @bink_query_analog_controller_buttons@0 [Static symbols, bink_playback.obj] VA 0x54da60 is only 80 B).
        Mechanism identical to debug: `mov bl,1` constant pin, boolean homed at [esp+0xb], `push ecx` slot.
  q3:   80 B, OCT=0x15f180 (2276P verbatim, reloc-masked full body), SEP=MATCH at 0x54da60 with 0 relocation-name
        mismatches (call -> @input_get_gamepad_state@4).
  NEW vs round 1 / r3w3_retail: the Sept-25-2001 retail cache.exe (a SECOND retail image, map-located, independent
  of the octw byte search) also matches q3 and rejects the || form. q3 now reproduces January, 2276betaP,
  Sept cachebeta, Aug (debug images, via January==those) AND 2276P + Sept retail (retail images); || matches none.
- TU calibration in this run (other functions, production source): OCT unique 11 + ambiguous 4 of 21 src-stable E rows;
  SEP MATCH 17/24 (misses = debug-only code: draw_frame/start/render short-image, dispose/initialize DIFF).

## Step 2: CONTROL on OTHER functions - the same spelling family (identical else-if arms vs merged ||)
- Hypothesis (falsifiable): the retail view discriminates "identical adjacent else-if arms" vs "one `A || B` arm" the
  same way the debug view does. Prediction: at production-EXACT sites whose production body is retail-verbatim
  (calibrated), merging a January split pair into || is rejected by retail exactly where debug rejects it, and inert
  where debug is inert. It fails if retail accepts merges debug rejects (or vice versa) at a material rate.
- duparms.py: text scan of source/**/*.c for `if (A) X else if (B) X` with identical normalised X, joined to
  scratch/baseline/stable.json status: 50 sites, 46 in production-EXACT functions (skip libtiff vendored, skip
  reserved path_structure_bsp). dupctl.py: ONE variant per site (merge that pair only), gate.py debug --all +
  retail_gate.py (production AND variant) -> dupctl.jsonl / dupctl.log. No other edits, no sweeps.
- RESULT (dupctl_classified.json; retail "calibrated" = production body verbatim in 2276P or Sept MATCH):
    debug REJECTS merge & retail REJECTS merge: 14 site-variants / 11 fns (action_converse_perform, actor_action_handle_evasion,
      actor_perception_desire_prop, game_engine_should_spawn_player x4, can_touch_team, game_time_update, queue_event x2,
      progress_bar_render, render_ui_widgets_postgame, weapon_can_be_fired)
    debug inert & retail inert: 8 (actor_compute_prop_unopposable x2, elite_decide_action, actor_type_flood_desire_shamble x4,
      extract_find_bitmap_bounds)
    debug rejects, retail inert: 1 (actor_type_flood_desire_shamble:208)      debug inert, retail rejects: 0
    uncalibrated in retail: 19 (16 debug-reject, 3 debug-inert) - uninformative for retail.
  => retail agrees with debug on 22/23 calibrated site-variants of this exact spelling family; prediction HELD
     (one retail-inert case, same class as r3w3_retail's 3/15 retail-inert negative controls). Both views are
     sensitive to "identical arms vs ||" at other first-party sites, so their joint acceptance of q3 is informative.
- Idiom census (no compile): canonical production already spells 46 identical adjacent else-if arm pairs in
  production-EXACT functions (26 fns); 30 of them are REQUIRED by debug bytes (merge rejected).

## Step 3: /Od CALIBRATION of the target's evidence class (allocation-only, arms share ONE block)
- dupctl alndiff (dupctl/*.alndiff.txt; dupctl_cfg.json): action_converse_perform:124 is the SAME class as the target:
  January 89 insns == merged 89; the in_range store `mov byte ptr [esi+0xa1],bl` exists ONCE (jnp 0x93 -> 0xa8 and
  fall-through from jp 0xa6: two in-edges, one block); the ONLY difference is the constant-1 EBX pin (`mov ebx,1`,
  push ebx x2, bl stores) vs immediates. Retail: production 2276P 0x12f10 verbatim; merged variant not found.
- /Od ground truth (independently regenerated: odbuild.py fn 0x432bf0 -> od_action_converse_perform.txt): TWO distinct
  stores `mov byte ptr [edx+5],1` at 0x432cce (then `jmp 0x432cef`) and `mov byte ptr [ecx+5],1` at 0x432ceb.
  Canonical admitted it on exactly that /Od attestation (d57ba30c, Lane B wave 2, 2026-09-21; manifest
  claude_lane_b_admission_manifest_20260920.json).
  => At the one site where /Od truth exists for this evidence class, "both optimized views reject || while the CFG
     shares one block" predicted the /Od split correctly. The target has the same optimized-code evidence in 6 images
     (January, 2276betaP, Sept cachebeta, Aug = debug; 2276P, Sept retail = retail) but no /Od body (stubs only).

## Step 4: whole-object audit of the q3 candidate at HEAD (section 4)
- gate --all: 24/0/0 (gate_q3.txt). object_audit.py: OBJECT AUDIT PASS, 59 January symbols 0 differ (audit_q3.txt);
  production FAIL(1) = the query row only (audit_prod.txt).
- pdb_storage (production symbols; q3 changes no symbol): 1 disagreement `_code_001b5850` split 2 / PDB-public False
  -> the held r2 naming packet (bink_decompress_audio_frame static + added call; brief s8 "byte-inert ADDED calls whose
  position is unattested" held by canonical). Not touched here.
- surplus_identity 0 candidate-only code COMDATs; provider_link q3.obj: SELECTED-PROVIDER LINK PASS (6 surplus rows,
  both orders); --baseline=prod.obj: no new surplus. fake_match_scan: 1 pre-existing lead (`__asm { int 3 }`,
  owner-approved) in both prod and q3. CL /Zs /W3: 14 == 14 warnings, identical multiset.
- prod.obj vs q3.obj: only differences are the query .text (96 -> 80), one extra .debug$F FPO record (frameless
  function, as January) and .debug$S path length; all other sections byte-identical; symbol table shifts by that
  one section only.

## Step 5: target-side controls in BOTH views - every RECORDED shape re-measured (not new spellings)
- transplant.py: the function BODY of each recorded shape file (w2 q1/q4/q5/q9/q10, n3a q1/q2/q3, n5 qA/qB, w19 c1..c4;
  donors in opus5-150k-house-clean-20260914 and opus5-throughput-recovery-20260916 scratch) spliced into HEAD prod.c.
- Prediction (falsifiable): the retail view accepts ONLY q3; it fails if retail accepts any debug-rejected shape.
- Result (shapes_both_views.txt): debug reproduces every recorded verdict (q3 EXACT; q1/q5/q9/qA sha-only 80 B; the
  other 9 size 96). RETAIL: q3 OCT=0x15f180 + SEP=MATCH; ALL 13 others OCT=none, SEP=DIFF (the four 80 B shapes) or
  short-image (96 B). Production || also rejected. => q3 is the unique measured spelling accepted by either view,
  and the two views agree on all 15 measured bodies. PREDICTION HELD.
- Invariance (lab only, not landable): q3 with `gamepad` -> `controller_state`, `button_index` ->
  `analog_button_index`: debug EXACT and retail OCT 0x15f180 + SEP MATCH for both - the q3 acceptance is not a
  name-count coincidence in either view.

## Step 6: packet + close
- patches/01_q3_source.patch + 02_parked_retire.patch; git apply --check (worktree, --cached) clean; post-images verified.
- records.json (1 record, OWNER_RULING_PACKET, zero credit), MANIFEST.md written. git status: clean (no tracked edit).
- Claim released with outcome.
