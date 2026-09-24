# LEDGER owner_queue/pick_hwgeom_models (wave 5, base cdc8ebd3)

Items: (1) rasterizer_xbox_hardware_geometry 0x158460 wrapper spelling A (IDirect3DResource8_MoveResourceMemory)
vs B (D3DVertexBuffer_MoveResourceMemory); (2) models .bss: P1 file-scope `= {0}` statics vs first-party static-local form.
Nothing lands. Sources: scratch/w/hardware_geometry/ (wave 2, base 931ed8dc), scratch/w/models_bss/ (wave 3, base 954eebd2).

## S0 2026-09-24 startup
- git status clean at cdc8ebd3; read WORKER_BRIEF (sections 0, 9, 10) and held-items table.
- Neither prerequisite packet has landed at cdc8ebd3: hardware_geometry.c still carries the 6 hand stubs
  (code_00158450..code_00158510); models.c still has 4 uninit file statics, public render_model_section and a
  public tangent helper (C1 not landed).

## S1 hardware_geometry rebase check (cdc8ebd3)
- source blob at cdc8ebd3 = c2bb9095 = the wave-2 patch base (file unchanged since 931ed8dc); symbols.json rows 5813-5825
  unchanged at the same line numbers. `git apply --check` of wave-2 hardware_geometry_{A,B}.c.patch + symbols_{A,B}.json.patch:
  clean. Applied (session scratchpad copies): post-images == wave-2 final/rasterizer_xbox_hardware_geometry.{A,B}.c and ==
  this slug's cfg_hg{A,B}/symbols.json (tools/mk_cfg.py, 13 in-place line edits each; A/B differ in line 5816 only).
- REAL CHANGE NEEDED FOR REBASE: none. Only the symbols patch's stale `index d0a16b3a..` header is refreshed.
## S2 csplit emulation (build/tools/csplit.exe -i cachebeta.exe -p cfg_X -o split_X, all under this slug)
- split_ctl (unmodified config copy) == build/split byte-for-byte (833/833) -> build/split is current for cdc8ebd3.
- split_hgA vs split_ctl: ONLY source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.obj differs; same for split_hgB.
- split_hgA vs split_hgB (objeq2): differ ONLY in the 0x158460 symbol name.
## S3 measurements (tools/gate_split.py = repo gate.py + --split-root/--tmpdir; object_audit_split.py = repo audit + AUDIT_SPLIT_ROOT)
- production vs build/split: gate 17/17 EXACT; object_audit FAIL(7) (7 wrapper rows static in ours, external in symbols.json);
  pdb_storage 13 disagreements (7 wrappers + 6 _code_ hand stubs; none is a cachebeta public).
- A vs split_hgA: gate 17/17 EXACT (emitted-symbol guard _point_from_line3d passed); object_audit PASS (25 symbols, 0 differ);
  pdb_storage_cand 0 disagreements; surplus_identity 0 candidate-only code COMDATs; provider_link PASS (6 rows: 3 pooled
  literals + 3 SDK tables, all pre-existing in production); fake_match_scan 0 leads.
- B vs split_hgB: identical results (17/17, PASS 25/0, pdb 0, 0 code COMDATs, link PASS, scan 0).
- cand_A vs cand_B: every Halo section byte+reloc identical (vertex_buffer_new 5442bf32, triangle_buffer_new e90e066b,
  deletes 1d9f1c5d/5075d6eb); both MRM bodies sha 2a2f950f (pure ret 8); same .text emission order. A/B differ ONLY in one name.

## S4 models rebase check (cdc8ebd3)
- models.c e5147d6e / models.h 12ce3738 unchanged since 954eebd2; symbols rows 4771/22672/23095-23098 at the same lines.
- wave-3 C1_models.c.patch + C1_models.h.patch: `git apply --check` clean; post-images == models_bss/new/models.{c,h}.
  OWNER_OPTION_P1 applies on top of C1 (fails standalone, by design); post-image == new/models_owner_option_P1.c.
  C1_symbols.json.diff is a plain `diff` (not git-applicable) -> regenerated as a unified patch (cfg_mC1, 2 in-place edits).
- REAL CHANGE NEEDED FOR REBASE: none.
- Option SL (first-party static-local form) = wave-3 lab L1 rebased on C1 (tools/mk_models_sl.py): the four defaults as
  UNINITIALISED static locals inside their own `if (!param)` blocks, names unchanged; symbols.json renames the 4 .bss rows
  to the VC7 names read from cand_SL.obj (?name@?7/?8/?9/?L@??render_model@@9@9). Variant P2 = wave-3 lab P2 rebased on C1
  (fv an uninit static local at render_model function scope = HCEX `Static Local`; the other three file statics `= {0}`),
  symbols.json renames fv to ?default_function_values@?1??render_model@@9@9. Diff vs wave-3 labs = the C1 hunks only.
