# LEDGER bink_playback (_bink_query_analog_controller_buttons)

## Context read (2026-09-25 run)
- Baseline gate: 23 exact / 1 residual: `_bink_query_analog_controller_buttons [size 96!=80, sha]`. bink_alloc (int3 ruling) and draw_frame already landed.
- Prior history: w2 (5 shapes: q1 && !pressed inner, q9/q10 cond order, q4 comma init, q5 setcc), n3a (q1 decl-order inert, q2 gamepad hoist inert, q3 else-if split EXACT 80/1/c88d131b), n5 (qA outer cond order swap -> sha, loop rotation inverted; qB inner short hoist inert).
- q3 (separate START/BACK arms each assigning TRUE) was REJECTED (commit 26601453: "rejected _bink_query_analog_controller_buttons, whose edges share one block"; astra research14 scout). Reopen criterion: an independently correlated unoptimized (/Od) implementation shows two separate START/BACK assignment arms. Astra scout did not find an /Od body (HCEA has only bink_playback_stub.obj).
- DO NOT repeat: any of the above spellings. New avenue this run: halo_cache_symbols.exe /Od build (odbuild.py) readout.

## Probe 0 (evidence, no compile): /Od build readout
- odbuild.py file "bink_playback" -> nothing; str "!bink_playback_active()" -> scenario.c caller 0x868860 calls bink_playback_active = 0x4d19c0 = `xor al,al; ret` (STUB).
- Region 0x4d1900..0x4d1a50 in halo_cache_symbols.exe is all empty/FALSE-returning stubs (bink_playback_stub-like). The /Od build has NO query body. Same as HCEA (bink_playback_stub.obj). => no /Od witness possible for the START/BACK arm structure.

## Probe 1 (LAB oracle, never landable): TU name-count sweep
- Hypothesis: EBX contest decided by a web arena-ordinal (tu-name-count-oracle law).
- Method: namesweep.py inserts n `extern long` names immediately before the definition, n=0..80.
- Result: all 81 variants residual 96 [size 96!=80, sha]. NEGATIVE: not a name-count tie (at least n<=80 in the span).

## Probes L1-L3 (LAB mechanism, semantic changes, never landable) via mkvar.py
- L1 analog compare `==2`: size 80, sha only (constant differs) => pin gone, boolean takes BL.
- L2 start/back `==2`: size 80, sha only => pin gone.
- L3 = q3 else-if split re-gated at this HEAD: EXACT 80 (whole TU 24/0). Reproduces n3a.
- Reading: the constant-1 pin is on a knife edge; removing ANY one constant-1 compare use flips it (so does adding one boolean assignment site). Contest = EBX between boolean web and constant-1 web.

## Probe 0b (evidence, no compile): PC /Od movie path
- /Od build imports binkw32.dll (_BinkOpen@8). ending.bik caller 0x61b360 -> movie player fn 0x4d1a50 (0x4d1a50..0x4d1efe, dumped to od_4d1a50.txt), located right after the FALSE-returning bink stubs.
- It is a Gearbox PC D3D/DirectSound blocking player (BinkOpen/BinkDoFrame/BinkCopyToBuffer via IAT, D3D vtable calls). NO gamepad scan, no input_get_gamepad_state call. Not a witness for the query arms.

## Probes L4-L5 (LAB mechanism): compare operand type
- Hypothesis: if the compares' narrowed constant had a different type (signed char) than the boolean TRUE store, the constant-1 web would split and lose EBX.
- L4 all compares via `(char const *)` cast; L5 analog only. Both: 96 [size 96!=80, sha] — pin unchanged. NEGATIVE: constant web is not split by operand signedness. (Also kills "January's buttons[] was signed char" as a lever.)

## Probe 0c (evidence, DATA only): October 2001 prototype builds (Halo 2276), C:/Users/isabe/Downloads/Halo - Combat Evolved (Oct 12, 2001 prototype)/Halo 2276/
- xbe.py (section map) + scan_xbe.py/scan_xbe2.py (capstone sweep for byte compares at +0x1c/+0x1d).
- 2276betaP.xbe VA 0x1c6100 (file 0x1b6100): byte-identical shape to January (push ebx/esi, BL boolean, immediates, one shared `mov bl,1`).
- 2276P.xbe / default.xbe (file 0x14f180): same allocation (BL boolean, immediate compares), but getter called `mov ecx,esi; call` (fastcall-style build).
- 2276defaultP.xbe VA 0x2c700..0x2c76e: SAME FUNCTION compiled with input_get_gamepad_state INLINED (valid-dword table 0x276a4c, override flag 0x276a48 -> 0x276afc, else &states[i] stride 0x28 @0x276a5c). There the constant 1 IS pinned in BL (`mov bl,1` preheader, `cmp [edx+1c],bl`, `cmp [edx+1d],bl`, `cmp [edi+edx+10],bl`) and the boolean lives in AL (no call => no callee-saved need); all three je go to ONE `mov al,bl` block. CFG again does not separate START/BACK arms by itself.
- New idea: defaultP is a SECOND compilation context of the same source. If || and else-if compile DIFFERENTLY in that context, defaultP's bytes discriminate the source arms (independent witness). Test by lab emulation (in-TU copy of the getter, release flags) — oracle only.

