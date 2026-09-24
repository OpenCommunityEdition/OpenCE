# bitmap_group worker ledger (wave 2, base 931ed8dc)

Unit: source/bitmaps/bitmap_group. Target: close `_bitmap_group_add_bitmap` (1008 padded, 59 relocs) + the 1424-byte .data.

## Baseline (931ed8dc)
- gate --all: exact 5, residual 1 (`_bitmap_group_add_bitmap` 992 != 1008, sha).
- build/report.json: .data 1424 at 83.3998% ; .rdata 100% ; .text 83.5%.
- datasec.py: January .data #144 and ours #3 are resolved-equal=True, size 1424/1424, relocs 152/152,
  normalized sha df5c6d69f4e13ea6df9398e2b5cc5c4455410b077b5ba6763930c4dd748505cb both sides, flags equal,
  owner storage 2/2. => .data is COFF-exact; the 83.4% is the objdiff 3.3.1 '$'-literal defect class.
  Needs a single-section config/semantic_data_matches.json entry (none exists for bitmap_group).

## Prior ledgers read
bitmap_group_obj.md, _codex_add_bitmap_fuzzy_20260902, _opus5_150k_w2_20260914, _opus5_next150_n1_20260915
(+ park evidence). Recorded negatives (do NOT repeat): csmemset clear, flags local, entry-live pixels_end removal,
long return, short casts per return, declaration order, signature-store position, register, combined predicate+goto,
branch inversions, shared failure return, group_type local, z1 (decl order pixels_end first), z2 (pixels_end=0 after
descriptor init), z3 (pixels_end=0 after assert), p1 (previous_bitmap hoisted), SE2/Pa/SE3/P1/G2, SE5/D2/Z1/F1/Vc/G1.
n1 decode: residual = ZERO-PSEUDO PIN: we pin 0 in EBX (pixels_end) and spend it on descriptor zero stores + `cmp eax,ebx`
group assert; January `test eax,eax`, `xor edx,edx` x2 rematerialised. 4 bytes + 7-byte loop pad = 11-byte gap.
LAW Z corpus: "we pin and January does not" = 0 rows board-wide => pin is SOURCE-determined; our source must have a
surplus zero web / a zero-initialised local January does not have (or January's pixels_end is not 0-initialised).

## Evidence gathered
- /Od build (halo_cache_symbols.exe): NO copy of bitmap_group_add_bitmap (tool-only function; only 2 fns reference
  "bitmaps\bitmap_group.c" there). HCEX.pdb: absent; HCEX_Release.pdb has the symbol (len 0x348) but no locals.
  => no /Od or PDB-locals evidence exists for this function.
- L5b (n1 best, field renamed bitmap_data->bitmaps) spliced into the current file = c_L5b.c:
  992/59, residual [size 992!=1008, sha]; 5 siblings EXACT. Disassembly identical in structure to n1's decode:
  `cmp eax,ebx` @0xc (Jan `test eax,eax`), descriptor zero stores through bx/ebx (Jan: `xor edx,edx` x2),
  cold-block fall-through of the non-interface cube check flipped (Jan jne 0x121/ jne 0x133), param-home spill
  choices differ (+0x18/+0x20 vs Jan +0xc/+0x10/+0x14), two commutative load-order pairs in the loop.
- NOTE: our EBX is NOT a Z1b whole-function pin (it is redefined: lea ebx,[edi-1], reload, mov ebx,eax...). It is the
  pixels_end web carrying the known value 0; VC7 spends that value on the zero stores and on the group compare.

## Probes
- labA (diagnostic, NOT a candidate): `long pixels_end;` uninitialised -> frame 0x30, zero web still pinned (ESI,
  `cmp ebx,esi`). The zero web exists independently of pixels_end.
- labD5 (diagnostic): pixels_end initialised from a non-constant global -> zero web still wins ESI, width's load sinks
  after the zero stores. So the zero web beats width's web in our IL unless width is loaded early.
- corpus scans scan_localzero.py / scan_scratchzero.py: January scratch-zero `xor S,S` + multi-store is mostly
  `= {0}` aggregate fills (exact controls in saved_game_files, event_manager, ui_widget_event_handler_functions);
  no clean non-aggregate control. (Led to looking at store ORDER instead.)
- **labS1** HYPOTHESIS: January's own bitmaps.c constructors (bitmap_2d_new etc., strict exact) assign the
  descriptor in STRUCT ORDER (signature, width, height, depth, type, format, flags, ..., mipmap_count). The
  n1/w2 L5b body used January's *emitted* store order as source order (type, flags, reg.y, reg.x, mipmap, po, hw,
  base, sig, w, h, d, f), which is an output order, not a Bungie source order. Change: struct-order assignments
  (sig, width, height, depth, type, format, flags, reg.x, reg.y, mipmap, pixels_offset, hardware_format, base_address).
  RESULT: **size 1008 == 1008, 59 relocs, residual [sha] only; 346/346 insns; 18 differing insns.** The ENTIRE
  zero-pin prologue now equals January (`test eax,eax`, `xor edx,edx` x2, loads dx/si/ax/cx/di). Only remaining
  prologue diff: reg.x/reg.y store order (Jan stores y first).
