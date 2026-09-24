# REVIEW (wave 3 adversarial): transport_endpoint_set_winsock storage packet (_net_startup_debug)

Reviewer slug: scratch/w/review3_transport_endpoint_set_winsock__object_storage___net_startup_debug_/
Tree 954eebd2 (clean). Worker: scratch/w/transport_endpoint_set_winsock/ (verdict ADMISSION_FIX_PROPOSED).
Proposal: static void net_startup_debug(void) + `net_startup_debug();` at top of transport_initialize's
init block; remove transport.h:90 prototype; symbols.json line 1368 "static": true.

## Log (appended as I go)
- Read WORKER_BRIEF 0-8, house rules, lane log held items, parked.json (3 unit parks), rejections (none for unit), worker LEDGER.
- git apply --check: storage_only, transport_h_storage, symbols, production, production_fuzzy_only all CLEAN on 954eebd2.
- Edited symbols.json copy differs from tree at line 1368 only (EOL preserved). hdr/transport.h == tree minus lines 90-91.
- /Od build (halo_cache_symbols.exe): the TU exists (create_endpoint_set at 0x50a8f0, PC winsock variant); scanned
  0x507000-0x50c000 for an empty `push ebp;mov ebp,esp;pop ebp;ret` function: NONE. No /Od attestation of net_startup_debug
  or of any call to it. HCEX.pdb: no net_startup_debug / transport_initialize symbols (360 network layer differs).
