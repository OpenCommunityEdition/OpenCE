# hud_weapon worker ledger (wave 4, base 16542e46)

Slug: scratch/w/hud_weapon/. Unit: source/interface/hud_weapon.
Missing: _hud_update_weapon_local_player (1472), _crosshairs_draw (2256), _render_weapon_hud (2656).

## Baseline (production, gate --all)
residual 2256 _crosshairs_draw [relocs 77!=76, sha]
residual 1472 _hud_update_weapon_local_player [size 1456!=1472, sha]
residual 2656 _render_weapon_hud [size 2624!=2656, relocs 85!=84, sha]
== exact 13 residual 3 unwritten 0 (of 16)

## Prior art read (do not repeat)
- opus5 150k w1/w3 ledgers; private_graph_fuzzy; fable crosshair x3; astra wave9 packet 9 (render_weapon_hud hold)
- Lane C CC4 FINDINGS (claude-lane-c-graphics-ui-20260920/scratch/research/CC4-hud-weapon): E7a no_grenades &&; E7b fast_ftol decimal
  (closes reloc 85->84); E7c = {0} arrays in block after csmemset; E7d union-typed number_fractions (frame 0x288 vs 0x284:
  magazine_size spill).
- Structural lane (claude-structural-20260923/scratch/lane/w/interface__hud_weapon/REPORT.md): uwlp EXACT via /Od case
  order + state pointer + unit_index local + default w/o result assignment (OWNER-HELD: structural owner packet sec 7);
  render_grenade_hud g1 landed (6eb9cb76). crosshairs_draw: firing uninit read authentic (/Od 0x6363e0) held; c2r ruling
  variant 2224/74 still fuzzy. render_weapon_hud r3u 2672 fuzzy; /Od local-count variants inert.