## S5 models csplit + measurements (split_mC1 / split_mSL / split_mP2 each differ from split_ctl ONLY in models.obj)
- production vs build/split: gate 12/12; object_audit FAIL(4) (.bss offsets fv 72/change 88/effect 32/region 0 vs January
  0/16/64/104); pdb_storage 2 disagreements (tangent helper, render_model_section).
- C1  vs split_mC1: gate 12/12; audit FAIL(4) (.bss offsets only); pdb 0; surplus 2 code COMDATs IDENTICAL
  (_set_real_point3d, _negate_vector3d, pre-existing); provider_link PASS (11); fake scan 0.
- P1  vs split_mC1: gate 12/12; audit PASS (30 symbols, 0 differ); pdb 0; surplus IDENTICAL; link PASS; scan 0.
- SL  vs split_mSL: gate 12/12; audit FAIL(4) (fv 32/change 48/effect 96/region 0 = wave-3 L1 exactly); pdb 0; link PASS; scan 0.
- P2  vs split_mP2: gate 12/12; audit PASS (0 differ); pdb 0; surplus IDENTICAL; link PASS; scan 0.
- _render_model bytes identical in all four (2144 B, sha bb666c87, 98 relocs).
- models.h C1 shared-header sweep (tools/hdr_sweep.py; shadow header in the gate tmpdir, `#error` control FAILED the
  compile of every TU = shadow used): first_person_weapons 33/34 (same residual), model_animations 32/32, objects 121/121,
  render_objects 22/22, render_sky 1/1 -> gate rows equal and objeq2 IDENTICAL cur vs C1 header for all five consumers;
  models.c differs only in the 2 intended storage rows (+ tangent emission deferral = January order).

## S6 report-level (objdiff 3.3.1 one-shot mini project objdiff_proj/, targets = emulated splits, bases = candidates)
- hwgeom prod/A/B: code 898/898, fn 17/17, data 584/584 (.rdata/.text 100). models prod/C1/P1/SL/P2: code 5038/5038,
  fn 12/12, data 2368/2368 (.bss/.data/.rdata/.text 100). => every option is credit-neutral; only admission changes.
- tools/audit_object_admission.py (read-only, no --output) at cdc8ebd3: hardware_geometry = candidate
  "audit-coff-ownership-before-admission" (17/0, 584/0); models = "object-admission-rejected" (bss-symbol-layout-mismatch).
  No parks / semantic entries for either unit.
## S7 patches (tools/mk_patches.py; all under patches/)
- hwgeom_A, hwgeom_B: 01 source (wave-2, unchanged), 02 symbols.json (wave-2, index line refreshed a4e82fec..), 03 config.json
  status NonMatching->Matching. hwgeom_fallback_hg0: wave-2 hg0 (index refreshed); re-measured at cdc8ebd3 vs split_hg0:
  gate 17/17, audit PASS, provider_link PASS, pdb 2 disagreements (the two _code_ hand stubs stay external) -> not admissible.
- models packets are STANDALONE vs cdc8ebd3 (first attempt stacked option-on-C1 patches: `git apply --check` of a stacked
  models.c pair fails in-repo under core.autocrlf although sequential application works -> rebuilt as one patch per file):
  models_C1 (fallback, zero credit), models_optP1 (C1 + `= {0}` + admission), models_optSL (C1 + in-block static locals +
  4 symbols renames; no admission), models_optP2 (C1 + fv static local + 3 `= {0}` + fv rename + admission).
- tools/verify_apply.py: every packet and 4 hwgeom+models combinations: repo `git apply --check` OK, sequential apply in a
  scratchpad copy OK, post-images byte-identical to the measured candidate files / config copies. ALL OK.
- combined symbols.json (A + C1 rows, 15 lines) csplit: differs from split_hgA only in models.obj and from split_mC1 only
  in hardware_geometry.obj -> the two items are independent.

## S8 deliverables (2026-09-24)
- patches/APPLY_ORDER.txt, MEASURED.md, RULING.md written. Precedent checks for RULING: player_ui
  `static char player1_profile_path[0x100] = { 0 };` landed in f7cd2e72; rasterizer_lights `= {0}` in cdc8ebd3;
  69 column-0 `static ... = {0};` definitions in source/ at cdc8ebd3; in-tree `IDirect3DResource8_Release((D3DResource *)...)`
  at progress_bar.c:1812. No other file/config references the removed `code_001584xx` stubs or the renamed .bss names;
  no tests pin either unit's status or the rejection count.
- Side effect outside the slug (by design of the tool): one run of the repo tools/campaign/gate.py wrote
  scratch/_gate_<pid>.c/.obj (the tool's fixed tmp path); left in place per incident rules (no rm outside slug).
- VERDICT item 1: APPLY-READY both A and B (each completes rasterizer_xbox_hardware_geometry); hg0 fallback ready.
- VERDICT item 2: APPLY-READY P1 and P2 (each completes models), SL and C1 ready (object stays blocked, FAIL(4)).
