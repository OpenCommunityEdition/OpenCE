# E1 outcomes log (compiler-application wave 3; worker E1; released Codex units)

Protocol: one immutable card per probe in `cards/<id>.txt`, clock-read timestamp, written BEFORE the compile it
predicts. Outcomes appended here with clock-read timestamps. Copies in `copies/`, objects in
`scratch/campaign/workers/E1/`. Base: worktree HEAD 26684ca8 (= a8854940 content for all nine E1 units; confirmed
`git diff --stat a8854940 HEAD -- <unit>.c/.h` empty and working tree clean for them).

## 0. Baseline (clock-read 2026-09-26 01:02:35 -0700)
- `gate_base.txt`: connected_geometry 7/3; periodic_functions 6/1; xbox_sound_cache 17/1; hardware_bitmaps 18/1;
  dead_camera 3/1; cinematics 16/1; lightning 8/1; path_structure_bsp 6/0; hardware_geometry 17/0.
- Fresh objects of the two admission candidates equal build/base (keyed_diff 0 changed / 0 added / 0 removed).

## 1. Record read before work (Codex packets read-only; no write or git op in any Codex worktree)
- claims.json: all nine units `reserved` by codex since 2026-09-24, empty history.
- Codex branches (read via `git log/diff` from this worktree only): cinematics-close (68c2401d/f66e522d),
  connected-geometry-five (f6d3c6a0/dd6a8e59), dead-camera-five (aa822721/d347d1d5), hardware-bitmaps-five/close
  (19f7c448, b9dd803d), hardware-geometry-inline-evidence (5e324b16), lightning-smallest-five (818f2890),
  path-structure-provider-research (6f834b4a), periodic-data / periodic-functions-five / periodic-il-research
  (8188648c/2d5eb521, 1f407d54, 42ac544a), xbox-sound-cache-five (da7ac2cc). Every doc/source file of these branches
  is byte-identical in HEAD except config (canonical moved on) and the superseded -five bitmaps doc and sound-cache
  source (canonical carries the later R2 name `cache_block_get_sound_permutation_name`).

### P1 (cards/P1.txt) ray2d /Od flag loop, copy copies/path_structure_bsp_P1.c  (outcome 2026-09-26 01:17:33 -0700)
- gate --all: **exact 6 / residual 0**; keyed_diff vs build/base: 26 -> 26 sections, **0 changed / 0 added / 0 removed**
  (whole object byte-identical to production). Prediction EXACT CONFIRMED. The `goto continue_from_surface` is NOT
  load-bearing; the /Od-attested `next_surface_index = NONE` flag loop reproduces January.
- Harness note: a first compile had single backslashes in the two new assert paths (heredoc escaping) and showed
  reloc-identity on those two literals only; fixed in the fragment before the recorded compile.

### P1b (cards/P1b.txt) /Od-faithful: no flags local, enter_t-first compare  (outcome 2026-09-26 01:18:27 -0700)
- gate: exact 5 / residual 1: ray2d [sha]; alndiff: ONLY the enter compare differs (J `fld [ebp+0x1c] distance;
  fcomp [ebp-0x18]; test ah,5; jp` vs ours `fld enter_t; fcomp distance; test ah,0x41; jne`) plus the two literal
  naming rows. The byte-local removal is INERT (the repeated pathfinding byte loads CSE as predicted); the compare
  spelling `enter_t > distance` is the one wrong element. Prediction PARTIALLY FALSIFIED (compare canonicalisation
  does NOT hold for x87: VC7 loads the textual left operand). January's spelling is `distance < enter_t`, which is
  consistent with /Od's SSE `movss enter_t; comiss distance; jbe` for `distance < enter_t` (SSE lowers x<y as y>x).

### P1c (cards/P1c.txt) P1b with the production enter-compare spelling  (outcome 2026-09-26 01:23:55 -0700)
- gate --all: **exact 6 / residual 0**; keyed_diff vs build/base: **0 changed / 0 added / 0 removed** (object
  byte-identical). Prediction CONFIRMED. P1c = /Od flag loop, no goto, no flags local: the proposed admission cleanup.