- **labS2** HYPOTHESIS: Jan stores reg.y (-0x22) before reg.x (-0x24); a C chained assignment `a.x = a.y = 0;`
  evaluates the inner (y) store first. Bungie precedent for chained zero assignments: editor_flying_camera.c:699,
  observer.c:482, static_camera.c:66, items.c:671, flags.c:347 (`right.j = right.k = 0.f;`).
  Change: `new_bitmap_data.registration_point.x = new_bitmap_data.registration_point.y = 0;`
  RESULT: 1008/59 [sha], 16 differing insns; prologue now byte-identical to January through +0x96.
  Remaining: (a) non-interface cube-check cold-block layout (Jan success block fall-through `jne 0x133`, ours
  message fall-through `je 0x14f`; also reload order di/ebx in that block), (b) two commutative load orders in the
  loop (+0x270 address/pixels_offset; +0x2a9 pixel_data.size/address inside the 0x354 assert).
- labV1 (operand order `pixels_offset + (byte*)address` at the +0x270 site): byte-identical to S2 -> pointer+int
  operand order is canonicalised (confirms w2 D2). Both loop load-order rows are not source operand order.
- labS3 (success-first cube arm `type != cube || width == height` on S2): byte-identical to S2 (hash 2e0e76cb9a3d6bb5)
  -> arm orientation is canonicalised; the cold-block layout needs a different cause.
- labL2 (nested `if (pow2 ok) { if (cube&&w!=h) msg1 else pow2 } else msg3`): byte-identical to S2 -> the
  if/else nesting and orientation of the first chain are canonicalised by VC7; layout not from branch shape.
