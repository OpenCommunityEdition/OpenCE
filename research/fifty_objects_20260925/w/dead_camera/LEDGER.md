# dead_camera worker ledger (lane claude/fifty-objects-20260925)

Started fresh (no prior slug dir). Prior ledgers read: codex_checkpoint, fable_semantic_names_20260831,
jonas_hcea_constructor_topology_rejection_20260826, opus5_150k_w1_20260914, opus5_next150_n1_20260915,
dead_camera_render_particles closure_freshness_audit_20260830.

Recorded negatives (do NOT repeat):
- new: HCEA combined direct-pitch + current_player-before-unit (272 B); w1 shapes A,C,E,E2,F,G; n1 nA (decl order inert), nB (pitch local removed).
- new: best prior = n1 nA / w1 B (tail phi) -> only R1 (fchs vs mov edi,[ebp+0x10] order) remains.
- update: one insn @0x176 mov eax,[ebp+0x10] vs mov eax,esi; v1 (long next_unit_index=(long)result at switch-block top, no else) gives the load + one extra store; v2 hoisted worse.
- DEGREES_TO_RADIANS(27) refuted for 0.47123894f.

## Probes

### P0 baseline (HEAD source) - gate: new residual 288 sha, update residual 1248 sha, 2 helpers EXACT.
prior_nA.c (copied from opus5-150k worktree n1) re-gated at this base: same; alndiff new = only R1 (0x99..0xa7).

### E1 /Od readout (NEW evidence; n1/w1 never used it)
halo_cache_symbols.exe dead_camera_new = 0x522a20 (found via 0x3ef1463b/0x3f8cbe4c literal refs); dump od_new.txt.
- observer = observer_get_camera(...) BEFORE asserts line 22 (sizeof(struct dead_camera)<=DIRECTOR_CAMERA_DATA_SIZE, const) and line 23 (camera) -> decl-initializer.
- distance/yaw: `fstp [camera+off]` straight after the call -> NO distance/yaw locals in the later source.
- pitch: fstp [ebp-8]; movss; xorps sign; store facing.pitch; THEN timer store (pitch stmt BEFORE timer stmt).
- switch_timer: nested-ternary temp pattern ([ebp-0x10] -> [ebp-0xc] -> camera->switch_timer).
- camera->player_index = local_player_get_player_index(...) direct (no local); player_get(camera->player_index) reload; unit_index via ternary temp; current_player_index = camera->player_index.
- real_local_random_range is a real call in /Od (0x521410) with fstp [ebp-4]; fld [ebp-4] = header `real result=...; return result;` form attested.

### P1 c1.c = /Od-attested dead_camera_new (only observer local; direct negated pitch before timer; nested ternary; no player_index local; unit ternary)
Hypothesis: M8/IL-local-count - prior shapes all kept distance/yaw/player_index locals; /Od shows none.
Result: 288/19; `mov edi,[ebp+0x10]` NOW FIRST after the call (matches January). Tail exact. Remaining R1':
  T: mov edi; mov edx,[const]; fchs; add esp,0xc; fstp [esi+0x10]; cmp; mov [esi+0x1c],edx
  O: mov edi; fchs; fstp [esi+0x10]; mov edx,[const]; add esp,0xc; cmp; mov [esi+0x1c],edx