### H1 (cards/H1.txt) hardware_geometry: delete the four hand Unlock stubs + symbols.json names/static  (outcome 2026-09-26 01:26:24 -0700)
- vs CURRENT split: exact 13 / unwritten 4 (predicted; source and symbols.json must land together).
- Scratch csplit (build/tools/csplit.exe, cachebeta.exe, copies of the 5 config inputs): split_ctl == build/split
  833/833; split_H1 differs from split_ctl in exactly ONE object (rasterizer_xbox_hardware_geometry.obj). CONFIRMED.
- vs split_H1: **17/17 EXACT** (sgate.py); object_audit **PASS** (25 January symbols, 0 differ; was FAIL(7));
  pdb_storage 13 -> **2** (_code_00158450@8/_code_00158460@8 only, the held MoveResourceMemory pair); surplus_identity
  0 code COMDATs (was 4 surplus Unlock wrappers); provider_link PASS (3 literals, 3 SDK tables); /W3 census identical to
  HEAD (C4146 x1, C4244 x13, no C4013); fake_match_scan 0; protoscan 0. All predictions CONFIRMED.
- Zero code/data credit (renames of already-exact bytes). The object STAYS NonMatching: the 0x158450/0x158460 hand
  stubs remain until the owner rules on the held MoveResourceMemory call (4d1ebf17 hold retained, not touched).
- Patches: rasterizer_xbox_hardware_geometry.patch (source; git apply --check OK) + LEAD-OWNED
  proposals/symbols_hardware_geometry_H1.patch (11 in-place lines 5813-5825; git apply --check OK). Must land together,
  then regenerate csplit. Battery: battery/hardware_geometry_H1.txt.

### P2 (cards/P2.txt) strip test of every 3D->2D view cast in the P1c candidate  (outcome 2026-09-26 01:27:56 -0700)
- 9 cast occurrences deleted (line2d: 4 vertex casts on 4 calls = 5 occurrences incl. the double-cast call, plus
  &point_in_surface; pill2d: &left_result.point, &right_result.point, &best_result->point).
- gate: 6/6 EXACT; keyed_diff vs build/base: 0 changed / 0 added / 0 removed; /W3: +9 C4133 (the stripped sites) and
  nothing else. Prediction CONFIRMED: every cast is byte-inert, /Od-attested per site (card P2), compatible prefix.
  Diagnostic only - the casts stay (C4133 otherwise).

### C1 (cards/C1.txt) find_or_add_edge owner-question candidate: e15 topology + SET_FLAG tail + named bit  (outcome 2026-09-26 01:32:04 -0700)
- gate: exact 8 / residual 2 (vertex, coplanar); keyed_diff: 1 changed (find_or_add_edge 256 -> 240, EXACT vs
  January), 0 added / 0 removed. Prediction CONFIRMED. The project SET_FLAG macro with the named bit reproduces
  January as well as Lane D's LONG_MIN/LONG_MAX spelling, so the packet keeps the macro and the enum (rules 12/13).
- BUG comment rewritten to the methodology template with the quoted January offsets and the NEW reachability fact
  (dynamic_array_delete stores count = NONE; the count<0 path calls dynamic_array_get_element at +0x9b first, whose
  asserts 0x7E element_size>0 / 0x80 count>=0 halt before the +0xbe read). Comment is byte-inert (re-gated).
- /W3 identical to HEAD; fake scan 0. Held owner item (uninitialised read): NOT for landing without a ruling.
- Re-verified at HEAD (no new shapes): Lane D candidate.c (safe, direction = TRUE): 7/3, edge 256 (only the
  +0x10 init store differs); Lane D ruling_direction: 8/2; ruling_direction_realcmp: 9/1 (vertex EXACT via the
  TU-local realcmp_epsilon macro).