- S2 now also matches January's param-home spill choices (+0xc width, +0x10 height): the old A24 slot deltas were a
  pure consequence of the zero pin (as n1 predicted). Remaining non-loop diff = success block placed after msg1
  plus its reload order (Jan: `mov di,[ebp+0x1c]` then `mov ebx,[ebp-4]`; ours ebx first).
  HYPOTHESIS for next probe: reload order at a join landing pad follows web definition order; Jan reloads format
  before pixels_end => pixels_end's zero definition may come AFTER format's first use in January's source
  (LAW Z3: a zero init's position is hoisting-inert for the pin, but may still order webs).
- labP3 (`long pixels_end;` + `pixels_end = 0;` after the descriptor stores): 992, frame 0x30, pin returns in EDI.
  Refuted: January's pixels_end is the declaration initialiser (entry), as in S2.
- ORACLE (diagnostic only, never landable): N dummy extern decls before the function, N=0..120: only two hashes
  (2e0e76cb / ac17dff0). ac17dff0 flips ONLY the success-block reload order to January's (di then ebx); block
  placement and both loop load orders never flip. => reload order is a TU name-count tie; the block swap and loop
  load orders are NOT name-count ties.
- labV2 (`&((byte *)address)[pixels_offset]`): identical to S2 (canonicalised).
- labH1 (DIAGNOSTIC shadow header via labcl.py, tag_data.address typed `unsigned char *`; verified the shadow is
  used with an #error probe): identical to S2 -> the address type/cast node does not order the loop adds.
- LV1 (bitmap declared at resize-block scope), LV3 (space_between at if-block scope), LV6 (pixels_end =
  offset + size): all byte-identical to S2. labH2 (shadow tag_data.address typed `long`): identical.
- ORACLE by declaration KIND (extern/typedef/struct/enum/proto, N=0..40, diagnostic only): always the same two
  hashes; only the success-block reload order is a TU-count tie. Loop add orders + block swap are function-internal.
- labC1 (`!(cube && w != h)` success-first): identical to S2. Arm orientation is fully canonicalised.
- labC8 (SET_FLAG(pow2) before a separate cube `if` in the else): 992, register swap cx/dx -> refuted.
- /FAsc listing of S2 (labcl.py LABCL_EXTRA): layout is a greedy fall-through trace (interface arm -> J -> ...),
  then the non-interface arm in IL order; in ours the && cube test falls through into its THEN (msg1), January's
  falls through into the ELSE (success). msg3 (THEN of the || pow2 test) is out of line in both builds.
- labT4 (diagnostic: early `return bitmap_index;` in each failure arm): epilogues become `or ax,0xffff` ->
  re-confirms w2's single-exit law (January's `or eax,-1` is the merged phi of one return statement).
- labF1 (FALSE-first valid, TRUE set in the success arms, on S2): byte-identical to S2 (2e0e76cb). Inert.
- Diagnostics E2 (no second cube check), E3 (bigger success arm), T4 layout, D1 (duplicated success arm): the
  first-chain && cube test always falls into its THEN (msg1) in our build; none reproduce Jan's layout.
- NEW EVIDENCE: January's COFF section (= C2 emission) order is postprocess_bitmap, delete_bitmap,
  postprocess_bitmap_group, try_and_get, get_from_sequence, add_bitmap. Ours emits the three private functions
  LAST. Wave-1 law (a): C2 emits in definition order (deferral only for calls to not-yet-defined TU functions;
  none apply here). => January DEFINED the private code first, so C2 compiled postprocess_bitmap_group BEFORE
  add_bitmap. HYPOTHESIS: compiler state carried from the earlier functions (arena/IL ordinals) decides the
  remaining ties.
- labO1 (January definition order: private code section before public code): add_bitmap byte-identical to S2
  (2e0e76cb); 5 siblings still EXACT. Definition order is inert for this function (matches the memory note).
- labI1 (non-interface arm first, interface arm as else): 992, different layout -> refuted (interface-first is
  right). Even there the && cube test falls into msg1.
- CONCLUSION on the cold-block swap: 9 structural spellings (S3, C1, L2, F1, E2, E3, T4, D1, I1) never make the
  first-chain cube test fall through into the success arm; matches n1's A43 classification. No source lever found.
- Local-name oracle (K unused locals at function top / resize block / loop body, K=0..12, diagnostic only):
  same two hashes as the TU oracle -> no local-count dependence for the loop add orders or the block swap.
- labN1..N4 (diagnostic: pre-interning `pixels_offset`/`size`/`address` in a dummy struct before the includes):
  inert. Loop add operand order is not identifier-interning order.
- /FAsc of S3 shows C1's || pattern: all jumps to THEN, ELSE inline, THEN placed after; && pattern: all jumps to
  ELSE, THEN inline. January's cube test has jumps to TWO different targets (type != cube -> S, w != h -> M, fall
  into S): neither pattern. That is the signature of a nested if whose inner arms are S1/M plus an outer else S2,
  with S1/S2 tail-merged (cross-jumped) into one block.
- **labD2** HYPOTHESIS (from that signature): `else if (group->type == cube) { if (width == height) SET(pow2);
  else { msg1; valid = FALSE; } } else SET(pow2);`
  RESULT: 1008/59 [sha], **6 differing insns**; January's block layout reproduced EXACTLY (jne S; cmp si,ax;
  jne M; S at 0x121; msg1 at 0x133). Remaining: success-block reload order (TU name-count tie) and the two loop
  add operand orders.
