# transport_endpoint_set_winsock worker ledger (lane claude/fifty-objects-20260925, wave 4)

Unit: source/bungie_net/network/transport_endpoint_set_winsock. Tree 954eebd2.
Missing: _transport_endpoint_set_compare_entries (48), _transport_initialize (416), _poll_endpoint_set (560); parks unclassified.

## Baseline (gate --all, production copy)
23 EXACT / 3 residual: _poll_endpoint_set [sha] 560; _transport_endpoint_set_compare_entries [sha] 48;
_transport_initialize [size 400!=416, sha]. (baseline_gate.txt)

## Prior work read (do NOT repeat)
- docs ledgers: set_operations_cluster_20260904 (compare/poll natural), initialize_natural_boundary_20260905 (+ independent review),
  claude_lane_d_refresh_HANDOFF_20260922, claude_structural_owner_packet_20260923 rows 163/164.
- Lane D worker dir C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/bungie_net__network__transport_endpoint_set_winsock/REPORT.md:
  poll: result var + while loops -> 560 [sha]; 13 tail spellings (||, &&, nested, direct returns, result-init timeout) none exact.
  init: I1..I15; I4 (explicit zero field stores + re-store pair before fopen + nested fail-first + timeout->NONE loop + redundant
  `result = none` before initialized=TRUE) is ONE BYTE from exact (XNetStartup-fail jne targets WSA-fail -5 clone, January the deadline clone).
  compare: C1 single-exit, C2 nested single-exit, C3 nested returns: none exact.
- Structural lane dir C:/halo-worktrees/claude-structural-20260923/scratch/lane/w/bungie_net__network__transport_endpoint_set_winsock/REPORT.md:
  poll g1 `boolean select_failed = select_result < 0 || select_result == SOCKET_ERROR; if (select_failed)` -> EXACT, owner-gated
  (named boolean attested only by layout). micro-lab: VC7 lays out bare `if (A||B) X else Y` else-first; materialised value gives January layout.
  h1 function-scope flag -> 576 worse. init rz1 (zero stores + restore pair) 416=416 with 2 epilogue diffs; rz4 long result inert; rz5 assume-fail same.
  compare c2 single-exit, c3 nested ifs, c4 pointer-to-entry locals, c5 b declared first (byte-identical to baseline): none exact.
- Owner packet 20260923: poll boolean and init zero stores are OWNER-HELD items (redundant stores / unattested named boolean).

## Finding F1: October 2001 builds (data-only)
- xbe_tool.py (read-only XBE parser + capstone). 2276betaP.xbe (Oct 12 2001, asserts on): transport_initialize at VA 0x82130
  is instruction-for-instruction identical to January cachebeta.xbe 0x821a0 (136/136 insns, 0 diffs after address masking)
  -> the redundant zero stores / re-store pair / ebx pin are a stable ORIGINAL source property (Oct+Jan), not a January accident.
  2276P.xbe/default.xbe are no-assert release builds (ethernet string ref at 0x64037).

## Finding F2: _net_startup_debug is FILE-STATIC and CALLED in January's TU (new evidence)
- pdb_storage: `_net_startup_debug` absent from cachebeta publics => file-static (only disagreement for the unit).
- No other January split object references it (xref_check: 0), and no relocation in January's own obj targets it.
- Lab (lab_static_unused.c): `static void net_startup_debug(void)` with NO caller -> VC7 does NOT emit it (UNWRITTEN 16).
  lab_static_called.c (call added in transport_initialize) -> emitted, EXACT 16, call inlined to nothing.
- => VC7 static-COMDAT law: January's source both defines net_startup_debug as static AND calls it somewhere in this TU
  (the empty body inlines away, leaving no bytes). The call is a genuine missing January statement; placement unknown (bytes-invisible).

## Probe P1 (lab): net_startup_debug static + call placement on lane-D I4 body (i4_P0..P9)
- Hypothesis: the missing inlined call (F2) perturbs IL/block numbering and flips the XNetStartup-fail cross-jump target.
- Result: I4 on this tree = raw normalized diff at ONE byte (0x10e, jne target 0x135 WSA-fail clone vs January 0x178 deadline clone);
  relocations equal (pooled-literal symbolic targets). All 10 placements (entry, block top, before fopen, before XNetStartup,
  WSA arm, before deadline, before XNetRandom, after initialized=TRUE, before result=none, before return): IDENTICAL (1 byte). INERT.
- /FAsc (i4.cod): label $L18836 (optimizer-created) sits on `pop edi` of the WSA-fail tail; the XNetStartup-fail block was cross-jumped into it.

