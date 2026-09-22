# action_charge worker report (Lane B)

Baseline: `== exact 21  residual 1  unwritten 0  (of 22 listed)`; `_action_charge_perform [size 3216!=3248, sha]`.
January: frame 0x38, 4 ret, 982 insns. /Od twin: fn 0x430170 (dump od_perform.txt).

## try_1 melee-first arm order (Astra research8 shape, re-measured)
Evidence: /Od 0x43027e..0x43028d tests goal==2||goal==3 and falls into the melee body (0x430293) first; January +0xa1..+0xaf `cmp ax,2; je; cmp ax,3; je` with non-melee laid first = the `if (A||B) X else Y` else-first layout law.
Gate: before `[size 3216!=3248, sha]` -> after `[size 3232!=3248, sha]`, 982/982 insns. check_range->[ebp-2], berserk_ranges->[ebp-1], BL across game_time_get now match. Non-melee arm regresses (0x164 prop/definition reload, 0x1b6 range stores sunk past join).
Verdict: kept as working base only if a second attested fact repairs the non-melee arm.

## try_2 = try_1 + `if (!actor_has_ranged_weapon(actor_index) || actor->input.underwater) advancing = TRUE; else {ranges}`
Evidence: /Od 0x43074c..0x430786: has_ranged `je 0x430773`, underwater bit `je 0x430786`, the advancing=TRUE body at 0x430773 is laid BEFORE the ranges body (0x430786) => the TRUE arm is the `||` then-arm. January +0x18a..+0x1a3: `je 0x88` / `jne 0x88` to the shared advancing=TRUE tail with ranges laid inline = else-first law, consistent.
Gate: `[size 3232!=3248, sha]` -> `[size 3216!=3248, sha]`, 982/982 insns, BUT the first 475 instructions (+0x0..+0x695: the whole decision half incl. +0x164 reloads, +0x1b6 sunk range stores, +0x256, check_range/berserk cells, BL across game_time_get, `cmp word [edi+4],2`) are now byte-identical. Real diff 399 -> 285 lines; first diff now +0x696.
unitcheck: NO CHANGE, regressions=0. Promoted to candidate.c.

## try_3 dot arg order `dot_product3d(&prop->actor_to_prop, &direction)` (/Od 0x430bb1 pushes &vector_to_target first)
Gate: `[size 3216!=3248, sha]` unchanged; real diff 285 lines identical -> INERT. January +0x699 `fld dir.k; fmul [prop+0xe8]` is the natural out-of-line order with a=&direction, so baseline order kept. Not adopted.

## diag_1 M8 instrument (D=1..7 dead `long x = 0;` before `prop`) on try_3 - DIAGNOSTIC ONLY
All seven: `[size 3216!=3248, sha]`, real diff 285 lines, same first diff +0x696 -> INSENSITIVE. The x87 leaf-order residual (ours j-first at all 3 inline dots; January k,j,i / i,k,j / k,j,i) is not an IL-local-count key.

## diag_2 (DIAGNOSTIC) try_2 + 8-byte struct copies `facing_direction = *(real_vector2d *)&direction;` / `melee_direction = ...`
Result: all three inline dots flip from ours j-first to January's natural k,j,i / i,k,j / k,j,i. Cause: scalar float reads `direction.i/.j` made direction.j a CSE value that Sethi-Ullman schedules first. January has no scalar reads of direction's components.

## try_4 = try_2 + `__inline` on the 11 local helper definitions (w3c `p1` linkage lever, re-based)
Evidence: January +0x6c8/+0x707 reload prop (`mov eax,[ebp-8]`) and use folded [eax+0xe0..e8] in BOTH direction copies; ours held edx=prop+0xe0 ACROSS `call _normalize3d` (legal only because a non-COMDAT-pick-any body in the same TU has a known clobber set) and cross-jumped the two copies = the whole 32-byte size gap. Inline (select-any) linkage removes that knowledge. January atlas marks these helpers inline COMDATs (w3c ledger).
Gate: `[size 3216!=3248, sha]` -> `[size 3264!=3248, sha]`; real diff 285 -> 177 lines; everything up to +0x699 identical, both direction copies now identical; remaining = j-first dot artifact + tail register names.

## diag_5 (DIAGNOSTIC) = try_4 + the two 8-byte struct copies of diag_2
Gate: `residual 3248 _action_charge_perform [sha]` (SIZE NOW EQUAL, 982/982 insns). Real diff = 20 lines, two items only:
 (1) +0x944/+0x956 `fld [edi+0x34]; fadd [eax+0x37c]` (January: state_data leaf first) vs ours definition leaf first;
 (2) tail `unreachable` byte cell January [ebp-0x14] vs ours [ebp-0x18].

## diag_5b M8 instrument at function top on diag_5 (D=1..7) - DIAGNOSTIC
D=1..4 identical to diag_5; D=5..7 additionally break dot #2 (+0x8e1). No D flips +0x944 or the tail cell -> neither residual is a uniform IL-ordinal shift.

## diag_6 swap source operands `definition->berserk.melee_attack_range + state_data->melee_danger_range` (both statements)
Byte-identical to diag_5 -> source operand order INERT (agrees with LAW M8 text). /Od 0x431057 attests state_data first; baseline spelling kept.

## diag_7 /Od declaration facts on diag_5 (no `unit_index` local; `direction` before `debug_info`; `distance` declared in the else-arm after `unit`)
Byte-identical to diag_5 -> INERT (declaration order/scope dead, as the brief says). Not needed.