- ORACLE on D2 (diagnostic): N=1..13 extra declared names flips the success-block reload order to January's
  (di then ebx) -> hash 89e4adcee46d2183 = only the two loop add rows left (4 insns). Reload order = TU
  name-count tie; landing it needs a GENUINE January declaration (not a filler).
- Precedent check: draw_string.c:500 `(byte *)font->pixels.address + character->pixels_offset` (exact) compiles
  ptr-as-destination with a folded memory operand; our canonical order. January's add_bitmap loop is int-as-dest.
- Shadow-header diagnostics (verified by #error): bitmap_data.pixels_offset as `long`, tag_data.size as
  `unsigned long`: both inert.
- labX1/X2 (diagnostic: integer-typed add at site 1, both operand orders): site-1 order unchanged (address->eax).
- labG1 (diagnostic: param renamed, `group` a local copy): loop orders unchanged. The loop add operand order is
  not source order, operand type, identifier order, base-variable ordinal, local/TU name count or definition order.
- AUDIT on labD2.obj: object_audit -> only `_bitmap_group_add_bitmap` DIFF (1008/1008); 174 January symbols, 0 differ;
  surplus = .drectve + 3 folded literals (`""`, "group", "default") -> provider_link PASS for all three;
  surplus_identity 0 code COMDATs. pdb_storage: `_delete_bitmap` and `_postprocess_bitmap` are NOT cachebeta
  publics (split 2 / ours 2 / PDB-public False) => file-static in January (like postprocess_bitmap_group, which
  symbols.json already marks static). Ownership fix needed: `static` in source + `"static": true` in
  symbols.json lines 1224/1225 (only local refs: the bitmap_block definition in .data).
- labA1 (`index++`), labM1/M2 (diagnostic alternative match_assert macro forms: conditional-expression and
  do/while): loop add orders unchanged. STOP RULE reached for the two loop rows: >10 evidence-led shapes/diagnostics,
  only an operand-register/load order left, no source, type, macro, name-count or order lever moves them.
- **candI** = D2 + static postprocess_bitmap/delete_bitmap + `#include "bitmaps/bitmap_group_internal.h"` (the
  object's OWN header declaring its two public functions; house rule "prototypes in the object's own header", the
  .c did not include it). The TU name-count oracle predicted N=1..13 extra names give January's reload order;
  this genuine include lands in that window: hash 89e4adcee46d2183, success-block reload order now January's.
  REMAINING: 4 instructions = the two loop add operand-register/load-order rows (+0x270, +0x2a9).
- Final diagnostics on the loop rows (all inert): shadow bitmap_data.base_address typed `unsigned char *`; shadow
  cseries.h with `byte` as a #define (verified used: it moved the name-count tie back).

## FINAL CANDIDATE: scratch/w/bitmap_group/cand.c  (= candI + one explanatory comment)
gate: `_bitmap_group_add_bitmap` residual [sha] only, **1008/1008, 59/59 relocs, 346/346 insns, 4 insns differ**;
the other 5 functions EXACT. sha 89e4adcee46d218320f022e26a8c07ce50713ec52857c7a53bcf9b631573eb47
(January b5fee4632a47ed183bb1aec903552bd1b140a591e64d42625900c8b7bf8a7e69; production 992/59 0bd88e42...).
Evidence files: cand.gate.txt, cand.alndiff.txt, cand.object_audit.txt.

Source changes vs production (all evidence-backed):
1. Descriptor initialised in STRUCT ORDER (bitmaps.c bitmap_2d_new idiom) -> removes our surplus zero web (the
   "zero-pseudo pin" three earlier ledgers called unreachable). Prologue, frame, param-home spills now January's.
2. `registration_point.x = registration_point.y = 0;` chained (Bungie idiom; Jan stores y first).
3. Single exit (w2 law, re-confirmed by labT4): `short bitmap_index = NONE; boolean valid = TRUE;`.
4. Cube-map special case nested: `else if (type == cube) { if (width == height) SET(pow2); else {msg; valid =
   FALSE;} } else SET(pow2);` -> January's two-target branch signature (tail-merged duplicate success arm).
5. `#include "bitmaps/bitmap_group_internal.h"` (object's own header) -> TU name count lands January's reload order.
6. `static` postprocess_bitmap / delete_bitmap (cachebeta publics lack them) + symbols.json `"static": true`.

REMAINING MACHINE DIFFERENCE (4 instructions, size-neutral, relocation-neutral):
  +0x270  Jan `mov ecx,[edi+0x3c]; mov eax,[esi+0x18]; add eax,ecx`   ours eax/ecx swapped (dest = address)
  +0x2a9  Jan `mov edx,[edi+0x30]; mov ebx,[edi+0x3c]; add edx,ebx`   ours loads address first (dest = address)
  i.e. in January the INTEGER operand is the add destination at both `(byte*)group->pixel_data.address + int`
  sites; our VC7 always makes the pointer operand the destination (as January itself does in the exact
  draw_string.c:500 / rasterizer_text.c:715 precedents, where the int is a folded memory operand).
Refuted levers (measured): operand order, array-subscript form, integer-typed add, address/base_address/
pixels_offset/size field types (shadow headers), byte typedef-vs-macro, assert macro form, base-variable
ordinal (param copy), identifier interning order, TU/local name counts, definition order, loop scoping/increment.
Classification: IL operand-order / two-address destination tie (upstream of allocation).

REOPEN CRITERIA: a source/IL fact that makes VC7 treat `group->pixel_data.address` as the non-destination
(still-live) operand at both sites, e.g. a first-party copy of this function (the later /Od build lacks it; a tool
build/HCEX_Release locals would do), or a decode of C2's commutative-operand canonicalisation (dbg32c on the add at
+0x270) showing which IL property selects the destination.
- diagE4 (diagnostic: drop the space_between block) -> site 1 becomes January's (dest = pixels_offset, address in
  edx); diagE5 (pixels_end without the pixels_offset term) -> site 2 load order becomes size-first. => the two
  loop rows are ALLOCATION-sensitive to the loop body's other webs, not fixed IL canonicalisation. Next: natural
  code-identical respellings of the loop body.
