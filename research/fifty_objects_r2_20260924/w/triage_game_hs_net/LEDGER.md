# Triage ledger: family game_hs_net (round 2)
Base f6d00a8c. Read-only triage; no candidate source edits; no claims.

## Probe 0: hints.json packets dumped (hints_dump.txt)
- game: 26/27, _game_set_game_variant_from_name S1 (missing csmemset/game_variant_global relocs, frame 208 vs 104).
- hs: 447/448 _real_random_range_evaluate T; data gap 54,780.
- hs_runtime: 64/66 begin_random T (4 ModRM bytes), render_debug_trigger_volumes T; storage/link failures for 10 hs_* converters.
- bink_playback: 23/24 query_analog_controller_buttons S2 (31 vs 40 insns).
- transport_endpoint_winsock: 29/30 connect_endpoint_process@4 S2 (97 vs 99).
- transport_endpoint_set_winsock: 23/26 compare_entries T, transport_initialize S2 (136 vs 126), poll_endpoint_set S2.
- network_connection: 21/23 connect S2 (119 vs 102), server_close_client_connection T (cmp 5 vs 4).
- xbox_sound_cache: 17/18 sound_cache_debug_render S2 (119 vs 116); storage diffs 2.
- bitmap_group: 5/6 add_bitmap S2 (333 vs 330), data gap 1424.
- players: 68/70 examine_nearby_device S2, teleport_internal S1 (extra _biped_fix_position).
- game_engine: 180/180 audit PASS (T1).

## Probe 1: game from_name (read-only)
- alndiff: January keeps `lea edx,[ebp-0x68]; test edx,edx` + memset arm pushing edx (=0) and 0xd0 frame; ours folds to 48 B.
- Prior negatives: Lane B 30 shapes/flags, n1 17 forms, round-1 55+ experiments (name-count 400 compiles, deferral, flags, inline keywords, decl+init). Round-1 reopen criteria require first-party fragment / C1 IL / C2 trace. Nothing new found -> RETIRE.

## Probe 2: hs real_random_range_evaluate
- alndiff: one `mov ecx,edx` placement (0x27 vs 0x2b), 50/50 insns. Round-1: 59 spellings + 13 families, stop rule fired. RETIRE.
- hs data gap 54,780 B = objdiff 3.3.1 '$' defect (round-1 wave3); needs owner scorer move / grouped entry -> OWNER (not code).

## Probe 3: hs_runtime storage
- object_audit/pdb_storage: 12 converters (hs_long_to_boolean ... hs_string_to_boolean) split=static(3), ours external(2), NOT in cachebeta publics; symbols.json already carries "static": true; source hs_runtime.c:469-491 lacks `static`. Only referenced inside hs_runtime.c (conversion table). Admission fix: add `static` to 12 prototypes+definitions (needs gate check that codegen unchanged; address-taken so no private ABI).

## Probe 4: hs_runtime render_debug_trigger_volumes -- NEW /Od evidence
- /Od function located via hs global table: "debug_trigger_volumes" string 0x9810e0 -> table entry -> global 0xc7de25 -> sole code ref 0x606cbd inside aligned-frame fn 0x606c80..0x607462 (od_trigger_volumes.txt).
- /Od shows: switch (axis-aligned: matrix=*identity; set_real_point3d(&matrix.position,...); set_real_vector3d(&local_extent, x1-x0,...); world_extent=local_extent | oriented: local_extent=extents; world_extent=extents; matrix4x3_from_point_and_vectors; matrix4x3_transform_vector | default assert line 0x214 (Jan 0x213)).
- Edge loop: `sides[2] = {0}`; side = edge%2 computed BEFORE axis = edge/2; if (side) { point_from_line3d(&matrix.position,&world_extent,1.0,&points[0]); sides[0].n[(axis+1)%3] = -local_extent.n[..]; sides[1].n[(axis+2)%3] = -...; transform x2; point_from_line3d(&points[0],&sides[0],1,&points[1]); (&points[1],&sides[1],1,&points[2]); (&points[2],&sides[0],-1,&points[3]) } else { points[0]=matrix.position; sides (no negation); transform x2; SAME three point_from_line3d calls }.
  => the points[1..3] tail IS written in both arms in first-party source (/Od does no cross-jumping). This attests the w7 (throughput lane) EXACT candidate's duplicated arm, which was rejected under R2 "duplicated identical stores" without this evidence.
