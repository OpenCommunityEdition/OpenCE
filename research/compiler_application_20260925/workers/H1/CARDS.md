# H1 CARDS / LEDGER - render_debug item (d): independent storage evidence for the seven cache globals (owner Q14)

Started 2026-09-26 01:43:55 -0700. Worktree C:\halo-worktrees\claude-compiler-application-20260925, HEAD fe283cc5
(includes db7da71a rotate_vector2d + fe283cc5 owner prototypes). Patch-only; nothing committed. Sole writer of
source/render/render_debug for this task.

## S0 January's own bytes (no compile; tools/bssaccess.py on build/split render_debug.obj, config/contribs.json)
- PDB section contribution (contribs.json, first-party, independent of symbols.json names): module 87 owns ONE .bss
  contribution at file offset 0x4B8C40, size 0x740C, flags 0xC0400080 = align 8. Neighbours: module 89 ends 0x4B8C3B,
  module 85 starts 0x4C004C. So January's render_debug.obj had exactly one .bss section, 0x740C bytes, align 8.
- Every relocation into it (31 sites in 3 functions) and its access width:
  +0x0000 byte stores (`mov byte ptr [+0],0/al`) + address arithmetic (add/lea)      -> char array base
  +0x03FF byte store  (`mov byte ptr [+0x3FF],0`)                                     -> last char of a 0x400 array
  +0x0400 address arithmetic only (`add ebx,0x400`, `add eax,0x400`)                   -> entries base
  +0x7400 word: cmp word / mov word store / movsx ecx,word                             -> 16-bit signed lvalue
  +0x7404 word: mov ax,word / mov word store / dec word ptr / cmp word / mov word,0     -> 16-bit lvalue (RMW dec word)
  +0x7408 word: mov ax,word x5 / mov word store x2 (incl. mov word,0x3FF)             -> 16-bit lvalue
  +0x740A byte: mov al,byte / mov byte,1                                               -> 8-bit flag
  +0x740B byte: mov al,byte / mov byte,1                                               -> 8-bit flag
  NO relocation touches +0x7402..0x7403 or +0x7406..0x7407.
- cachebeta publics: no public symbol anywhere in 0x4B8C40..0x4C004B (pdb_storage: January static).

## L1 outcome (2026-09-26 01:55 -0700) - card cards/L1_vc7_bss_bucket_laws.txt - ALL PREDICTIONS CONFIRMED (P4 incl.)
Lab: tools/bsslab.py + l1.py/l1b.py/l1c.py/l1d.py, production VC7, render_debug cflags, random neutral names
(objects in scratch/campaign/workers/H1/lab/, logs L1_log.json, L1b_log.json, L1c_pool.json).
- P1 one .bss section in every one of 100 TUs (uninit-only, zero-only, 60 random mixes).
- P2 uninitialised statics precede every '= 0' static in 60/60 random mixes with random names (0 violations).
- P3 '= 0' statics: declaration order in 80/80 TUs; all-'= 0' in January's member order -> January's exact offsets and
  size 29708 for 20/20 random name draws (name-INDEPENDENT).
- P4 per-object alignment is the SAME rule in both buckets: short -> 4-byte slot, boolean -> 1, char[1024] -> 8,
  struct[512] -> 8 (uninit examples: short@0x404 after byte@0x403, entries@0x418 after byte@0x410, char[1024]@0x8
  after short@0x0). The uninit-lab sizes 29706..29720 are ORDER effects, not a different alignment rule.
- P5 section align 8 in every TU (both buckets).
- NEW: the uninitialised order is a TOTAL ORDER BY A PER-NAME KEY (150-name pool; 12 random 12-name subsets in
  shuffled declaration order: 12/12 consistent). Any permutation is therefore reachable by some names.