## Probe P2 (lab oracle only): TU name-count scan before compare_entries (oracle/k00..k32)
- Hypothesis: compare_entries' EAX/ECX swap is a TU name-count (arena ordinal) tie.
- 0..32 opaque struct tags inserted right before its definition: compare_entries residual at EVERY K. Not a name-count tie in this range.

## Probe P3: transport_initialize result-structure grid (grid/gen_init.py, 16 shapes on the I4 body, stores kept)
- Hypothesis (lane-D I13 + micro-lab micro/m4): where result's 0/-5 are assigned decides which -5 tail is canonical.
- Axes: result init at declaration vs outer else; explicit `result = none` on success; XNetStartup-fail then-first vs else-last;
  assume-failure (result=-5 at block top).
- Result: best = init_decl_s1_x0_a0 (== I4) ndiff 1 (0x10e). decl+x1 variants 12-17 bytes (WSA tail gets `mov eax,-5`
  before pops, already-init path shares WSA pops); assume-failure variants 432 B; outer-else variants 253-270 bytes. None exact.
- Micro-lab (micro/t1,m2,m4,m5,gen.py; frameless): X->D only when result has no declaration initializer (then no hoisted xor);
  January needs BOTH the hoisted xor and X->D -> not reproduced by result placement alone.
- /FAsc: the XNetStartup-fail target label $L18836 is optimizer-created; the source exit label $L18468 is S's epilogue.

## Finding F3: whole TU identical Oct 2001 betaP vs Jan 2002 (data-only)
- jan_tu_xbe.txt (cachebeta.xbe 0x81e20..0x82d00 = split 0x70420..0x71300) vs oct_tu_xbe.txt (2276betaP 0x81db0..):
  1353/1353 instructions identical after address masking; all 39 assert line numbers identical (lines.py).
  => transport_endpoint_set_winsock.c was unchanged Oct->Jan; every residual feature (zero stores, `<0`/`==-1` pair,
  compare EAX/ECX colouring, XNetStartup-fail -> deadline tail) is a stable original-source property.