### P2-P7 (c1 context variants; all 288/19; R1' orders, notation U=mov edi,[ebp+0x10] L=mov edx,[const] N=fchs A=add esp,0xc S=fstp pitch C=cmp T=timer store J=je)
January: U L N A S C T J
- c1 (a: facing.pitch = -call; timer)            -> U N S L A C T J
- c2 (b: pitch=call; facing.pitch=-pitch; timer) -> U N S L A C T J (pitch local inert)
- c3 (c: pitch=-call; facing.pitch=pitch; timer) -> U N S L A C T J
- c4 (d: pitch=call; timer; facing.pitch=-pitch) -> N L S U A C T J (= nA outcome)
- c5 (c1 + if/else switch_timer local)           -> same as c1 (inert)
- c6 (c1 + player_index local)                   -> same as c1 (inert)
- c7 (e: pitch=-call; timer; facing.pitch=pitch) -> N L U A C S T J (differs from w1 shape E in B-context, which = B) => CONTEXT (local set) matters.
Corpus scan (tools/fchs_scan.py, fchs_scan.txt): only dead_camera has fchs on a call result; no exact donor for the idiom.
- ORACLE unused declared locals k=1..7 on c1/c4/c7 (oracle/): all inert (unused decls do not move R1).
- LAB helper-form oracle (lab/*_direct.c = inline with direct return; lab/*_macro.c = hand macro expansion; LAB ONLY):
  c1_direct N U S L; c4/c7/nA_direct N L S U; c1_macro N U S L; c2_macro U N S L; c3_macro N U S L; c4/c7/nA_macro N L S U.
  => helper temp structure moves U/N but never puts L before N. Header form (result local, /Od-attested + glow) stays.
- FACTORIAL (tools/gen.py, fac/*.c, fac/sigs.txt): 5 pitch forms x {distance/yaw locals, switch_timer local, player_index local, watched_unit local} = 80 compiles.
  Context factors ALL INERT. Outcome depends only on the pitch statement form: a/b/c -> U N S L A C T J; d -> N L S U A C T J; e -> N L U A C S T J. None = January.
- LAB helper-form oracle 2: `real result; result=...` (sep) = same as header form; seed local (seedlocal) changes size (272) - excluded by /Od anyway (args pushed before seed call).

### E2 /Od readout of dead_camera_update (0x522c90, od_update.txt) - NEW
Later source: at the TOP of the `switch_timer == 0 && !paused` block `mov [ebp-8], 0xffffffff` = `long next_unit_index = NONE;`
(i.e. the later build FIXED an uninitialised local by adding `= NONE`); then player_has_allies/next-player calls, then
`if (camera->current_player_index != NONE) { [datum_try_and_get guard - later addition] next_unit_index = player_get(...)->unit_index; }`
with NO else arm. HCEA donor also has `next_unit = -1`. => January's topology is `long next_unit_index;` (no initialiser, no else).

### P8 u_bug.c (LAB/OWNER PACKET ONLY): HEAD source with `long next_unit_index;` uninitialised, else-arm deleted, BUG comment.
Result: `_dead_camera_update` **EXACT 1248** (gate: exact 3 residual 1). Target proof complete: the single differing insn
(0x176 mov eax,[ebp+0x10]) is reproduced only by the indeterminate read (n1 v1/v2 proved no defined initialiser elides the store).
Needs OWNER RULING (authentic January bug; brief: report, don't land).

### E3 cachebeta.pdb storage (pdb_storage.py): `_dead_camera_constants` (0x256AE8) is NOT public in January => FILE-STATIC.
Ours/split: external (2). Hypothesis H-static: a file-static never-address-taken const aggregate cannot be aliased by the
`camera->` pointer stores, so VC7 may hoist its load (L) above the pitch store - exactly January's L-before-N/S.
### P9 H-static CONFIRMED: `static struct dead_camera_constants const dead_camera_constants` (st_*.c)
sigs: st_c1 U L N A S C T J (= JANUARY), st_c2 = JANUARY, st_c3 U L N A C S T J, st_c4/st_nA/st_u_bug N U S L, st_c7 N U L A C S.
gate st_c1/st_c2: _dead_camera_new residual [reloc-identity] ONLY (bytes equal; reloc target storage/naming gap vs split's external symbol).
st_u_bug: update still EXACT with static constants.
NOTE: c1..c7/fac/lab/oracle files had a single-backslash __FILE__ literal (tool collapsed \); fixed (127 files) - it only changed the
string literal reloc, never codegen (sigs unchanged).
### P10 st_c1.c / st_c2.c (fixed literal): `_dead_camera_new` **EXACT 288** (gate exact 3 residual 1; update residual = HEAD form).
=> dead_camera_new closes with: /Od-attested statement order (pitch store before timer) + FILE-STATIC constants (cachebeta.pdb).
### P11 static factorial (stfac/, gates.txt): with static constants, pitch forms a AND b are EXACT in ALL 16 contexts each; c/d/e residual.
=> Law for this row: pitch store `camera->facing.pitch = -X` must precede the timer statement; locals/ternary context inert.
/Od frame-slot argument (named locals allocated before temps, in declaration order - shown by update /Od: unit -4, block next_unit_index -8, temps after):
 new /Od slots observer -4, [-8] pitch-neg, [-0xc] switch, [-0x10] inner ternary, [-0x14] unit => unit tail is a TERNARY temp (named local excluded);
 pitch-temp + named switch_timer excluded; consistent: (i) pitch local + switch_timer local, (ii) pitch local + nested ternary, (iii) all temps (a-form + nested ternary).
Chosen production shape (i) = stfac b_dy0_sw1_pi0_wu0 (closest to HEAD, /Od-consistent).
### P12 FINAL candidates
- final_prod.c (landable w/o ruling): HEAD + `static` constants + dead_camera_new shape (i). gate: exact 3 residual 1 (update HEAD residual).
- final_full.c (= final_prod + update uninitialised next_unit_index, BUG comment; OWNER RULING): gate **exact 4 residual 0**.

### Section-4 audit (final_full.obj)
1. gate: exact 4/4 (+ _point_from_line3d guard passed).
2. object_audit: all 6 January-owned sections ok (sizes/flags/align); ONLY diff = `_dead_camera_constants` storage split 2 / ours 3
   -> fixed by symbols_static.patch (cachebeta.pdb: 0x256AE8 not public = file-static). Simulated split with storage 3
   (split_static_sim.obj): section_infos_equal True for all 4 functions.
3. pdb_storage: 1 disagreement (_dead_camera_constants PDB-public False) - resolved by the same static change.
4. surplus: 8 code COMDATs + literals, byte-identical to HEAD's base object surplus; surplus_identity 0 not identical;
   provider_link FAIL(1) `_valid_real_vector3d_axes2` vs bored_camera NODUP hand copy - PRE-EXISTING (HEAD base fails identically);
   precedent: camera_scripting, first_person_camera, flying_camera, following_camera, orbiting_camera, static_camera are all
   Matching while emitting the same ANY COMDAT against bored_camera's NODUP copy.
5. data: build/report.json dead_camera .rdata 48/48 (100%); both data sections byte-identical in candidate.
6. no parks, no admission rejections, no semantic entries for the unit. fake_match_scan: 0 leads (final_prod, final_full).

### Deliverables
- production.patch (HEAD -> final_prod.c; landable): static constants + dead_camera_new shape (i) => _dead_camera_new EXACT.
- symbols_static.patch (config/symbols.json line 14063 in place: "static": true on _dead_camera_constants) + csplit-only regen.
- owner_update_bug.patch (final_prod -> final_full; OWNER RULING, packet claude_structural_owner_packet_20260923 s4 row
  `_dead_camera_update`): new /Od evidence = later build initialises `next_unit_index = NONE` at block top (the fix).
- full_object.patch = HEAD -> final_full (4/4).
Hashes: HEAD de77a780..., final_prod 708198eb..., final_full 7186c901..., split obj 3831c1cc...