### T1 (cards/T1.txt) list-scheduler trace of the production periodic builder, run e1_t1b_periodic_prod  (outcome 2026-09-26 01:36:13 -0700)
- Sealed run, stock-equal True (whole object equal to the unmodified compiler except the COFF timestamp); gate (6,6);
  286 cycles; 81 encoded nodes. Table: scratch/campaign/workers/E1/analysis/e1_t1b_periodic_prod.sched.json.
  (First attempt e1_t1_periodic_prod aborted before compiling: W7's fns_in_order drops '@' fastcall names; fixed.)
- Rows (offset bytes | insert cycle | +0x2c | +0x36 | unit | select cycle):
  5d fadd[1.0] 20 0x53c00 20 u2 20 | 63 mov edx,[ebp-8] 20 0xa000 49 u0 21 | 66 add esp,10 20 0x1c00 19 u0 22 |
  69 <0-byte node> 20 0x45000 21 u0 23 | 69 fmul[.25] 23 0x52000 22 u2 23 | 6f fld[ebp-4] 23 0x4c800 23 u2 24 |
  72 inc edx 22 0x1800 50 u0 24 | 73 fmul[K] 24 u2 25 | 79 add esi 20 0x1400 52 u0 25 | 7c dec edi 25 u0 26 |
  7d mov [ebp-8],edx 24 0xc00 51 u0 27 | 80 fcos 25 u2 28.
- (a) FALSIFIED: inc is ready at cycle 22, fld only at 23 - not the same cycle. (b) CONFIRMED in form (fld key
  0x4c800 >> inc 0x1800) but it is NOT what decides the order. (c) The decision is UNIT contention on class 0:
  inc is ready at 22 but loses the single unit-0 slot to `add esp,0x10` (0x1c00 > 0x1800) at 22 and to a
  ZERO-BYTE unit-0 node (+0x36 21, key 0x45000, IL between the fadd[1.0] and the fmul[.25] of the first term) at 23;
  at 24 it issues with fld, which sorts first. Every `(X + 1.0f) * Y` term has such a 0-byte unit-0 node between
  its fadd and fmul (+0x8b w36 28, +0xa4 w36 36, +0xbd w36 44).
- Consequence (inference, to test): January's order (inc in cycle 23 next to fmul[.25]; add esi/dec edi one cycle
  earlier) is exactly what this schedule gives if the 0-byte unit-0 node at +0x69 does not compete for unit 0 in
  cycle 23. The node's identity (opcode/operands) is the next observation.

### T2 (cards/T2.txt) node identity, run e1_t2_periodic_nodes  (outcome 2026-09-26 01:38:33 -0700)
- The 0-byte unit-0 nodes (+0x69, +0x8b, +0xa4, +0xbd) are C2 opcode **0x267**, FP type 0x4004, src/dst kind 1 (the
  fadd result web -> the fmul input web), one after every `fadd [1.0]` and before its multiply. Prediction (FP value
  transfer, 0 bytes) CONFIRMED; its C-level origin undetermined by T2 alone.

### L1 (cards/L1.txt) lab_paren.c node censuses, runs e1_l1_*  (outcome 2026-09-26 01:38:33 -0700)
- (a+1.0f)*b: 0x267 between fadd and fmul (predicted). a*b+b: NO 0x267 (predicted). (a*b)+b: 0x267 between fmul
  and fadd (predicted). `real t = a + 1.0f; return t*b;`: 0x267 PRESENT - H-PAREN (parens only) FALSIFIED in its
  narrow form. Refined reading: 0x267 marks an FP value that C semantics require to be materialised as a `real`
  (a parenthesised FP subexpression or a named-float assignment): a reassociation/rounding barrier, 0 bytes without
  /Op, but a real scheduler node in unit class 0. Node line field high byte: 0x03 for (a+1)*b and the named local,
  0x01 for (a*b)+b.

### L2 (cards/L2.txt) lab_paren2.c, runs e1_l2_*  (outcome 2026-09-26 01:39:41 -0700)
- 0x267 present between fadd and fmul in all four: (a+1)*0.25f, (real_random()+1)*0.25f, (a+1)*0.25f+b,
  0.25f*(a+1). Prediction (unconditional) CONFIRMED. No context of `(X + 1.0f) * Y` drops the barrier.
- Schedule reading (T1 + L2, inference): January's order = ours with the term-1 barrier absent from integer unit 0 at
  cycle 23 (then inc issues at 23 after fmul[.25], add esi with fld at 24, dec edi before fmul K at 25, the
  [ebp-8] store before fcos) - every January integer op one cycle earlier, exactly the recorded residual pattern.
