# review_critic_3_profile LEDGER (adversarial review of critic_3 patch 05_profile)

Worktree HEAD 5d3ca708 (task named 7b9de585; 7b9de585..5d3ca708 = one docs ledger only). git status clean except
pre-existing untracked research/. Claimed source/cseries/profile as review_critic_3_profile (verification compiles only).

## R1 pairing (independent of critic_3's tools)
- Raw Sept-2001 cachebeta.map: profile.obj has exactly 11 entries in "Static symbols", one name per address (no ICF
  aliases). Merged by address with the public entries, order and gap-sizes equal January's split order/sizes for all 11
  (0x160,0x40,0x610,0x10,0x50,0x80,0x40,0x30,0xa0,0x30,0x60).
- rawcmp.py: January split bytes vs Sept xbe bytes at the Sept map VA, masking ONLY January's relocation fields:
  0 differing bytes for all 11 pairs (rawcmp.txt).
- callcheck.py: every January E8 relocation to a renamed static (6 edges: dump_frame->describe_frame,
  sections_(de)activate->sections_activation, tick_start/frame_start->internal_step, frame_end->dump_frame) decodes in the
  Sept xbe to the proposed Sept name: ok 6 bad 0. The other statics are inlined at every January site.
- January literals (cachebeta.exe scan): "parent_timesection->self_msec >= child_timesection->elapsed_msec",
  "profile_dump_frames" (hs global), "d:\framedump.txt". Nothing contradicts any proposed name.
- HCEX.pdb 2011: no profile module (profile_initialize absent) -> neither corroborates nor contradicts.
- New names collide with nothing: 0 hits in source/, config/symbols.json, build/split, build/base.

## R2 byte-inertness
- Patch is pure rename: reverse-mapping the candidate reproduces production except the 11 symbol-listing comment lines
  (where `_code_0007e7e0`-style placeholders also disappear).
- Own csplit: split_ctl == build/split 833/833; split_05 differs in source/cseries/profile.obj ONLY; objcmp.py:
  128 sections, 298 symbols identical modulo the 11-name map (bytes, reloc records, storage 3, section, value).
- gate: prod (profile_prod.c vs build/split) 43/1/0; candidate (vs split_05 via with_split) 43/1/0; rows identical modulo
  map; sole residual = parked _compare_profile_sections [size 304!=288, sha] unchanged.
- objcmp prod.obj vs cand.obj: 148 sections / 340 symbols identical modulo map (only .debug$S differs: names).
- Battery prod vs cand: object_audit FAIL(1) both (same _compare_profile_sections DIFF; output identical modulo names);
  pdb_storage 0/0 disagreements; surplus_identity 0 COMDATs; provider_link PASS both orders (3 __real literals).
- objdiff 3.3.1 (sha1 3130e428) own mini report: 8112/9316 code, 42/44 fns, 1131932/1131932 data both sides.
- git apply --check 05 alone at HEAD: OK; series 01,03..21 together (git apply multi-patch): OK.
- Old names referenced elsewhere: none in config/tools/tests (docs history only). parked.json profile entry is
  _compare_profile_sections (not renamed).

## R3 verdict
APPROVE. Zero credit, byte-inert, first-party names attested by map+bytes+call edges. No amendment. Integrator must
regenerate csplit (ninja) in the same landing, else the 11 renamed functions regress (build/split keeps old names).