## Probes P1-P2 (LAB oracle): emulate 2276defaultP context (in-TU static copy of the getter, no assert; auto-inlined under /O2)
- P1 (|| form, production body) and P2 (q3 else-if) both compile to code IDENTICAL IN SHAPE to defaultP 0x2c700..0x2c76e (push ebx/esi, xor al,al, push edi, mov bl,1, lea esp,[esp] pad, inlined getter, cmp ..,bl x3, shared `mov al,bl`, pop edi/esi/ebx). Validates that defaultP = same source + same compiler with getter inlined.
- But P1 == P2 byte-for-byte: the defaultP context does NOT discriminate || vs else-if. Negative for the witness search in this build.

## Probes G1-G2 (LAB oracle): emulate 2276P/oct-default.xbe context (getter reached by register-arg call `mov ecx,esi; call`)
- (/Gr globally fails on cseries.h redeclarations; emulated with an extern `__fastcall` lab getter instead.)
- G1 (|| form): 96 bytes, EBP frame, constant 1 pinned in BL, boolean at [ebp-1] — does NOT match 2276P.
- G2 (q3 else-if): 80 bytes, byte-identical to 2276P.xbe file 0x14f180..0x14f1c3 (call rel32 masked). oct-default.xbe == 2276P.xbe (same file).
- 2276betaP 0x1b6100 == January bytes (call rel32 masked).
- Summary of the four compiled instances of this function: January (cdecl), 2276betaP (cdecl), 2276P (register-arg call), 2276defaultP (getter inlined).
  || form matches 1/4 (defaultP only); q3 else-if matches 4/4. Still an allocation-based discriminator (EBX boolean-vs-constant-1), NOT the recorded reopen criterion (an unoptimized witness of separate START/BACK arms). defaultP, the only context with a different CFG/IL (inlined getter), is neutral.

## Whole-object audit on the q3 candidate (L3_q3.c / L3_q3.obj)
- gate --all --forbid-emitted-symbol _point_from_line3d: 24/0/0, guard passed.
- object_audit.py: OBJECT AUDIT PASS (59 January symbols, 0 differ). Surplus: .drectve, D3DPRIMITIVETOVERTEXCOUNT, D3DSIMPLERENDERSTATEENCODE, D3DTEXTUREDIRECTENCODE, __real@00000000/3f800000/4f800000 (same as production).
- surplus_identity.py: 0 candidate-only code COMDATs. provider_link.py: SELECTED-PROVIDER LINK PASS (all 6 surplus).
- build/report.json: data 1701/1701 (100%), .bss/.data/.rdata 100%; only the query function (70 meaningful) is unmatched.
- fake_match_scan: 1 lead = the owner-approved `__asm { int 3 }` already in production.

## Storage audit (pdb_storage.py): 5 split-vs-PDB disagreements
- `_bink_pointer_blocks`, `_bink_bitmap`, `_bink_texture`, `_bink_pointer_block_count`: absent from cachebeta publics AND zero UNDEF refs in any build/split object (and in any build/base object; no other source TU names them) => file-static in January (storage-truth rule). Split/ours both external only because symbols.json lacks "static": true.
- S0_static_bss.c (production + `static` on those 4): gate 23/1 unchanged (byte-inert); object_audit only shows storage 2/3 on the 4 (vs a split still generated from the unedited symbols.json). S1 (= q3 + static bss): 24/0, same 4 storage-only diffs. => storage.patch (source + symbols.json line edits in place) closes them after csplit regen.
- `_code_001b5850` (16-byte ret leaf): also absent from publics, no refs anywhere; but S2 (static) makes VC7 drop it (UNWRITTEN) — unreferenced static is not emitted — so it must stay external. Documented anonymous leaf (address name; jonas_anonymous_ret_leaf ledger). Not changed.

## Conclusion
- The || single-assignment production body is REFUTED as January's source (not a register tie): it pins constant 1 in BL in the January context (cdecl), and in the 2276P register-arg context, with TU name count 0..80 inert and operand types inert. The else-if split (q3) reproduces January, 2276betaP and 2276P byte-for-byte and matches defaultP (neutral context).
- The recorded reopen criterion (unoptimized witness of separate START/BACK arms) is UNSATISFIABLE with on-hand artifacts: /Od halo_cache_symbols and HCEA both carry only a bink stub; the Oct 2001 prototypes are all optimized.
- Stop rule: no new production shapes spent (prior waves: 10 shapes). Needs an owner ruling on q3 with the new cross-build evidence.