- Two render_debug_string_at_point calls in if/else (colors 0xa1c244 / 0xa1c22c), not a ternary: also /Od-attested.
- BUT /Od uses point_from_line3d (8 call sites incl. center with 0.5); w7 candidate hand-expands them (rule 6 violation). Route: w7 duplicated-arm shape with real point_from_line3d calls.
- Measured: gate --source <throughput lane w7 cand_render_debug_trigger_volumes.c> at f6d00a8c -> _render_debug_trigger_volumes EXACT (1088) (old file; reconcile rows differ). Premise holds at current headers.
- January reloc census: 4x matrix4x3_transform_vector (1 oriented + cross-jumped arms), 1x render_debug_string_at_point (if/else merged), 3x 0.5 fmul (center), 3x 0.95; no point_from_line3d ref (all-inlined TU).
- /Od floats: 0x93dd5c=1.0, 0x9401b4=-1.0, 0x93dd50=0.5 (center via point_from_line3d t=0.5), 0x9411fc=0.95, 0x941b60=0.15.
- nodup_census _point_from_line3d: 24 base definers, 0 NODUP -> a SELECT_ANY emission from hs_runtime is link-safe; owner ruling 5 (2026-09-21) admits the all-inlined COMDAT class with strict-exact caller + byte-identical COMDAT + sweep.
- Route: w7 duplicated-arm body with real point_from_line3d(…,1.0f/-1.0f/0.5f,…) calls + /Od side-before-axis order. Not yet measured (no candidate edits in triage).

## Probe 5: hs_runtime begin_random
- alndiff: 4 instructions (movsx eax/ecx vs edx/eax into dead param homes). w3c: SKIPPED-EXHAUSTED; tie. RETIRE.
- w1 (opus5-150k) t6/t7: real point_from_line3d fixes side-branch x87 operand order but frame layout unchanged (COMDAT then forbidden). w7 (throughput) duplicated arms fix frame ranking (IL refs doubled for points/sides, F3 quicksort (1000*refs)/size) but hand-expand the helper. The COMBINATION (both /Od-attested) was never measured -> NEW route.
- action_vehicle_find_destination (January, exact): point_from_line3d(...,1.0f,...) site inlined with NO __real@3f800000 reloc -> VC7 folds *1.0f, so helper form can reproduce January's add-only arithmetic.

## Probe 6: bink_playback
- Round-1: q3 else-if split exact (24/24, audit PASS) but owner-held (26601453 ruling); Oct-2001 2276P/betaP cross-build corroboration. Storage packet (4 .bss statics) not yet landed (pdb_storage shows 5 non-public externals incl. _code_001b5850). OWNER.

## Probe 7: transport_endpoint_winsock
- Round-1: exact only with uninitialised `thread` (January-bug class, Oct betaP byte-identical). Storage packet already landed (pdb_storage 0 disagreements). OWNER (owner packet 20260923 s4).

## Probe 8: transport_endpoint_set_winsock
- compare_entries: EAX/ECX tie, 25 micro-lab + 8 prior shapes, Oct release same colouring -> RETIRE.
- poll_endpoint_set: exact with staging boolean (owner packet row 163) -> OWNER. transport_initialize: needs 3-build-attested zero stores (owner) + 1 byte -5 tail join unresolved after ~25 shapes. net_startup_debug static needs invented call (review rejected). OWNER overall.

## Probe 9: network_connection
- connect: exact only with redundant unreliable-arm success=TRUE (class H, owner packet s6 recommends reject). close_client: authentic N+1 bound (owner packet s4). cand_full 23/23 audit PASS. OWNER.

## Probe 10: xbox_sound_cache
- One x87 k-term pair keyed on +4 referenced IL symbol units vs PC /Od inventory; no Xbox source. Round-1 stop rule. RETIRE. Storage patch A still pending (pdb_storage 2 ours-static vs split-external + 4 non-public).

## Probe 11: bitmap_group
- 4 instructions (commutative add destination at two pixel_data.address sites); no /Od/HCEX; stop rule. RETIRE. Pending: static postprocess/delete_bitmap + single-section semantic data entry (1,424 B, resolved-equal).

## Probe 12: players
- examine_nearby_device: owner RB2 (decoration paren / wrapper macro) -> OWNER. teleport_internal: o1 (real point_from_line3d) exact only under 3 forced allocator decisions; 11 spellings -> RETIRE.

## Probe 13: game_engine
- 180/180, audit PASS; owner queue item 6 (Q1 alias vs /Od view cast, Q2 headers, Q3 pad aggregate). HCEX: netgame_goal.position is union real_point3d with only x/y/z,u/v/w,n[3] members -> no 2D member, so the /Od 8-byte copy needs the cast; no first-party evidence removes the ruling. OWNER.