- NEW lead: the /Od builder (0x6c8460 +0x5b..+0x68) spells term 1 as `real_random()*0.25f + 0.25f`
  (mulss [0x93ddf4]; addss [0x93ddf4], one constant) - a form with NO parenthesised FP subexpression. The ledger
  classed it as a later-revision formula difference; no recorded probe compiled it.

### L3 (cards/L3.txt) lab_factor.c, runs e1_l3_*  (outcome 2026-09-26 01:40:19 -0700)
- `a*0.25f + 0.25f` compiles to `fld a; fadd [__real@3f800000]; fmul [__real@3e800000]` and
  `real_random()*0.25f + 0.25f` to `call; fadd [1.0]; add esp,4; fmul [0.25]`: VC7 13.00.9254 /O2 FACTORS c*a + c
  into (a + 1.0f)*c (creating the 1.0f constant), and the node census shows NO 0x267 barrier. Prediction (no
  factoring) FALSIFIED. So January's `fadd [1.0]; fmul [0.25]` is exactly what the /Od-attested term-1 spelling
  produces - and it is the only known spelling of that byte pattern with no barrier node.

### P3 (cards/P3.txt) periodic builder term 1 = /Od `real_random()*0.25f + 0.25f`  (outcome 2026-09-26 01:42:08 -0700)
- gate --all: **exact 7 / residual 0** - `@periodic_function_build_variable_period_x_table@4` **EXACT** (256 padded /
  244 meaningful, 18 relocs); keyed_diff vs build/base: 58 -> 58 sections, 1 changed (the builder, EXACT vs January),
  0 added, 0 removed. Prediction CONFIRMED.
- Admission battery (battery/periodic_functions_P3.txt): object_audit PASS (46 January symbols, 0 differ, every
  January section incl. .data/.bss ok); pdb_storage 0; surplus_identity: _fast_ftol (= actor_combat) and _real_random
  (= action_charge) IDENTICAL; provider_link PASS both orders (17 rows); --baseline: no new surplus; /W3 identical to
  HEAD (no C4013); fake scan 0; protoscan 0. census_all: the object's only remaining row was this function (data
  already credited) -> periodic_functions becomes an admission candidate.
- Source-review note: the builder's three `(real)cos(...)` are a hand-expanded form of real_math.h cosine(), which
  the /Od builder calls (fn 0x455220). Card P3c tests the /Od-faithful helper call.

### P3c (cards/P3c.txt) P3 + real_math.h cosine() in the builder  (outcome 2026-09-26 01:42:51 -0700)
- gate 7/7 (builder EXACT); keyed_diff: 1 changed + **1 ADDED `_cosine` (16 B)**; surplus_identity: _cosine IDENTICAL
  to January's selected actor_combat copy; provider_link (new surplus) PASS both orders; object_audit PASS; pdb 0;
  /W3 identical; fake 0. Prediction CONFIRMED.
- Choice (inference, disclosed): January's build_table REFUTES cosine()/sine() by bytes (P38: helper form breaks
  build_table), while the later /Od revision calls the helpers in BOTH functions; so the /Od cosine() calls are a
  later-revision style, and within-TU January style is the direct (real)cos. Primary patch = P3 (no new symbol);
  P3c kept as periodic_functions_ALT_cosine.patch for the lead/owner.
- Strict comparator: builder section_infos_equal True for P3 and P3c (normalized sha = January 802ef4da...).

### T3 (cards/T3.txt) OR operand sort in cinematic_render, run e1_t3_cine_sort  (outcome 2026-09-26 01:46:18 -0700)
- Sealed, stock-equal True; function ordinal 18; the OR node (+0x466) passes the sort site 16 times (8 pre/post
  pairs in two optimizer rounds). Incoming order [0x279 subtree, 0x27a subtree] (source order: SHL | AND);
  keys: 0x279 = 0x1020421 throughout; 0x27a = 0x10300df (round 1) then 0x102fe3a (round 2). Sorted order
  [0x27a, 0x279] every time => 0x27a = input 1 = the destination (ours `or ecx,eax`, ecx = masked colour) =>
  0x27a is the AND subtree, 0x279 the SHL subtree. (a) CONFIRMED: key(AND) > key(SHL) in production.