## diag_8 /Od form of the tail: no `target_range` local (`state_data->acceptable_target_range = find(...)`; `MAX(minimum_move_range, state_data->acceptable_target_range)`)
Byte-identical to diag_5: VC7 makes the same [ebp-0x14] CSE temp (`fstp [ebp-0x14]; mov eax,[ebp-0x14]; mov [edi+0x2c],eax; ... fcom [ebp-0x14]`). INERT for both residuals.

## diag_9 declare `state_data` before `definition` - fadd unchanged, prologue `lea edi` breaks -> declaration ordinal is not the key. Rejected.

## diag_10 = diag_5 + the other two 2D sites as 8-byte struct copies (`state_data->leap_alignment_vector = *(real_vector2d *)&aim_vector;`, fallback `melee_direction = *(real_vector2d *)&actor->input.facing_vector;`)
Evidence: /Od 0x430f04 (one state_data load, two dword moves) and 0x431203 (one actor load, L,L,S,S) are SINGLE statements = 8-byte struct copies; /Od never reloads a base pointer inside one statement.
Gate `[sha]`, size equal. +0x944/+0x956 fadd order now MATCHES, but the three dot leaf orders regress to j-first -> a count key.

## diag_10b M8 instrument on diag_10 (D dead locals at top) - DIAGNOSTIC
D=0..3: dots wrong. D=4,5,6,7: dots AND fadd all match; only the tail `unreachable` cell ([ebp-0x14] vs ours [ebp-0x18]) remains (9 diff lines). => our IL-local count is 4..7 (mod 8) short of January's.
/Od-attested locals we lack: real at [ebp-0xf4] (dot #2 result), real at [ebp-0xf8] (dot #3 result), real at [ebp-0x110] (dot2d result), boolean at [ebp-0x8d] (projectile_aim_ballistic result), boolean at [ebp-0x3e] (dead, =0 after `melee`). /Od lacks our `unit_index` and `target_range`.

## diag_11 = diag_10 + three named real locals for the dot results (/Od slots -0xf4,-0xf8,-0x110)
Gate `[sha]`; dots wrong AND fadd wrong (50 lines); instrument on it: no D ever repairs the fadd. => January does NOT have these named reals (2020-only). Rejected.

## diag_12 / diag_13u / diag_13t = diag_10 minus `unit_index` and/or `target_range` (/Od has neither; each byte-inert alone, diag_7/diag_8)
All three: `[sha]`, dot #1 OK, dot #3 OK, fadd OK; only dot #2 (+0x8e1: ours k,j,i vs January i,k,j) + tail cell remain (23 lines).

## diag_14 = diag_12 + /Od shape of the unreachable test (saved as owner_ruling_struct_copy.c)
Evidence: /Od 0x4315c9..0x43160c has TWO `unreachable = TRUE` stores: `if (out_of_range) { if (unable_to_advance || !actor_path_has_path()) unreachable = TRUE; else if (target_radius > acceptable_target_range) unreachable = TRUE; }`. January +0xc17..+0xc3f is the cross-jumped single store, consistent.
Gate: `residual 3248 _action_charge_perform [sha]`, 982/982 insns. Tail cell FIXED ([ebp-0x14]). ONLY remaining diff = dot #2 term order at +0x8e1..+0x8f8 (6 rows permuted, 0 bytes size delta).
Instrument on diag_14 (D dead `long` at top, DIAGNOSTIC): D=0,1 dot #2 wrong; D=2..5 all dots wrong; **D=6 and D=7 => `EXACT 3248 _action_charge_perform`**.
Census: diag_14 declares 38 locals; the /Od frame has 45 named slots (= +7): dead boolean [-0x3e], boolean [-0x8d], alignment_vector [-0x88], target_point [-0xac], reals [-0xf4] [-0xf8] [-0x110]. The three reals are counter-indicated (diag_11); the rest cannot be written as live January code (no extra normalize2d calls / adjusted target point exist in January's bytes), so no admissible +6/+7 set is known. NOT landed; dead locals are the instrument, never the landing.

## try_6 = try_4 + the /Od two-store unreachable shape  => FINAL candidate.c (no punning casts)
Gate: `residual 3248 _action_charge_perform [size 3264!=3248, sha]`; `== exact 21 residual 1 unwritten 0 (of 22 listed)`; unitcheck regressions=0; newsyms OWNERSHIP PASS (0 new, 0 lost); fake_match_scan 0 leads; _point_from_line3d guard reports exactly the one genuine 48-byte owner (same single hit as the real source; row EXACT).
Real diff 399 -> 172 lines: +0x0..+0x698 identical (475 insns), both direction copies identical. Remaining = the scalar-read artifact (j-first dots x3, fadd order, +0x9ec copy/CSE, tail register names), all cured only by the 8-byte struct-copy spelling.
STRIP TEST: removing `__inline` => try_2 family (edx held across `call _normalize3d`, copies cross-jumped, size 3216): linkage is load-bearing and January-proven. Removing the casts from owner_ruling_struct_copy.c => this candidate (dots/fadd regress): the cast copies are load-bearing, /Od-attested single statements => NEEDS OWNER RULING (brief bans `*(real_vector2d *)&v3`; precedent exists in source/ai/actor_looking.c:623,645,646).
Fallback without `__inline`: try_2.c (`[size 3216!=3248, sha]`, 285 lines).