- NEW (decisive for Q14): seven UNINITIALISED statics whose (neutral) names happen to sort in January's order
  (tt0k9m7, sfr7pku_gau0f, f3h6b6g1w, j_pa, fpkdod, ww5y3uft6, vbnil) reproduce January's offsets, size 29708 and
  align 8 EXACTLY, in 3 different declaration orders (L1d). => January's layout does NOT establish the '= 0'
  initialisers. The owner's objection is confirmed by measurement.
- Name-independent residue: January-compatible bucket assignments are exactly the 8 "prefix" forms (the first k of
  strings, entries, game_time, entry_count, string_offset, entry_overflow, string_overflow uninitialised, the rest
  '= 0', k = 0..7). k = 0 is D1; k >= 2 needs the k unknown names to sort in January's order.

## L2 outcome (2026-09-26 02:23 -0700) - card cards/L2_od_build_bss_law.txt - Q1 FAILED, Q2 FAILED, Q3 not reached
/Od facts (name-independent, first-party, halo_cache_symbols.exe as data; tools/odpe.py, odrange.py, odfuncs.py):
- The seven are at SEVEN SEPARATE addresses in three clusters, with foreign objects inside the span:
  F0AFB4 game_time (movsx/mov word; `cwde` compare with game_time_get), F0AFB8 entry_count (word; `cmp eax,0x200`),
  F0AFBC string_offset (word, movsx), F0AFC0 entries (`imul edx,ecx,0x38; add edx,0xF0AFC0`), F11FC0 strings
  (RTC range check `cmp idx,0x400`), F13AF6 entry_overflow (byte, movzx), F13AF7 string_overflow (byte, movzx).
  hs toggles debug_bsp/debug_input sit at F0AFB6/F0AFB7, BETWEEN game_time and entry_count; strings FOLLOWS
  entries (reverse of January); 0x1736 unreferenced bytes separate strings from the two flags.
  => in the 2020 build these are not one aggregate (an aggregate is contiguous and cannot contain foreign objects).
