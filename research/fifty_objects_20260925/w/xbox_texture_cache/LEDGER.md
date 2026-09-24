# xbox_texture_cache - _texture_cache_steal_memory (fifty-objects lane 20260925)

## Baseline
- gate --all on production: 24 exact / 1 residual (_texture_cache_steal_memory [sha]) / 0 unwritten.
- Prior ledgers read: steal_memory_owner_reconciliation_20260904 (ESI/EDI returned-pointer tie, no search),
  opus5_150k_w2 (sm1 named writable early: worse; sm3 no stolen_size local: worse; sm4 end-guard operand order: same;
  sm5 named writable after resize: same), w3 (skipped exhausted). Park reopen: authoritative locals or donor.
- NEW evidence available since those waves: /Od build readout (odbuild.py), decoded allocator/frame laws.
- /Od build (halo_cache_symbols.exe) is the PC build: only pc_texture_cache.c; no xbox steal_memory body (0x51f160 is a different 79-byte fn asserting !pc_texture_cache_globals.stolen_memory at line 0xa0). No /Od readout available.
- HCEX.pdb: no texture_cache_steal_memory record (texture_cache_return_memory is a 4-byte stub in the PPC build). No locals source.
- Alndiff baseline (production): 84/84 insns; only diff = late CSE temp `lea R,[ebx+0x104000]` (writable ptr) R=ESI in January
  (ESI's stolen_size last used by the push immediately before), EDI in ours; plus return mov/pop order. Everything else identical,
  including end-guard spill into [ebp+8].
- PROBE O1 (LAB ORACLE, never land): TU name-count oracle, N=0..44 dummy `extern long` names immediately before the function:
  all 45 residual (no flip). Name-count/arena-ordinal is NOT the lever here.
- PROBE O2 (LAB ORACLE): 1..12 dead block-scope locals at top of the function: all residual (no flip).
- PROBE v1 size_reuse (stolen byte count stored back into `size`, no stolen_size local): residual + WORSE: end-guard lea operand
  order flips to [ebx+esi+..] (Jan [esi+ebx+..]) -> January's end guard IL is built with stolen_size as the lea base; stolen_size
  is a distinct local. Rejected.
- PROBE v2 declarations-then-assignments (same statement order): identical to production (ESI/EDI only). Inert.
- PROBE v3 pointer locals declared before counts: identical to production. Declaration order inert.
- PROBE v5 stolen_size computed right after page_count: much worse (IL order follows statements; confirms January computes
  stolen_size AFTER base_address). Rejected.
- PROBE v7 base_address in two statements (`= get(); +=`): identical to production. Inert.
- TOOLING: copied Lane A res9 alloc-oracle (trace.py/oracle.py/mapweb.py + dbg32c.exe) to scratch/w/xbox_texture_cache/ao,
  retargeted WT to this worktree (C2.dll sha1 identical 65de5daa...). Oracle on production: R0..R3 all pass.
  Decisions: #0 id1 esi pri144 (page_count chain) / #1 id5 edi pri99 (remaining) / #2 id2 ebx pri97 (base) /
  batch1: #3 id11 = writable CSE temp pri48 allowed{esi,edi} cost esi=1001 -> EDI ; #4 id10 pri45 allowed{esi} only -> ESI.
  mapweb: forcing id11->ESI spills id10 = stolen_size (reloaded from [ebp+8]) and moves end guard to a new [ebp-4] slot.
  => ROOT CAUSE (ours): in our IL the writable temp INTERFERES with the stolen_size web (id10), which has only ESI left, so
  the chooser charges ESI +1000 (100 x f40=10 single-register neighbour). January has BOTH in ESI => in January's IL they
  do NOT interfere: January's writable-address def comes after stolen_size's last use (or they are not both live).
- ORACLE DECODE of W_tmp's ESI cost 1001 = 600 (id10 stolen_size: uncoloured nbr, single allowed reg ESI, f40 6) + 400 (id7 END GUARD
  web: uncoloured nbr, single allowed ESI, f40 4, later spilled to [ebp+8]) + 1 pref. EDI cost 0 -> EDI.
- LAB LA (assignment inside arg `stolen_memory = base+G` + return stolen_memory): identical decisions (1001). Inert.
- LAB LC (stolen_size last used in an EARLIER call, W_tmp in a later call): 10-11 interference gone, cost esi still 400 (end guard) -> EDI.
- LAB LD (stolen_size as arg3 of same call): still 1001. => in our IL every arg web of the SAME call interferes with the CSE temp.
- sm4/sm5 re-run under the oracle: identical decision tables to production.
- MODEL requirement for January's ESI: (a) stolen_size must not interfere with W_tmp, AND (b) the end-guard web must not be an
  uncoloured single-ESI neighbour at W_tmp's decision (e.g. stolen_size coloured first so end guard's allowed set is empty).