- variants.py batches on cand.c (all code-identical respellings; loop rows unchanged): `if (space_between)`,
  `!= NULL` tests, space_between block-scope decl, previous_bitmap init at loop, for-loop one-line/`index++`,
  previous_size local, `a - (b + c)`, size-first subtraction, void*/char* casts, offset-first, array form,
  bitmap_index doubling as loop counter. pe_first (pixels_end before previous_bitmap) moves Jan's proven
  previous_bitmap store -> refuted. baseaddr2 (two-step `base_address = address; base_address += offset`, NOT
  natural, diagnostic only) flips site 2 only -> the two rows are coupled allocation choices driven by site-1 IL.
- STOP: methodology stop rule fired (only an operand-register/load order pair left after >25 shapes/diagnostics).

## PRODUCTION PACKET (for the integrator)
1. scratch/w/bitmap_group/production.patch (git apply --check clean vs 931ed8dc): source/bitmaps/bitmap_group.c
   -> cand.c. Function stays PARKED (fuzzy): re-baseline config/parked.json `_bitmap_group_add_bitmap` base to
   size 1008 / relocation_count 59 / normalized_sha256 89e4adcee46d218320f022e26a8c07ce50713ec52857c7a53bcf9b631573eb47
   (objdiff_percent from the rebuilt report). Zero strict credit.
2. scratch/w/bitmap_group/config/symbols.json: lines 1224-1225 gain `"static": true` (_postprocess_bitmap,
   _delete_bitmap; cachebeta publics lack both). Needs the csplit regen of bitmap_group; the .data section stays
   resolved-equal (verified with the edited symbols.json).
3. scratch/w/bitmap_group/semantic_data_entry.json -> append to config/semantic_data_matches.json
   (allow_incomplete_unit; +1,424 data bytes; unmatched data 7868-6444 = 1424 = the whole section).
Alternative minimal packet: production_storage_only.patch (static + own-header include, parked body untouched) ->
parked body hash drifts 0bd88e42 -> 2e0bbd84c45f240758cc76a2d4df8c78be689cfe9bc21af6a52e017f218e7ca0 (992/59); re-baseline.