- Rich header: 508 objects by Utc1900_CPP build 24234, 71 by Utc1900_C: the Halo .c files were compiled as C++
  (INFERRED from the counts; consistent with the toggles - COMMON/tentative in January - being ordinary definitions
  inside render_debug's block in 2020).
- Modern-compiler lab (MSVC 14.51, the only modern MSVC installed; the /Od build's 19.00.24234 is NOT available):
  /TC and /TP, /Od: layout is IDENTICAL for 12/12 random initialiser masks and random renames (bucket- and
  name-inert); statics are placed in declaration order with first-fit hole filling (booleans fill the 2-byte hole
  after a 4-slot short), externals before statics. It does NOT reproduce the /Od arrangement (Q2 failed: 14.51
  puts the six real toggle names in declaration order, not /Od's order). The /Od placement therefore carries NO
  readable bucket information with the tools available.
- /Od cross-map (tools/odindex.py + janfn.py + crossmap.py: January function -> /Od function by distinctive shared
  literals, statics by address intersection): objects.c (object_name_list, object_memory_pool, object_globals),
  periodic_functions.c (3) and hs.c (5) come out in the EXACT REVERSE of January's declaration order; decals.c does
  not fit one simple law (2020 decals differs: decal_geometry is 0x1000 B there vs 0x7804 in January).

## L3 outcome (2026-09-26 02:26:48 -0700) - card cards/L3_sapien_vc71_cross_compiler.txt - R1 FAILED as stated, R2 held
- tools/l3_census.py (HCEX.pdb -g dump -> 7,9xx authentic data names; January objects with >=2 authentic .bss
  statics uniquely located in Sapien): 9 TUs, scratch/campaign/workers/H1/L3_census.json.
- R2 held: rasterizer_lights, draw_string (January order == VC7 hash order) keep it in Sapien.
- R1 as stated FAILED: 5 TUs whose January order is NOT the hash order keep January's order in Sapien
  (ai_communication n=4 [1/24 by chance], game_engine, player_queues_new, hud, motion_sensor) -> VC7.1 does keep a
  declaration-order bucket; H-a' ("VC7.1 hashes every zero-initialised static") is REFUTED.
- But objects.c (3) and periodic_functions.c (2) move to EXACTLY the VC7 hash order in Sapien.
  Split by linkage: the 4 TUs that keep order are EXTERNAL '= 0' definitions (ai_communication, game_engine,
  player_queues_new, hud) plus motion_sensor (static '= NULL' + static '= 0.f'); the 2 that move are STATIC '= NULL'
  and STATIC '= { 0 }'. New hypothesis H-c recorded in card L4 before testing.
- Earlier builds (no compile): Aug-2001 1749betaP.xbe has strings, entries (0x3800 = 256 entries), game_time,
  entry_count, string_offset, entry_overflow, string_overflow with the same slots; Sept-2001 cachebeta.xbe and
  Oct-2001 oct-betaP.xbe are identical to January (0x7000 entries). oct-default.xbe has no render_debug.
  9210 == 9254 hash, so the 2001 builds cannot separate the buckets.

## L4 outcome (2026-09-26 02:28:27 -0700) - card cards/L4_vc71_static_zero_law.txt - S1 HELD, S2 FAILED (H-c refuted)
- tools/l4_census.py over all January objects (17 TUs with >= 2 .bss owners uniquely located in Sapien;
  scratch/campaign/workers/H1/L4_census.json). Order Sapien vs January:
  EZ-only: ai_communication(4), game_engine, hud, network_client_manager -> all SAME (S1 held).
  SZ-only: player_queues_new, hs, rasterizer_lights, rasterizer_xbox_profile, render_cameras, render_objects,
    motion_sensor (SZ+SF) -> SAME (7 of 7; (1/2)^7 = 1/128 under H-c); periodic_functions(3), objects(3) -> DIFFERENT.
  mixed: ai_debug (SU EZ EZ), recorded_animations (SU EZ SZ) -> SAME; decals (SU SU SZ SZ) -> DIFFERENT (its 2004/
    2020 source differs: decal_geometry size; its January static names are not authentic).
- => VC7.1 keeps VC7's two-bucket law with the SAME hash (objects/periodic land exactly on the VC7 hash order of their
  HCEX-authentic names). The objects/periodic moves are SOURCE changes between 2002 and 2004 (their '= NULL'/'= {0}'
  initialisers are absent in the 2004 source: January's order is not the hash order of the authentic names, so
  January had them; Sapien's order is exactly that hash order).
- Consequence for render_debug (INFERENCE, not proof): with the same law and hash in both builds, the Sapien swap
  (entries before strings, everything else identical) is itself a 2002->2004 SOURCE change affecting only the two
  arrays. Two single-change explanations, assuming unchanged names:
  (a) January declared strings, entries WITH initialisers (declaration order), 2004 dropped the arrays'
      initialisers so the hash (entries < strings) ordered them - the mechanism observed in objects.c/periodic.
  (b) January left both arrays UNINITIALISED (hash strings < entries under January's names) and 2004 ADDED
      initialisers with a declaration order entries, strings - no observed precedent for adding.
  Neither is a witness for the five scalars' initialisers.

## L5 outcome (2026-09-26 02:28:55 -0700 card) - cards/L5_flag_static_local_form.txt - T1 and T2 CONFIRMED
- Five '= 0' file statics + the two flags as '= 0' STATIC LOCALS (?fx@?1??f_entry@@9@9 / ?gy@?1??f_string@@9@9):
  with f_entry defined before f_string the .bss is January's exactly (0x740C; flags at +0x740A/+0x740B); with
  f_string first the flags swap. Uninitialised static locals hash to the front (0x7412, wrong).
- => January's bytes do NOT decide file-scope vs function-scope for the two flags (each is used by exactly one
  function in January). D1's file scope for them is as unwitnessed as its initialisers.

## L6 outcome (card cards/L6_sapien_scenarios.txt, 2026-09-26 02:32:54 -0700) - ALL PREDICTIONS CONFIRMED
- NEW (no compile): January, Sapien (\halopc\haloce\source\render\render_debug.c) and the 2020 /Od build carry the
  SAME 58 render_debug.c assert line immediates (0xDB..0x652, value for value): the three texts are line-aligned
  through line 1618, so every 2002->2004->2020 difference is line-neutral.
- VC7 lab (pool-ordered neutral names; VC7.1 == VC7 law per L4): (a) Jan all '= 0' + 2004 drops the arrays'
  initialisers -> Sapien layout exactly; (b) Jan arrays uninitialised + scalars '= 0' declared entries-first + 2004
  adds '= { 0 }' -> Sapien exactly; (c) Jan all '= 0' + 2004 swaps the two array lines -> Sapien exactly;
  (d) Jan all uninitialised + 2004 adds an initialiser to A, B or both -> NOT Sapien (arrays move behind the
  scalars: 0x7010/0x0..., 0x0/0x410..., 0x10/0x410...).
- Reading (INFERRED, conditional): if 2004 did not RENAME an array, every one-edit route from January to Sapien needs
  January's game_time, entry_count, string_offset, entry_overflow_reported and string_overflow_reported in the
  '= 0' (declaration-order) bucket; January's arrays stay undetermined (k = 0, 1 or 2 uninitialised prefix).
  An all-uninitialised January survives only with a 2004 array rename, or as a ~2/5040 coincidence if VC7.1's
  hash differed after all (objects+periodic make that 1/36).

## L7 outcome (2026-09-26 02:36:04 -0700) - card cards/L7_D1_reverify_at_HEAD.txt - ALL PREDICTIONS CONFIRMED
- A2's OWNER_D1 pair still passes `git apply --check` at fe283cc5 (both files). Lab copy
  scratch/campaign/workers/H1/d1/render_debug_D1.c (HEAD + D1 source patch), object rd_D1_head.obj:
  vs A2's csplit emulation split_RD1: 36/36 EXACT; object_audit PASS; pdb_storage 69 split symbols, 0 disagreements;
  .bss 0x740C align 8, offsets 0/0x400/0x7400/0x7404/0x7408/0x740A/0x740B.
  vs unmodified build/split: 33/36 (the 3 cache functions: relocation-name only, sizes equal) - source and
  symbols.json must land together, as A2 recorded. keyed_diff vs production: 106 -> 106 sections, 3 changed
  (relocation target names), _render_debug_globals -> _render_debug_cache_strings (29708 B section owner).
- No NEW packet: the evidence does not support a different storage form (see verdict).

## EVIDENCE TABLE (final, 2026-09-26 02:36:04 -0700)
Sources: J = January's own bytes/relocations/contribution (build/split, config/contribs.json, cachebeta publics);
L = VC7 lab with neutral/varied names (L1, L5, L6); S = HEK sapien.exe 2004 (VC7.1 13.10.3077, data only);
O = halo_cache_symbols.exe 2020 /Od (19.00, C++ compile, data only); A = Aug-2001 1749betaP / Sept-2001 cachebeta /
Oct-2001 betaP XBEs (data only); T = 58 identical assert line anchors J == S == O.

| global (D1 name, INFERRED) | separate object | size/type/width | linkage | relative order | bucket ('= 0'?) |
| strings  | INFERRED: its position relative to entries/scalars differs J (strings,entries,scalars) vs S (entries,strings,scalars) vs O (scalars,entries,strings) in line-aligned texts (T); J fits separate statics with no invented member (L1) | 0x400 char: J offsets + byte stores at +0,+0x3FF; O range check 0x400; S, A same | static: J publics (no public in 0x4B8C40..0x4C004B) | J +0 (PROVEN) | NOT established: k = 0/1/2 all fit J+S (L6 a/b/c) |
| entries  | INFERRED (as strings) | 0x7000 = 512 x 0x38 struct, first field short: J delta, O `imul 0x38`/`cmp 0x200`, `movsx word [eax]`; Aug-2001 had 256 (0x3800) | static (J) | J +0x400 | NOT established (same as strings) |
| game_time | PROVEN in O: named external toggles debug_bsp/debug_input (hs table pointers) sit at F0AFB6/7 between it and entry_count, so it is not in one aggregate with entry_count; J INFERRED (T + L1) | signed 16-bit: J `mov word` stores + `movsx ecx,word`; O movsx; S word | static | J +0x7400, 4-byte slot (VC7 separate-short rule, L1; also in S, O) | INFERRED '= 0' (L4+L6, conditional on no 2004 array rename); not proven |
| entry_count | not in one aggregate with game_time (O, PROVEN); vs string_offset/arrays INFERRED (order differs J/O) | signed 16-bit: J `dec word`, `mov word,0`, cmp+jge/jl/jle; O movsx, cmp 0x200 | static | J +0x7404 | INFERRED '= 0' (same condition) |
| string_offset | INFERRED (contiguous with entry_count/entries in O, 4-byte slots in J/S/O) | signed 16-bit: J word loads/stores, cmp 0x3FF + jge; O movsx | static | J +0x7408 | INFERRED '= 0' (same condition) |
| entry_overflow_reported | INFERRED strongly: in O 0x1736 unreferenced bytes separate the flags from strings, in J/S they follow string_offset directly | 8-bit: J `mov al,byte`/`test al,al`/`mov byte,1`; O movzx -> unsigned char (boolean) | static; FILE vs FUNCTION scope NOT established (L5: '= 0' static local in add_cache_entry reproduces J if that function is defined first) | J +0x740A | INFERRED '= 0' (same condition) |
| string_overflow_reported | INFERRED strongly (as entry_overflow_reported) | 8-bit, as above | static; scope NOT established (L5) | J +0x740B | INFERRED '= 0' (same condition) |
Name-independent residue from J + L alone: one .bss section, align 8, 0x740C; every uninitialised static precedes
every '= 0' one; '= 0' ones follow declaration order; alignment rule identical in both buckets -> the 8 prefix forms.
An all-uninitialised January reproduces J exactly with suitably-hashing names (L1d) - J alone never proves '= 0'.

## VERDICT for owner Q14 (2026-09-26 02:37:31 -0700)
- Supported by independent evidence: seven separate storage objects (O proves game_time is not in one aggregate
  with entry_count; the flags sit 0x1736 unreferenced bytes from strings in O; the arrays/scalars change relative
  order across J/S/O although T shows the texts are line-aligned; J's 2-byte gaps are VC7's separate-short slots in
  BOTH buckets, so no invented member is needed); sizes, 16/8-bit widths and signedness (J, corroborated O/S/A);
  static linkage (J publics); January's offsets (J). D1 reproduces all of these (L7: 36/36 vs split_RD1, audit PASS,
  pdb 0/69 at fe283cc5).
- NOT established: (1) strings' initialiser - undeterminable from ANY layout (a lone uninitialised first object and
  a '= 0' first object give the same bytes, whatever the names); (2) entries' initialiser; (3) the flags' file scope
  (L5). The five scalars' '= 0' is INFERRED only (L4+L6: needed by every one-edit route from January to the 2004
  Sapien layout unless 2004 renamed an array).
- No different storage form is better supported: every uninitialised alternative for the arrays depends on the
  unknown names (L1d), which is weaker than D1's name-independent form. So no corrected D1 packet.
- Missing witness, precisely: the original declaration text of render_debug.c's cache state (the lines before the
  first assert anchor at line 219 - the 2004 \halopc\haloce\ and 2020 MCC files are line-aligned with January's),
  or failing that the original identifiers of entries and the five scalars (then VC7's hash decides each of them
  by the prefix law: an object is proven '= 0' as soon as its real name sorts before an earlier January neighbour's).
  strings' initialiser needs the text itself.
- D1's disclosure comment overclaims two points: "as in January's .bss" (January's bytes do not show separateness or
  scope by themselves) and the flags' file scope (L5). Suggested wording is in the H1 report.