- CENSUS (pattern_census2.py): `push R; <redefine R>; push R` inside ONE call's args with R already live: 17 sites in January,
  16 in production. The ONLY January site where the redefinition is a computed value (lea) is texture_cache_steal_memory; the
  natural VC7 form elsewhere is a variable LOAD (e.g. files.c file_reference_copy: push source(esi); mov esi,[ebp+8]
  (destination param); push esi; ... mov eax,esi return). January draw_gravy_layer has a register-copy variant (residual here too).
- LAB LH (arg1 = the `size` param LOADED from its home, returned): allocator gives arg1's web ESI with NO penalty and the output is
  `push esi; mov esi,[ebp+8]; push esi; ... pop edi; mov eax,esi; pop esi` = January's exact epilogue/pop shape. LAB L1/LG: a
  COMPUTED arg (lea temp, CSE or not) interferes with the other arg webs; a LOADED variable web does not.
  => January's `lea esi,[ebx+0x104000]` behaves like a reload: hypothesis R = a named writable-address variable assigned EARLY
  and REMATERIALISED at its use (web starts at the use), not a CSE temp built at the call.
- **PROBES R1-R4 -> STRICT EXACT (gate --fn)**: a NAMED writable-address local initialised EARLY with the other locals
  (`byte *stolen_address = base_address + GUARD;`), end guard still computed from base_address (NOT from stolen_address: that is
  sm1, structurally worse), first protect call and the return use the named local. All four declaration orders exact
  (R1 after stolen_size, R2 after end guard, R3 end guard spelled base+stolen+guard, R4 before stolen_size) => order inert.
  Mechanism: the early named value is not kept across the assert/lruv blocks but rematerialised at its first use (web starts at
  the use like a reload), so it no longer interferes with the stolen_size arg web -> chooser picks ESI (class order).
- FULL GATE cand.c (= production + R1 body): **25/25 EXACT**, 0 residual, 0 unwritten, _point_from_line3d guard passes.
  (Declaring an extra local did not disturb any later function - name-count safe for this TU.)
- object_audit cand.obj: every January-owned section OK; 3 symbol storage DIFFs (IDirect3DBaseTexture8_IsBusy@4,
  IDirect3DDevice8_IsBusy@4, IDirect3DDevice8_KickPushBuffer@4: January 2/ours 3) = stock XDK `D3DINLINE static __forceinline`
  (same systemic class as admitted-complete rasterizer_xbox_debug, which audits 8 such DIFFs); plus candidate-only surplus.

## RESULT (final)
- Final candidate: scratch/w/xbox_texture_cache/cand.c (CRLF preserved; = production + F_final body), sha256
  55588ed4daa0180f7d1c81d7e35b06ae635ef2707062f391a357a9a8bcdb92f9; object cand.obj sha256
  7e0164452072e5a6884d0c9e82fd876b8db7433164d892cd29fe023695f40f93.
- gate --all --forbid-emitted-symbol _point_from_line3d: **25 exact / 0 residual / 0 unwritten** (cand_gate_all.txt).
  _texture_cache_steal_memory 224 padded / 209 meaningful / 16 relocs, sha 7ffbda42... == January.
- Oracle on the exact candidate: stolen_size web coloured ESI first (round 1); the named local's early web (id7, live across the
  asserts) is left uncoloured and rematerialised; its late piece (round 2) takes ESI at zero cost. Exactly January's allocation.
- object_audit (cand_object_audit.txt): all January-owned .text/.rdata/.bss sections OK. 3 symbol storage DIFFs are the stock XDK
  D3DINLINE wrappers (split 2 / ours 3); pdb_storage: cachebeta publics say NOT public => file-static truth == ours (same class as
  admitted-complete rasterizer_xbox_debug). Surplus (pre-existing, unchanged by this edit): .drectve, 10 string literals, 3 XDK
  tables, 4 __real constants, _add_vectors3d/_dot_product3d COMDATs (surplus_identity IDENTICAL); provider_link both orders PASS.
- build/report.json (production): data 6344/6344 = 100%; code 3248/3457 with steal_memory the only non-100% function -> 3457/3457.
- No admission rejections recorded for this unit.
- Production changes: apply production.patch (git apply; LF under core.autocrlf=true) or production_crlf.patch (GNU patch
  --binary -p1) to source/cache/xbox_texture_cache.c; retire config/parked.json entry {unit source/cache/xbox_texture_cache,
  function _texture_cache_steal_memory, class unclassified}. No header/symbols.json/config changes.
- House-rule notes: the only change is a genuine named local (`byte *stolen_address`, declaration+initialisation combined, used
  by the READWRITE protect call and the single return). Not a decoration: it carries the value used twice; placement/type are
  inert (R1-R4, void* variant all exact), so exactness does not hinge on a knife-edge spelling. The un-named spelling (production)
  is the residual; sm1 (end guard derived from stolen_address) is refuted by bytes.