- Both keys are (level<<16 | inner16) with level 0x102 at the final sorts; AND inner 0xfe3a vs SHL inner 0x0421.
  January (input 1 = SHL) needs key(SHL) >= key(AND) (ties keep the incoming SHL-first order).
- Next: child entries of both subtrees (card T3b) to decompose the inner sums into symbol ids.

### T3b / T3c (cards T3b, T3c) children of the OR subtrees: production vs lab_min0 (January order)  (outcome 2026-09-26 01:49:58 -0700)
- Harness notes: sort_trace first assumed the encoder in the LAST gated segment; dbg32c prints a GATE line at every
  MARK, so the backend of function k is segment k (fixed). Lab function MARK ordinals come from mark_names.py
  (cinematic_render is compiled LAST in the lab TU). Two sealed lab runs whose post-processing had aborted on that bug
  were deleted and re-run under the same labels (e1_t3c_min0b/c) - recorded here; no production evidence involved.
- Key decomposition CONFIRMED on both functions (every final key reproduces):
  kind-13 key = (0x100 | level) << 16 | inner16, level = max(child level) + 1, inner = sum(child_inner << i) +
  opcode - 0x248 (mod 2^16); SHL = 0x279 [alpha, const 24 (key 0x18)] ; AND = 0x27a [shadow_color memory operand
  (kind 6, opcode 0x250, key inner f), const 0xffffff (key 0xff00)].
  => SHL inner = (alpha_inner + 0x61) mod 2^16 ; AND inner = (f + 0xfe32) mod 2^16.
- production (named local `shadow_alpha`): alpha = kind-2 symbol id 30 (key 0x103c0), f = 0x0008 at the final
  sorts (0x2ad, level 2, in round 1) -> SHL 0x1020421 < AND 0x102fe3a -> AND first (ours).
- lab_min0 (p1 shape: PIN inline in the argument, no shadow_alpha local; 0 clamps): alpha = kind-2 symbol id 0x23e
  (574, a compiler-created temp; key 0x147c0), f = 0x4008 -> SHL 0x1024821 > AND 0x1023e3a -> SHL first (January).
- Prediction (T3c: difference = field-operand base, not alpha) PARTIALLY FALSIFIED: BOTH operands differ - alpha's
  id (30 named local vs 574 temp) and f (0x0008 vs 0x4008, i.e. the base term (base key << 8) mod 2^16).
- Consequence (arithmetic, not yet a source claim): with a low-id named alpha (id < ~239) SHL can never win
  (AND inner >= 0x1e3a unless f wraps it to 0xfe3a+) - so January's alpha is NOT a plain named local; it is a
  high-id temp, as in the /Od spelling (no shadow_alpha local; PIN inline). The remaining variable is (alpha temp id,
  f) - both set by upstream symbol/temp numbering.
- lab_min3 (3 clamps, AND first): alpha temp id 0x24a (586, key 0x14940), f = 0x0008 -> AND 0xfe3a > SHL 0x49a1.
  lab_min4 (4 clamps, SHL first): alpha temp id 0x24e (590, key 0x149c0), f = 0x4008 -> AND 0x3e3a < SHL 0x4a21.
  => with the /Od spelling (alpha = high-id temp) the decisive term is f's base byte: (base key << 8) mod 2^16 =
  0x4000 -> SHL first (January), 0x0000 -> AND first. The base key moves with upstream temp counts (lab10 MIN count).

