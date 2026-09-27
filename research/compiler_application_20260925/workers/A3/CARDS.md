# A3 CARDS.md (outcomes appended; cards are immutable files under cards/)

## PAO-1, PAO-2, PAO-12 (cards written 2026-09-26 00:42:35 -0700; first compile 00:43:50 -0700)
Outcome appended 2026-09-26 00:45:47 -0700. Objects: scratch/campaign/workers/A3/pao1.obj, pao2.obj, pao12.obj.
- PAO-1 (genuine header inline valid_real_point2d; hand memcpy body, rename, #undef memcpy, <string.h> removed):
  gate 24/24; keyed_diff vs build/base: 0 changed, 0 added, 0 removed. PREDICTION CONFIRMED (byte-inert).
- PAO-2 (genuine header inline normalize2d; rename + consumer-local prototype removed): gate 24/24; path_add_step
  still calls _normalize2d out of line (EXACT). keyed_diff: 0 changed, +4 added: _normalize2d 80, _magnitude2d 32,
  _magnitude_squared2d 32, _scale_vector2d 32. PREDICTION PARTLY FAILED: predicted +1 symbol (_normalize2d only);
  actual +4 (normalize2d's own header-inline callees are emitted too).
- PAO-12 (both): gate 24/24; keyed_diff 0 changed, same +4. Additive, as predicted.
  - surplus_identity (A3 variant with object path): 15 candidate-only code COMDATs, 0 not identical; the 4 new ones
    are IDENTICAL to January's selected action_charge copies.
  - provider_link (both orders): PASS, 21 symbols.
  - object_audit: PASS, 52 January symbols, 0 differ.
  - /W3: identical multiset to HEAD (12 header-only warnings). fake_match_scan: 0 leads (HEAD files also 0).

## PAO-S view-cast strip test (card 00:47:53 -0700; compile 00:47:53 -0700, same second: card file written first)
Outcome appended 2026-09-26 00:48:49 -0700. Object scratch/campaign/workers/A3/pao12_strip.obj.
- gate 24/24; keyed_diff pao12 vs strip: 0 changed/0 added/0 removed; /W3 gains exactly six C4133
  ('real_point3d *' -> 'const real_point2d *') and nothing else. PREDICTION CONFIRMED.
- Rule-28 conditions for the six casts: per-site /Od attestation (0x4cafb0 path_find pushes [ebp-0x34]/[ebp-0x38];
  0x4cc6b0 path_new pushes 0xae9f84/0xae9f94), prefix layout, byte-inert strip, strictly exact callers
  (_path_avoid_obstacles, _render_debug_obstacle_path). Needs ledger disclosure only. Casts stay (not a patch).

## error_heap /Od readout (no card: read-only evidence, no compile)
- /Od 0x4c9b40 ('%3d. %.12g (%x)'): frame 8; heap_index short [ebp-4]; heap_cost result fstp dword [ebp-8], then
  push dword [ebp-8] (bits) and cvtss2sd [ebp-8] (value). No _RTC_CheckStackVars descriptor. Indistinguishable
  between the current block-local union and a '*(long *)&cost' pun. Disclosure item, no change.

## H-1, H-2, H-3, H-4a, H-4b, PAO-3 (cards 01:02:37 -0700; first compile 01:04:28 -0700)
Outcome appended 2026-09-26 01:05:03 -0700. Objects scratch/campaign/workers/A3/hs_H1..hs_H4b.obj, pao3.obj.
- H-1 enum_value local + stringified match_assert: 66/66; keyed_diff vs build/base 0/0/0. CONFIRMED.
- H-2 const on 40 externs: 66/66; 0/0/0. CONFIRMED.
- H-3 HCEX constants + sizeof: 66/66; 0/0/0. CONFIRMED.
- H-4a no-goto two returns: 66/66; 0/0/0. CONFIRMED.
- H-4b /Od single-call shape: 66/66; 0/0/0. PREDICTION FAILED (predicted not exact): VC7 rebuilds January's two
  call sites from the one-call /Od source.
- PAO-3 path_add_step /Od flag + break (no gotos): 24/24; keyed_diff vs pao12 0/0/0. CONFIRMED.

## H-4c, PAO-4, PAO-5 (cards 01:05:03 -0700; first compile 01:05:59 -0700)
Outcome appended 2026-09-26 01:06:24 -0700.
- H-4c (full /Od hs_can_cast: object_type declared in the object branch; object_name branch passes
  desired_type-_hs_type_object_name directly): 66/66; keyed_diff vs build/base 0/0/0. CONFIRMED.
- PAO-4 (RTC name desired_direction): 24/24; keyed_diff vs pao12 0/0/0. CONFIRMED.
- PAO-5 (/Od factor order cross(opp,desired)*cross(opp,previous)): 24/24; 0/0/0. (50/50 prediction; exact.)

## H-L / H-1c (cards 01:07:01 / 01:07:31 -0700; compiles 01:07:01 / 01:07:31 -0700, card files written first)
Outcome appended 2026-09-26 01:11:20 -0700.
- H-L (H1+H2+H3+H4b+H4c): 66/66; keyed_diff vs build/base 0/0/0; /W3 multiset unchanged; fake scan 0. CONFIRMED.
- H-1c (const enum_definition): with H-L = hs_L2: 66/66; 0/0/0; /W3 loses the only C4090. CONFIRMED.

## H-5 (card 01:09:28; compile 01:10:04 -0700) and H-5b (card+compile 01:10:47 -0700) - RESEARCH PROBES
- H-5 S1 (12 converters in HCEX 'static long f(long)' form on hs_L2): 65/66; only _hs_long_to_short residual
  (VC7 folds the self word store). /W3: C4028 64->0, C4133 55->0, hs_runtime C4244 3->0 (only C4013 x2 and 12
  header warnings remain). PREDICTION PARTLY FAILED (predicted 12/12 incl. an in-place long_to_short).
- H-5b S2a (long_to_short with HCEX 'long result' local, upper 2 bytes indeterminate): 66/66 EXACT. CONFIRMED.
- H-5b S2b (initialised local, UB-free): residual 32 vs 16 B. CONFIRMED (not exact).
- Consequence: an exact HCEX-ABI converter set exists (hs_H5b_S2a copy) but hs_string_to_boolean (3 bytes) and
  hs_long_to_short (2 bytes) must return partially uninitialised storage = January's machine behaviour
  (January string_to_boolean: push ecx / mov [ebp-4],al / mov eax,[ebp-4]). No UB-free exact form exists for
  string_to_boolean (any initialisation adds a store). The current union ABI carries the same January behaviour
  behind the union return + incompatible-function-type call. => OWNER QUESTION (original-bug class), not a landing.

## H-6 (card 01:11:54 -0700; compile 01:12:24 -0700) - RESEARCH PROBE on H-5b S2a
Outcome appended 2026-09-26 01:12:41 -0700.
- HCEX inspector ABI (short type, long value, char *buffer), long *value in hs_evaluate_inspect: 66/66;
  keyed_diff vs build/base 0 changed/0 added/0 removed; /W3 = C4013 x2 + 12 header warnings. CONFIRMED.
- Cumulative hs_H6 copy (H1 H1c H2 H3 H4b H4c H5 H5b-S2a H6) is keyed-identical to build/base.

## H-7 (card 01:14 -0700; compile 01:15:18 -0700) - RESEARCH PROBE, union-free TU on H-6
Outcome appended 2026-09-26 01:15:49 -0700.
- struct hs_global_datum.value -> long (HCEX hs_global_runtime), typed views everywhere, 'long result_long' in
  equality/inequality/logical/arithmetic (HCEX local name), 'long *' stack slots in if/sleep_until, union deleted:
  66/66; keyed_diff vs build/base 0/0/0; /W3 = C4013 x2 + 12 header warnings. CONFIRMED.
- January evidence for the packaging: hs_evaluate_equality +0xc5 'mov byte [ebp+8],al; mov edx,[ebp+8]; push edx'
  (boolean stored into the dead function_index argument slot, whole dword passed to hs_return).

## HDR-1 (card 01:17 -0700; control sweep 01:18:19, patched sweep 01:18:37 -0700)
Outcome appended 2026-09-26 01:20:42 -0700. Alt roots scratch/campaign/workers/A3/root_base (git archive HEAD 09f5208f source) and
root_hdr (+ header patch). Logs hdrsweep_base.txt, hdrsweep_hdr.txt, hdr_w3_sweep.txt.
- Control: 17/17 consumers keyed-identical to build/base (harness valid).
- Patched: 17/17 keyed-identical to build/base (0 changed/0 added/0 removed each). CONFIRMED.
- /W3: hs_runtime C4013 2->0; hs_compile C4013 2->1 (remaining one unrelated); 15 other consumers identical.

## PAO-6 (card+compile 01:20:12 -0700)
- '__inline' on path_get_step / path_get_step_index (Sept-2001 cachebeta.map 'i' tags): 24/24; keyed_diff vs pao5
  0/0/0. CONFIRMED byte-inert. Offered as an optional separate patch.

## H-8 (card+compile 01:24:07 -0700): delete the invented 'byte pad;' in struct hs_runtime_globals
- both finals 66/66; keyed_diff vs build/base 0/0/0. CONFIRMED. Folded into final/hs_runtime(+_owner) via mk_final.py.

## Integration root (01:24:46 -0700): HDR-1 + final hs_runtime + final pao via revgate
- hs_runtime 66/66 0/0/0 (landable and owner variants); path_obstacle_avoidance 24/24, +4 COMDATs only;
  hs_compile unchanged.

## PAO-D (card+run 01:25:22 -0700) DIAGNOSTIC, zero credit, names NOT proposed
- D0 debug_path/debug_obstacles: path@0, obstacles@0x1538, flags@0x2140/41 (= January), 24/24.
- D1 obstacle_path_snapshot/obstacles_snapshot: obstacles@0, path@0xc08, flags@0x213c/3d (FLIPPED), 22/24.
- D2 failed_path/failed_obstacles: January order, 22/24 (names differ from symbols.json -> reloc identity).
- D3 current names, declarations swapped: January order, 24/24 (declaration order irrelevant).
- PREDICTION CONFIRMED: uninitialised-static order follows the names (hash); the current descriptive names are
  layout-coupled. January's own evidence fixes the order independently: .bss size 0x2142 and PDB-public flag
  offsets 0x2140/0x2141 require path (0x1534) at +0, 4-byte gap, obstacles (0xC08, 8-aligned) at +0x1538.