## Finding F4: Oct 2001 RELEASE build 2276P.xbe transport_initialize @0x63f70 (no asserts, aligned frame, LTCG-style reg args)
- The bypass_security fopen block is COMPILED OUT (#if'd) in release, but the re-store pair
  `cfgSizeOfStruct = 0xb; cfgFlags = bl(0)` SURVIVES, placed right before XNetStartup (mov byte [esp+0x10],0xb; mov [esp+0x11],bl),
  and the first-block zero stores (cfgFlags, Recv/SendBufsize via bl) survive too.
  => the re-store pair is a source statement pair OUTSIDE the conditional bypass block (not an artifact of it); third build attesting it.
- Release XNetStartup-fail `jne 0x640bd` also targets the DEADLINE-fail tail (X->D), as in January.

## Probe P4: storage packet (static net_startup_debug + call) is byte-inert
- lab_static_called.c (static + `net_startup_debug();` at transport_initialize entry) vs production obj (secdiff.py):
  only `_net_startup_debug` storage 2->3; all 26 function sections section_infos_equal. Needs symbols.json line 1368 `"static": true`.
- P3b: loop shape (while with PENDING seed, for(;;)+break) INERT (1 byte); result reuse for the WSA short INERT (1 byte);
  `result = -5` before the cleanup calls in W / D / both: 432-448 B (result gets a callee-saved reg). None exact.

## Probe P5 (lab): which -5 tail the XNetStartup-fail joins (micro/n.c, n_else.c; real lab_long_*.c)
- Micro law (frameless micro, N = 2..4 fail blocks): with the result's 0 initialiser in the ENTRY block (hoisted xor)
  the XNetStartup-fail joins the FIRST -5 tail; without it (outer else / direct returns) it joins the LAST -5 tail.
- Real function: outer-else / long+direct-returns / long+mixed variants reproduce January's X->D target (jne 0x178) but lose the
  hoisted `xor eax,eax` and move S's xor into a shared `pop;pop;xor;pop` R_0 epilogue (253 raw diffs, 1-byte shift).
  `long` return + single-exit result: 432 B. January combines BOTH (hoisted xor + S xor before store AND X->D): not reproduced.
- Conclusion: the 1-byte residual is the -5 tail-join choice; no natural shape found in 30+ structural variants (lane D 15 + structural 7 + this lane 25).

## Probe P6: compare_entries spelling census (micro/cmp1..cmp4.c, 25 spellings, frameless micro with unit cflags)
- Hypothesis: January colours A=eax (coalesced with the `a` pointer) and B=ecx because A outranks B in its IL; a spelling that
  references A more (or B less) should flip it.
- Every register-held spelling gives A=ecx, B=eax (base, ==NULL forms, else-return, single-exit, operand-order x4 combos,
  andnot); nested/ternary forms make A a memory compare (`cmp [ecx],0`), 0/1 returns become setcc. B-first tests colour the
  FIRST-tested value into its pointer register - i.e. VC7 gives eax to the value tested in two later blocks.
- Release build 2276P.xbe (frameless, different config) compare @0x641c0 ALSO has A=eax/B=ecx -> robust source property, and
  the P2 name-count scan is negative. No natural spelling found; stop rule (only an interchangeable register left).

## Probe P7: fuzzy (isolated objdiff 3.3.1 project, fuzzy.py) of transport_initialize shapes
- production 78.47794; I4 (+stores, owner-held) 99.963234; i4_nostores (I4 minus the four redundant stores) 89.117645;
  i3_nostores (single-exit, fail-first nesting, timeout->NONE loop, NO redundant statements) 88.63971.

## Audit on cand_combined (fuzzy init + storage packet) - object NOT complete (3 code rows)
- csplit regen with scratch cfg (symbols.json line 1368 `"static": true`): 833 objects, only this obj differs, only
  `_net_startup_debug` storage 2->3. object_audit vs regen split: 49 January symbols 0 differ; DIFFs = the 3 residual .text only.
- surplus_identity: 0 candidate-only code COMDATs. provider_link: SELECTED-PROVIDER LINK PASS (3 pooled literals, 3 D3D tables).
- build/report.json: .bss 8/8 + .rdata 444/444 = data 100%.
- fake_match_scan: only the inherited /Od-attested add_endpoint_to_set identical-branches lead.

## Storage packet needs a shared-header edit (transport.h:90 declares `void net_startup_debug(void);`)
- A static definition under that extern prototype is the brief-7 owner-gated "extern prototype over a static definition".
  So the packet = remove the transport.h prototype (hdr/transport.h, transport_h_storage.patch) + static + call + symbols.json.
- Paired shadow-header sweep (sweep.py: each consumer compiled twice from a scratch root whose bungie_net/network/transport.h
  shadows the real one - verified with an #error probe - identical cflags; ctrl = real header, test = edited header):
  13 consumers (transport_address, transport_endpoint_set_winsock, transport_endpoint_winsock, game, ui_widget_event_handler_functions,
  ui_widget_game_data_input_functions, network_client_manager, network_client_message_handler, network_connection,
  network_game_globals, network_server_manager, network_server_message_handler, telnet_console):
  1,876 sections, ONLY difference = `_net_startup_debug` storage 2->3 in this TU. Zero declaration-count flips.
- Under the edited header vs January: cand_combined 23 exact (same 3 residual), cand_owner_gated 24 exact (init 1 byte + compare).

## FINAL STATE
- cand_combined.c (= production.patch + transport_h_storage.patch + symbols.patch): landable, zero new exact credit.
  (a) transport_initialize single-exit reconstruction: objdiff 78.47794 -> 88.63971; target 416/33 vs 400/33,
      new base sha d5ca96192c597078bf92d9707918dc4888fd6edae9d9bc8c6393ebf1f4159e59 (park re-measure).
      Remaining machine diff: January's four zero stores + re-store pair (cfgFlags=bl, Recv/SendBufsize=bl, size/flags re-store
      before fopen) and the EBX zero pin they create (LAW Z), `mov eax,-5`/xor scheduling that follows from the pin.
  (b) storage packet (net_startup_debug static + called, header prototype removed, symbols.json line 1368 static): byte-inert.
- cand_owner_gated.c (owner_gated.patch): + January/Oct/release-attested redundant stores (I4) + g1 poll boolean:
  24/26 exact; transport_initialize ONE byte (0x10e: XNetStartup-fail joins the WSA -5 tail, January the deadline tail);
  compare_entries EAX/ECX tie. Not landable (owner items: redundant stores; unattested staging boolean).

## Integrator notes
- Apply: production.patch + transport_h_storage.patch + symbols.patch (line 1368 in place; edited full copy config/symbols.json),
  then csplit regen. The storage half (static + call + header + symbols) must land ATOMICALLY; production_fuzzy_only.patch is the
  init change alone (no storage), storage_only.patch the storage source change alone (both git-apply clean).
- Park `_transport_initialize` re-measure: base size 400, relocs 33, sha d5ca96192c597078bf92d9707918dc4888fd6edae9d9bc8c6393ebf1f4159e59,
  objdiff 88.63971. Parks for poll/compare unchanged (98.53933 / 97.61905). No park retired.
- Consumer sweep list for transport.h: consumers.txt (13 TUs), result above (0 flips).
VERDICT: object blocked. poll exact only with owner-gated boolean; init needs owner ruling on the Oct/Jan/release-attested redundant
stores AND still misses one byte (-5 tail join); compare_entries is an EAX/ECX colouring tie with no source lever found.