### T3e (cards/T3e.txt) base of the shadow_color memory operand  (outcome 2026-09-26 01:51:39 -0700)
- Final sorts: the base is a kind-2 entry whose symbol has storage byte 3 and a HIGH C2-created id: production 0x240,
  lab_min3 0x240, lab_min4 0x241 (round 1: a different temp, storage 0x80003 with a defining subtree, ids 0x12d/0x127/
  0x129). Storage 3 keys are id<<6, so f = ((id << 6) << 8) mod 2^16 + 8 = ((id & 3) << 14) + 8: ONLY id mod 4 matters:
  AND inner = 0xfe3a / 0x3e3a / 0x7e3a / 0xbe3a for id = 0 / 1 / 2 / 3 (mod 4). Prediction (C2-created high id, not
  the named title local) CONFIRMED; the "id mod 8 via <<5" detail FALSIFIED (storage-3 <<6 path, mod 4).
- DECODED RULE for this OR (inference from the decoded key + 5 traced variants, all consistent):
  SHL-first (January) <=> (alpha_inner + 0x61) mod 2^16 >= AND inner. With a NAMED low-id `shadow_alpha` (production:
  id 30 -> 0x421) this is impossible for every base id => January's alpha is not a named local (consistent with /Od:
  PIN inline, no shadow_alpha). With the alpha temp (ids ~0x23e..0x24e -> 0x48xx) it holds iff base id = 1 (mod 4).
  p1-shape with 3 clamps gives base id 0x240 (= 0 mod 4); every recorded /Od-attested variant p1..p7 stayed AND-first.
- STOP for cinematic_render (bounded): the remaining lever is the count of C2-created symbols before the title-base
  copy (one more, mod 4). No first-party construct supplying it is known; tuning counts is forbidden (rule
  "Do not tune declaration counts, symbol/name counts ... or local IDs"). Reopen criterion (sharpened): an authentic
  January source difference upstream of the pack (evidenced independently) that changes the C2-created symbol count
  before the base copy by 1 mod 4, on top of the /Od no-shadow_alpha spelling.