- gate.py --source cannot see a shadow header (it copies the .c to scratch/_gate_PID.c; an #error shadow header did NOT fire).
  Wrote rgate.py (compiles the .c in place, build.ninja cflags + unit dir, gate-equivalent rows + storage); #error probe FIRES.
- R1 candidate (storage_only.patch + edited transport.h): 23 EXACT / 3 residual (poll 560 sha, compare 48 sha, init 400!=416);
  _net_startup_debug EXACT 16, storage cand 3 vs current split 2. Control (production, real header): same 23/3.
  Whole-object section diff cand vs production: only `_net_startup_debug` storage 2->3 (53/53 sections). CONFIRMS worker P4.
- R2 STRIP TEST (static, call removed): `_net_startup_debug` UNWRITTEN. The call is LOAD-BEARING for the section's existence.
- R3 alt placement (call moved into transport_dispose's initialized arm): byte-identical to R1 (only storage 2->3).
- R4 alt arity (`static void net_startup_debug(real value)`, call `net_startup_debug(0.0f)`): byte-identical to R1.
  => January bytes cannot discriminate WHICH function calls it, WHERE, or WITH WHAT ARGUMENTS.
- R5 csplit regen (cfg = config/{config,contribs,relocs,splits}.json + worker's edited symbols.json, which equals
  `symbols.patch` applied): 833 objects, ONLY transport_endpoint_set_winsock.obj differs, ONE byte (@8937 =
  `_net_startup_debug` storage 2->3). CONFIRMS worker.
- R6 object_audit: cand.obj vs regen split = 49/49 January symbols equal, FAIL(3) = the 3 residual .text only;
  base(production).obj vs regen split FAIL(4) (+storage); cand.obj vs current split FAIL(4) (+storage) => source static +
  symbols.json must land atomically (as the worker says).
- R7 surplus_identity: 0 candidate-only code COMDATs. provider_link cand.obj: SELECTED-PROVIDER LINK PASS (3 literals, 3 D3D).
  Data: build/report.json .bss 8/8 + .rdata 444/444 = 100% (unchanged by the packet).
- R8 paired shadow-header sweep re-run (worker sweep.py, my slug roots, test TU = my cand.c): 13 consumers, 1,756 sections
  (worker wrote 1,876 - count slip, result the same), only difference `_net_startup_debug` storage 2->3. CONFIRMS worker.
- R9 January/Oct/Sep evidence for the SIGNATURE and CALL (data only, xrefs.py):
  * 0 rel32 calls/jumps and 0 code absolute refs to net_startup_debug in oct-default.xbe (0x63dc0), oct-betaP.xbe (0x81db0)
    and January cachebeta.xbe (0x81e20; one abs hit inside the D3D section is data). No build shows an out-of-line call.
  * Oct 2001 RELEASE oct-default.xbe: the function at 0x63dc0 (first function of the obj, right before get_next_index 0x63dd0
    whose fastcall body matches) is a bare `ret 4` (c2 04 00).
  * Atlas map records (first-party linker maps, 2001-09-25): retail cache_exe `@net_startup_debug@4` (0x452e00, fastcall,
    4 bytes of DECLARED params - compare `@transport_get_key@0` which has `ret 4` for its hidden return pointer only);
    cachebeta_xbe `_net_startup_debug` (0x80970).
  => in the release builds net_startup_debug has ONE 4-byte declared parameter passed on the STACK under fastcall
     (so a float/`real` or a <=4-byte struct, not an integer/pointer which would go in ECX). The January cdecl body `ret` is
     identical for any arity (R4). The proposed `net_startup_debug();` (void, no argument) CONTRADICTS this evidence.
- R10 fastcall lab (VC7 13.00.9254, /O2, fastcall_lab/fc.cod): `@fc_float@4` -> `ret 4`; `@fc_struct4@4` -> `ret 4`;
  `@fc_long@4`/`@fc_ptr@4` -> `ret 0`; `@fc_void@0` -> `ret 0`. So `@net_startup_debug@4` + Oct-release `ret 4` PROVES a
  single 4-byte float-or-small-struct parameter in the Sep/Oct 2001 source; worker F3 shows the TU unchanged Oct->Jan.
  The already-landed `void net_startup_debug(void)` (6bcdbba5a) and the proposed argument-less call contradict it.
- R11 board-wide check of the storage oracle: of 4,868 January split externals referenced from ANOTHER split object, only 1
  (`_item_update_section`) is absent from cachebeta publics. `_net_startup_debug` has no cross-object reference.
  => "absent from publics = file-static" is solid; the STATIC claim itself is accepted.
- R12 emission order is not an oracle for the call site: `_net_startup_debug` is the first .text COMDAT in base, cand,
  alt_dispose and alt_realparam alike (and in January).
- PRECEDENT found: af9f444a (Wave 17, owner branch) landed `_progress_bar_create_noise_texture`: a 16-byte bare-`ret`
  static, no callers, name from the 2001-09-25 maps, with an INFERRED call in progress_bar_setup ("call position inferred
  from map order; the call is required for emission"), a private prototype and a source disclosure comment. It overrode
  progress_bar_obj_base_layer_reconciliation_20260907.md:142 ("do not ... invent its caller to obtain emission").
  Differences from this proposal: (1) there the Sep-2001 decoration `@progress_bar_create_noise_texture@0` AGREES with the
  void call; here `@net_startup_debug@4` + Oct-release `ret 4` CONTRADICT it; (2) there map order gave a positive placement
  argument (between the loading and mask texture builders, both called in sequence); here the placement is a bare guess;
  (3) there the source carried a disclosure comment and a private prototype; this patch has neither.
- R13 owner-option packet (NOT for landing without a ruling): owner_option_source.patch = storage_only.patch + a private
  prototype (as in the file's other statics and in af9f444a) + a source disclosure comment. Built with the edited header:
  byte-identical to cand.obj on all 59 sections except .debug$S (path); 23/3, `_net_startup_debug` EXACT, storage 3.
  git apply --check clean alone and together with transport_h_storage.patch + symbols.patch.

## VERDICT: approve = false (hold as an owner-gated item; zero credit either way)
Verified: every measurement the worker reported (23/3 rows unchanged, storage-only diff, 1-byte csplit regen delta,
49/49 symbols, 0 surplus code, provider link PASS, data 100%, 13-consumer sweep with 0 flips, patches apply cleanly).
The STATIC claim is PDB-solid (R11).
Rejected because the packet's load-bearing source statement is invented:
 1. STRIP TEST fails: without `net_startup_debug();` the section is UNWRITTEN (R2). The call exists only for emission.
 2. No first-party evidence attests that construct: no /Od body, no HCEX symbol, no out-of-line call in Oct betaP, Oct
    release or January, and the bytes accept a call in ANY function (R3) with ANY arity (R4).
 3. The arity is CONTRADICTED by first-party evidence: the 2001-09-25 retail map `@net_startup_debug@4` and the Oct-2001
    release body `ret 4` prove one 4-byte float/struct parameter (R9, R10). `net_startup_debug();` cannot be the
    January statement as written.
 4. Brief section 7 / law (f): a dead statement with no /Od attestation whose only role is the match is owner-gated.
The af9f444a progress_bar precedent shows the owner CAN admit an inferred emission call, but that case had a matching
void decoration and a map-order placement argument; this one has neither. The object stays blocked by 3 owner-held /
tie code rows anyway, so holding costs no credit.
Integrator: apply NONE of storage_only.patch / transport_h_storage.patch / symbols.patch / production.patch now
(production.patch contains the same storage hunk). If the owner rules to admit it, land owner_option_source.patch +
transport_h_storage.patch + symbols.patch ATOMICALLY + csplit regen, then run the 13-consumer full sweep
(consumers.txt). The owner should also rule on the signature (keep void, or add the one 4-byte parameter the release
builds prove, type unknown).
Side note (outside this packet): production's definition order (push_key, get_xnaddr, get_key_id, get_key, pop_key ...)
differs from January's emission order (push_key, pop_key, get_nonce, nonce_is_equal, is_nonce, client_stop, get_xnaddr ...).
