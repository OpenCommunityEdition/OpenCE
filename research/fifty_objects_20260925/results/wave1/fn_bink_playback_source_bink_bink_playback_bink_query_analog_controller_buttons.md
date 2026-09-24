# source/bink/bink_playback::_bink_query_analog_controller_buttons

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\L3_q3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\S1_q3_static_bss.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\G2_fc_q3.fn",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\bink_playback\\xbe.py"
]

## production_changes
None to apply unconditionally. The only exact body needs an owner ruling.

If the owner re-rules in favour of the else-if split (the earlier n3a/n5 'q3' candidate), apply:
- C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bink_playback\production.patch. This is a unified diff of source/bink/bink_playback.c (CRLF). It changes only the START/BACK test at about line 1028 from the `||` form to `if (start==1) {button_pressed= TRUE;} else if (back==1) {button_pressed= TRUE;} else {analog loop}`.
- The full candidate file is scratch/w/bink_playback/L3_q3.c. The combined candidate with the storage fix below is S1_q3_static_bss.c.
- Then retire the park config/parked.json entry {unit: source/bink/bink_playback, function: _bink_query_analog_controller_buttons, class: register-allocation}.

If the ruling stays no:
- Keep the park.
- Correct its class and evidence. It is NOT a register-allocation tie: the production `||` body is refuted as January's source (evidence below). The recorded reopen criterion cannot be met with the artifacts we have, because both unoptimized first-party builds contain only a bink stub.

## evidence
Full log: scratch/w/bink_playback/LEDGER.md.

(1) Baseline, gate --all: 23 exact / 1 residual (`_bink_query_analog_controller_buttons [size 96!=80, sha]`). In the production build the constant 1 is pinned in BL, the boolean is spilled to [ebp-1] and an EBP frame appears. January keeps the boolean in BL and compares against immediates.

(2) /Od readout (halo_cache_symbols.exe): bink_playback_active (0x4d19c0) is `xor al,al; ret`, and 0x4d1900..0x4d1a50 contains only stubs. The real PC player at 0x4d1a50 (called from the ending.bik caller 0x61b360) is a Gearbox BinkOpen/D3D loop with no gamepad scan. HCEA also has only bink_playback_stub.obj. So there is no unoptimized witness.

(3) Lab name-count oracle: 0 to 80 dummy names inserted before the definition all stay at 96. Not a TU name-count effect.

(4) Lab mechanism probes:
- Changing only the analog compare to ==2, or only the start/back compares to ==2, frees BL (80 bytes).
- Signed-char compare operands are inert.
- So the contest is the boolean web against the constant-1 web for EBX, and it is decided by a single reference either way.

(5) New primary-artifact evidence: the Oct 12 2001 prototype builds (Downloads/Halo - Combat Evolved (Oct 12, 2001 prototype)/Halo 2276/, read as data only with xbe.py and scan_xbe*.py).
- 2276betaP.xbe file offset 0x1b6100 matches January byte for byte (call rel32 masked).
- 2276P.xbe (identical to oct-default.xbe), file offset 0x14f180, calls the getter with `mov ecx,esi`. A lab __fastcall emulation gives: the `||` form pins (96 bytes, G1_fc_or); the q3 form matches 2276P byte for byte (G2_fc_q3).
- 2276defaultP.xbe VA 0x2c700 inlines the getter. A lab in-TU getter emulation reproduces it exactly, but `||` and q3 compile identically there, so that build is neutral (P1/P2).
- Result: across the four compiled instances of this function, `||` matches 1 of 4 and the else-if split matches 4 of 4.

(6) Whole-object audit of the q3 candidate (L3_q3.obj):
- gate: 24/0/0, _point_from_line3d guard passed.
- object_audit: PASS (59/59 symbols).
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: SELECTED-PROVIDER LINK PASS for all 6 surplus symbols (3 D3D tables and 3 __real literals).
- report.json: data 1701/1701 (100%).
- fake_match_scan: only the owner-approved int 3.

## blockers
1. The owner ruling that rejected q3 is still in force: commit 26601453 and astra research14 hold that 'identical optimized CFG plus allocation does not identify the source arms'. Its reopen criterion (an unoptimized first-party witness of separate START/BACK arms) cannot be met with current artifacts, since both the /Od build and HCEA carry only a bink stub. The new October 2276P/betaP agreement is still allocation-based evidence, so it goes to the owner rather than satisfying the criterion literally.
2. Object-level storage: see the second unit entry. That fix is needed for whole-object completion whatever the ruling.
3. `_code_001b5850` keeps an address-derived name (documented anonymous 16-byte `ret` leaf; exhaustive identity search found no name). It is absent from PDB publics and unreferenced, but making it static makes VC7 drop it, so it must stay external.

## reopen_criteria
Land q3 (production.patch) only on an explicit owner ruling that accepts the cross-build evidence: the `||` form is refuted in two calling contexts, and the else-if split reproduces January, 2276betaP and 2276P exactly. Otherwise, reopen only if a first-party unoptimized build containing the real bink_playback.c body appears (not the PC/HCEA stubs). Do not re-spend on spellings: 10 prior shapes, plus this run's name-count sweep (0 to 80), operand-type probes and context emulations, are all recorded.

## task notes
No slug directory existed, so this run started fresh; prior shapes were taken from the committed ledgers (w2, n3a, n4 and n5 in the opus5-150k worktree) and none were repeated. No tracked file was edited and ninja, configure and git were not run. The Oct 2001 prototype XBEs and halo_cache_symbols.exe were read as data only.

Tools written in the slug directory:
- mkvar.py builds a variant by swapping in a .fn body and gating it.
- namesweep.py is the lab-only name-count oracle.
- disobj.py disassembles one function from an object, with relocations.
- xbe.py, scan_xbe.py and scan_xbe2.py parse XBE sections and sweep for the byte compares.

The October 2276 prototype function locations are recorded in the ledger. They are reusable as a second compilation context: 2276P uses a register-argument call convention and 2276defaultP inlines the getter across modules. Both could discriminate other held allocation rows.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\bink_playback\LEDGER.md