## P0 (re-measure) uwlp: structural-lane body spliced into production -> s1.c
gate: == exact 14 residual 2; _hud_update_weapon_local_player EXACT on this tree (no other row moved).
January proof of the uninitialised `result` web: T+0x144 `mov esi,[ebp-0x28]` loads return_eip's home into ESI
(result's register) at loop entry - result's home is never stored, so the frame packer merged it with return_eip's
slot; the default arm (T+0x404 assert, falls to +0x424 `test si,si`) reads it. /Od 0x638ac2..0x638af3: default arm
has no store and falls to the read at 0x638af3; NO _RTC_UninitUse for `result` (RTC descriptor lists only the two
64-byte arrays). => exact form is the held owner class (read reachable only if system_exit returns). Not landable.

## P1 render_weapon_hud re-measure of donors on this tree
- CC4 hw_v4 (production body + E7a-d): 2624/84 sha (size -32).  structural r3u (w1 R3 + union array): 2672/84 sha (size +16),
  876 vs 871 insns. r3u residual = pure allocation: frame-slot permutation (definition T-0x1c/O-0x14, element_index T-0x18
  homed vs O statics-loop ESI), flash-loop ESI/EDI swap, statics/numbers/overlays loop latch registers, x87 distance sum
  (T j2,i2,add,k2,add vs O k2,j2,add,i2,add). Every other instruction identical.
- B.c = s1 (structural uwlp, +2 locals before rwh) + r3u rwh: rwh byte-identical to r3u (sha b420177a...). +2 names inert.

## P2 oracle sweeps on B.c render_weapon_hud (LAB ONLY; fillers never land)
- names before rwh prototype k=0..40: all byte-identical (b420177a). names right before rwh DEFINITION k=0..40: identical.
- dead locals (`long x;`) D=0..16 in rwh: identical. initialised copies (`long x = hud_index;`) D=1..9: one flip (param-home
  reuse, 2656/84 but 159 hunks) then identical for all D -> count inert.
- pow(10.0f, 4) (the /Od 0x632150 float,int overload = direct pow, not real_math power()) vs power(): byte-identical.
=> none of the three residual ties (statics-loop allocation, flash-loop IV regs, x87 sum order) is keyed on TU name count
   or in-function local count.
- x87 micro-lab (scratch/w/hud_weapon/micro): plain distance3d gives three different sum orders (kij/kji/jik) depending on
  inline helper expansions AFTER the site (after_mag1-2 kij, after_mag3+ kji; after_dot1-8 kij; after_dist1+ kji);
  plain statements before/after, dead locals, names, param count, arg order, decl order all inert. So the order is keyed on
  later inline-helper content in the same function (not M8 dead-local count). January rwh = jik (same as ctf_init site 1).
- rv_V1/V1b: flash-loop counter declared in the outer block (before/after map/element_index; /Od has it at -0x80 right after
  numbers_real): byte-identical to B (b420177a). Inert.
- Harness rv.py prints the three tie signatures. January = x87 jik, flash[hud=esi flags=edi], statics lea-esi, frame 0x284.
  B/r3u = x87 kji, flash[hud=edi flags=esi], statics other.
- micro-lab refinement: the x87 order flips with the operands' frame placement (mag3 moved position nearer EBP) - it follows
  frame/reference facts of the aggregates, not local counts. In rwh both points already sit at January's slots.
- prototypes (`void p(void);`) k=0..34 right before rwh definition: all identical (H3-style prototype-unit key inert too).
- K1: `cutoffs` pointer local replacing definition->flash_cutoffs.* in the else block: byte-identical (VC7 folds).
- A1: `long age_percentage` local for the two overlay PINs: byte-identical.
- M1/M2 (/Od idiom `scenario = global_scenario_get(); map_type_flags = 0; SET_FLAG(map_type_flags, 0, ...)`, /Od 0x63b40f
  and crosshairs /Od 0x636452): byte-identical to B (VC7 folds to setne).
- S1 (state_index local in statics loop): identical.
- BISECTION r3u(B) x hw_v4 else-block segments (hyb.py): only the hw_v4 OVERLAY section (flags local copies, dword ops)
  flips the statics loop to January's allocation (lea esi,[edi+0x60], element_index homed) - but it breaks the overlay
  bytes (size 2624). hw_v4/production statics loop == January's. So the statics-loop allocation IS source-reachable and
  keyed on upstream else-block content, not counts.

## P3 allocator trace (Lane A dbg32c copied into scratch/w/hud_weapon/dbg; runs our own CL/C2 under INT3, outputs only in slug)
- lab_min.c = B with every body except get_hud_state + rwh removed, rwh last, plus a trivial caller: rwh byte-identical
  (b420177a). Trace (res_min.txt / show.py) lists rwh webs 11..78.
- Flash loop webs: p_hud W=75a8 pri 46 pos 110 -> EDI (cost esi=1, edi=0), p_flags W=75f0 pri 28 pos 111 -> ESI,
  counter W=c740 pri 16 pos 10f -> EBX. Pop order is by priority; the EDI choice is the chooser's used-register cost
  (EDI already used by the first-popped web W=c620 pri 204 pos 2d1, the numbers-loop element; EBX still unused = 4200).
  January's p_hud=ESI therefore needs ESI cheaper than/equal to EDI when p_hud is coloured, i.e. a different set of
  higher-priority webs (upstream IL fact), not a flash-loop spelling. lab_F1 (inverted flash if/else) stays hud=edi.
- hyb_00100 trace (res_hyb.txt): the hw_v4 overlay `flags` webs (pri 63-90) are popped before the flash loop and take ESI;
  from then on the chooser's ESI cost is 0 (tie with EDI -> class order ESI), so p_hud=ESI (January) and the statics-loop
  &statics web (pri 34) also takes ESI (January). In ours (res_min.txt) a persistent +1 ESI cost (neighbour preference
  from long-lived param webs W=e298/e208, pos 2, coloured last with ESI) makes EDI win for p_hud (#28) AND for the statics
  &statics web (#32). => the flash-loop and statics-loop residuals share ONE cause: the ESI-vs-EDI tie in the chooser
  (neighbour preference weight 1). January's source must differ upstream in something that either colours a
  high-priority web into ESI earlier or removes the ESI preference of the long-lived param webs. Not found.
- LAB G1 (get_hud_state made non-static, cdecl push instead of the private ESI-argument ABI): flash loop becomes
  hud=esi flags=edi AND statics loop lea-esi (both January). => the ESI neighbour-preference that flips both residuals
  comes from get_hud_state's private register-argument call (`mov esi,[ebp+0xc]; call _get_hud_state`, T+0x35).
  January has the same call bytes, so the difference is in how that ESI web is weighted (upstream).
- Full cost tables (mem idx 4=ebx,7=esi,8=edi): ours p_hud #28 esi=1 edi=0 -> EDI; hyb p_hud #33 esi=0 edi=0 -> ESI.
  The +1 ESI is a neighbour preference from a later-coloured long-lived web (param webs W=e298/e208, pos 2, coloured
  last into ESI). Could not map it to a source fact (no symbol names in the web record without a debugger rebuild).
- Definition order (January section order, reorder.py -> Bord.c): rwh byte-identical (inert, as w3 found).

## P4 candidate construction (mkcand.py; r3u body + /Od-attested inert spellings + RTC names)
| variant | rwh | note |
| c_union (=r3u) | 2672/84 b420177a, 112 hunks | TU-level union element type (CC4 E7d; invented type name; HCEX says float[8]) |
| c_od_union_names_tblock | 2672/84 b420177a | /Od: pow(10.0f,4), inline unit-index ?: arg, inline decimal arg, scenario+map=0+SET_FLAG; RTC names numbers/numbers_real/position; /Od target-block decl order - all byte-inert |
| c_od_realarr_names_tblock | 2672/84 a4f40686, 118 hunks | real numbers_real[] + admitted local-union transfers (homes the sentinel/fraction temps) |
| c_cast (lab) | 2672/84 ec333ee6 (= old R3 hash), 111 hunks | pointer-cast sentinel (inadmissible representation access) |
| production | 2624/85, 208 hunks | hand-expanded distance (rule 6), (short)fmod (extra __ftol2), flags/cutoffs locals |
All variants keep the same three residuals: x87 kji vs jik, flash hud=edi vs esi, statics loop.
- x87 order: lab_G1 (no private ABI) and hyb both still kji -> independent residual. Lab x87o_1..6 (1-6 extra
  magnitude3d inlines after the site): kji. Micro variants (frame/pointer/param args, dsq vs d3d, compare use): kij or
  other; could not reproduce January's jik outside the ctf_init first-site context. No lever.

## P5 admission side-effects of the rwh fidelity candidate (cand.c = c_od_realarr_names_tblock)
- gate --all: exact 13 residual 3 (no row lost); emitted-symbol guard (_point_from_line3d) passes; /W3 clean.
- relocation-target multiset == January (production has an extra __ftol2); relocations 0..53 at January's addresses.
- Symbol census vs build/base: drops _power, ADDS header-inline COMDATs _distance3d, _distance_squared3d,
  _magnitude_squared3d, _vector_from_points3d, _fast_ftol (January's split defines none; COMDAT folding invisible).
  => wave-3 law (i)/P1: new header COMDATs from a NON-exact caller are held. The fidelity packet is NOT landable now;
  it becomes the base to land when rwh closes. Also: its block after csmemset depends on the stack-sentinel prologue
  (hud_stack_check lane) - if that lane introduces a block-opening sentinel macro, drop the explicit block.

## P6 uwlp owner-gated packet
- owner_uwlp.c = s1.c + BUG-style comments (default-arm unassigned result; primary-trigger test in the secondary
  state, /Od 0x638a07). gate: EXACT 1472/68 (0524ed47), 14/16, guard passes, no new symbols vs base.
- owner_gated_uwlp.patch: `git apply --check` clean against 16542e46.
- object_audit with owner_uwlp.obj: FAIL(2) = _crosshairs_draw, _render_weapon_hud; 23/23 January symbols ok; bss ok;
  surplus = literals + production's _power/_square_root COMDATs. pdb_storage: 0 disagreements.

## crosshairs_draw (not re-probed beyond a re-measure; held owner item)
- cd_r0 (production + uninitialised firing_active = the ruling form): residual [relocs 77!=76], 693 vs 704 insns,
  157 hunks, frame 0x314 vs 0x318. Prior art: P5 lane-C (bind bitmap_group_get -> 0x318 frame), structural c2r
  (/Od switch shape, 2224/74). January facts re-read: aim/zoom/animated colour tails cross-jump (T+0x51c jmp 0x393),
  if-chain 18/8/9 with the state-14 read of firing at T+0x495. Needs owner ruling + a full /Od-guided rebuild.

## STOP (methodology stop rule)
rwh: >5 evidence-based shapes (pow, /Od idioms x4, flash scope x2, names, target-block order, sentinel forms x3) plus
three count oracles all inert; remaining difference is the decoded ESI/EDI chooser tie (+1 ESI neighbour preference
tied to the get_hud_state private register-argument call) and an independent x87 add-chain order.

## P7 interaction with the concurrent hud_stack_check packet (P1 header inline check_stack_buffer + P5 hud_weapon.c)
- lab/sc_*.c = stack-lane gtu_h2 hud_weapon TU + my rwh candidates: rwh byte-identical (b420177a / a4f40686) -> the rwh
  residual does NOT depend on the sentinel implementation (hand macro vs header inline).
- LAB G2 get_hud_state(0) (no local_player_index->ESI copy): flash + statics go January's way -> the +1 ESI preference is
  the local_player_index web's copy into the get_hud_state ESI argument (T+0x35 `mov esi,[ebp+0xc]`, identical in both).
- owner uwlp on top of the stack packet (after_p5/gtu_owner.c): EXACT, 14/16. owner_gated_uwlp_after_P5.patch applies
  (git apply --check --directory=after_p5) to production+P5; owner_gated_uwlp.patch applies to 16542e46 (they conflict on
  the stack_buffer context line, use the one matching the tree state).
- git status: clean throughout (no tracked file touched).

## FINAL
- _hud_update_weapon_local_player: strict EXACT only in the held authentic-bug form (default arm leaves `result`
  unassigned; owner packet sec 7 / brief sec 7). Patches ready, not landable.
- _render_weapon_hud: best candidate cand.c (reloc multiset == January, 54/84 relocs at January addresses, 2672 vs 2656);
  held by P1 (new header COMDATs from a non-exact caller). Residual = ESI/EDI chooser tie from the get_hud_state
  private-ABI copy preference (flash loop + statics loop + downstream frame permutation) + x87 add-chain order.
- _crosshairs_draw: held (authentic uninitialised firing read) and still fuzzy (157 hunks in the ruling form).