- Zero-credit consequence: production's named `shadow_alpha` local is refuted by January's bytes under this rule (it
  can never give January's order); the fifty-objects optional_fidelity.patch (removes it) is supported.

### T4 (cards/T4.txt) lightning_submit tail schedule, run e1_t4_lightning_prod  (outcome 2026-09-26 01:53:46 -0700)
- Sealed, stock-equal True. In ours `fstp [esi+0xc]` (+0x876, w36 81) closes one scheduling block and
  `mov ecx,[ebp-8]` (+0x879) opens the NEXT block (w36 4 of 1..6: color store, texture.y store, add esi, reload,
  test, jne). So the transposition is a block-membership difference (January schedules the reload among the FP stores
  of the previous block, or the texture.x store in the next), NOT a key tie within one block. Prediction (a) (unit-0
  refusal inside one block) FALSIFIED; (b) CONFIRMED (reload key 0x9800 is the highest in its block after the branch).
- Not pursued further: the function also keeps the independent global_z_axis3d copy register residual (+0x38e);
  closing one of the two gains nothing. Park unchanged; zero shapes compiled.

### T5 (cards/T5.txt) rasterizer_bitmap_new IL at the first post-MARK walk, run e1_t5_bitmap_new_il  (outcome 2026-09-26 01:57:08 -0700)
- Sealed, stock-equal True; MARK ordinal 14 (mark_names.py); 203 IL nodes.
- Tail at MARK (lines = source lines relative to the definition): N171-N173 `if (!hardware_format)` jcc -> N178;
  N175 `success = FALSE`; N176 JMP (line 100) -> N182 = already THREADED past the `if (!success)` test into the error
  block; N178 label: N179/N180 `if (!success)` jcc -> N189; N182..N187 error block (hardware_format = NULL; error());
  N189 label (refs N180 only): N190 return-value assign, N191 op 0x2a8, N192 JMP -> N199; N194 label (the no-device
  arm, refs N40): N195 hardware_format = NULL, N196 return-value assign, N197 op 0x2a8; falls into N199 = exit label
  (refs N192 only); N200-N202 epilogue/ret.
- Prediction (a) FALSIFIED: at MARK the exit label has ONE jump ref (the success return's JMP N192); the no-device
  arm reaches it by FALL-THROUGH, so the step-#2 END-list pair rule (>= 2 JMP refs on one label) cannot by itself
  decide this binding. The final two epilogue copies arise after MARK (the success path's JMP is replaced by an
  epilogue copy, then the `if (!success)` skip branch N180 is bound to the copy after the no-device arm in ours,
  to the copy after the error block in January). (b) not reachable from this observation.
- STOP (bounded; zero compile shapes spent): the deciding step is a later cross-jump/epilogue pass (candidates:
  0x107483d6 -> 0x107487cf, "second step-#2 route, rule not decoded" in the S3-3 memory). Reopen criterion
  (sharpened): trace which pass retargets N180 from N189 to the no-device copy and decode its survivor rule; only then
  look for a source construct.

### Not attempted / bounded without compiles  (2026-09-26 01:59:13 -0700)
- `_triangle_coplanar` (380): no new first-party evidence; Lane D stop rule (x87 facing-dot term order; January
  yzx/ikj outside both measured paren-count cycles). Hypothesis for a future reopen (NOT tested): L1/L2 show every
  parenthesised FP subexpression and named-float assignment creates a C2 0x267 node with its own temp web - a
  plausible carrier of Lane D's "FP paren nodes mod 4" effect through the decoded x87 temp-slot sort (mod 4).
- `_sound_cache_debug_render` (368): structural, not a tie (January 119 vs ours 126 instructions; January keeps
  state_index and page_index memory-homed and uses two pointer IVs in the point loop; frames differ). Recorded
  declaration/scope negatives cover the natural permutations; no new evidence -> 0 shapes.
- `_lightning_submit` (2,571): T4 only (block-membership finding); 0 shapes.
- `_rasterizer_bitmap_new`: relocation delta asked by the lead = NAMING only: January's split carries the pooled
  `??_C@_06PCHFJCOP@bitmap?$AA@` (and the "unsupported bitmap type" literal) as an undefined `symbol:` reference
  (csplit gives the pooled literal to another object) while ours defines the identical literal locally
  (`defined-noncode:.rdata`); same identity, same offsets; object_audit lists both as candidate-only surplus.
  The one real byte is the `jne` target (T5).

## FINAL SUMMARY (2026-09-26 01:59:13 -0700)
- EXACT: `@periodic_function_build_variable_period_x_table@4` 256 padded / 244 meaningful (P3). periodic_functions
  7/7 + full battery PASS -> admission candidate. Park entry to be retired by the lead after fresh verification.
- Zero-credit landable cleanups: path_structure_bsp (P1c, object identical; admission battery PASS; owner E1-Q1),
  hardware_geometry (H1 + lead symbols.json proposal).
- Owner packets: E1-Q2 find_or_add_edge (C1), E1-Q3 find_or_add_vertex, E1-Q4 dead_camera; E1-Q5 no change.
- Mechanisms (new, all trace-backed, stock-equal sealed runs): C2 0x267 FP barrier node (parens / named-float
  assignment) occupies scheduler unit class 0; VC7 factors c*a + c -> (a + 1)*c with no barrier; the cinematic_render
  OR operand order = (alpha temp key + 0x61) vs ((base symbol id & 3) << 14) + 0xfe3a (16-bit), so a named
  shadow_alpha can never give January's order.
- FAILED PREDICTIONS (all retained above): T1(a) same-cycle readiness; L1 H-PAREN (parens only); L3 no-factoring;
  P1b x87 compare canonicalisation; T3c base-only difference (alpha id also differs); T3e id mod 8 via <<5 (actual:
  storage-3 <<6, mod 4); T4(a) unit refusal inside one block (actual: different blocks); T5(a) exit-label list with
  the no-device JMP at the head (actual: one JMP ref, the no-device arm falls through).
- Harness errors disclosed: P1 first compile had collapsed backslashes (heredoc); sort_trace segment bug -> two
  sealed LAB runs (e1_t3c_min0b/c) deleted and re-run under the same labels.
- Card coverage note: runs e1_t3d_min3c (lab_min3, planned in card T3c) and e1_t3d_min4c (lab_min4, an extension
  added after T3c's lab_min0 result, WITHOUT its own card) - disclosed; both are observation-only lab traces.
  mark_names runs (e1_marks_lab10, e1_marks_hwbitmaps) are ordinal utilities with no prediction.
