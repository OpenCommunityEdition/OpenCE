# review3_damage - adversarial review of scratch/w/damage (source/objects/damage, FUZZY_IMPROVED)

Base tree 954eebd2. Reviewer ledger; appended after every check.

## R1. Patch application + independent gate (reviewer-built files)
- `git apply --check` clean for production.patch (A) and production_minimal.patch (B) against 954eebd2 working tree.
- Applied each to `git show HEAD:source/objects/damage.c` (LF) in applytest/, CRLF-converted: A == damage/final.c, B == damage/minimal.c (cmp).
- gate --all --forbid-emitted-symbol _point_from_line3d: rev_A 33/1 (AOE 1312!=1328, sha; relocs 51 ok), rev_B 33/1 (AOE 1312/52 as production),
  production base 32/2. _object_damage_body EXACT 1360 in both. No other row changed (sorted EXACT diff = only body added).
- Storage: cachebeta publics have NO symbol at 0x126090 / 0x127020 / 0x128150 / 0x456E48 (first-bracket = symbols.json file_offset,
  verified via neighbour publics _object_cause_damage 0x127570 etc). Absence is address-based, not just name-based -> file-static truth confirmed.
  No other source file references any of the four names (grep source/*.c,*.h).
## R2. Strip tests (reviewer-built variants from production / rev_B)
- s1_static_only (production + `static` on object_damage_body only, typed unit_get re-lookup kept): body 1392/44 residual.
- s2_nostatic_body (rev_B minus `static` on the body): body residual [sha] (size/relocs ok) -> static is load-bearing.
- s3_statics_nowalk (production + all 4 statics, no child walk): body 1392/44 residual.
=> BOTH the static and the child-walk rewrite are load-bearing. static: independently proven (cachebeta publics absent BY ADDRESS;
   HCEX.pdb `static function: true` for object_damage_body, area_of_effect_cause_damage_to_object, object_destroy_notify_children).
   child walk: /Od 0x770491..0x7704d9 object_get(child,-1)->[ebp-0x2c]; `and eax,3` unit-mask; `mov ecx,[ebp-0x2c]; mov [ebp-0x30],ecx`
   plain copy (no verify call); player_index +0x1dc through [ebp-0x30]; next via [ebp-0x2c]. Same idiom verbatim in admitted units.c
   (1719-1727 rider, 8089-8093 child_unit). Declaration order child_object (0x2c) then child_unit (0x30) matches the nested block.
## R3. Emulated split (csplit-only regen)
- control csplit (config copy) == build/split 833/833 byte-identical; emulated (worker symbols.json, 4 lines, +64 bytes) differs ONLY in
  source/objects/damage.obj.
- object_audit_split vs emulated split: rev_A and rev_B -> 47 January symbols, 0 differ; only DIFF section = AOE (1328/1312).
  production base vs emulated: AOE + body sections + 4 storage diffs (as expected).
- Surplus vs production: rev_B identical set (34); rev_A adds _collision_test_line and _real_random (36).
- January damage.obj has NO undefined ref to _collision_test_line/_real_random (both inlined in January's AOE); only callers of the
  new COMDATs in rev_A are AOE (collision_test_line at 2272/2290, real_random at 2345) - AOE is NOT strictly exact.
## R4. Surplus / link / data / scans
- provider_link rev_A: SELECTED-PROVIDER LINK PASS (incl. new _collision_test_line, _real_random vs action_charge); rev_B PASS.
- surplus_identity_cand (worker tool, section_infos_equal vs January selected copy): rev_A 10 code COMDATs 0 not identical; rev_B 8, 0 not identical
  (rev_B set == production set).
- build/report.json damage data 760/760 (100%); object_audit emulated: every January .rdata/.bss section ok for A and B.
- fake_match_scan rev_A, rev_B, production: 0 leads. /W3 /Zs (w3.py, ninja cflags): production/rev_B/rev_A 12 header diagnostics each
  (C4146 cseries.h, C4244 real_math.h), 0 TU-local.
- xref.py: no object in build/split (833) or build/base (621) other than damage.obj names any of the four symbols.
- parkmeas.py (tools.coff_compare.section_info, as parked_functions): rev_B AOE = 1312/52/46952a11... == existing park base (park stays
  valid, no re-baseline); rev_A AOE 1312/51/93606db1... (worker's value confirmed). Body in A and B section_infos_equal January (also vs
  emulated split).
## R5. RULE-6 FINDING on patch A (blocker for A)
- A replaces the AOE hand-expansions with the genuine collisions.h `collision_test_line` and real_math.h `real_random` inlines (/Od calls at
  0x76c50b, 0x76c558 -> 0x432880 and 0x76c6ca -> 0x42e1c0 confirm the helpers). This newly emits _collision_test_line and _real_random
  COMDATs whose ONLY caller in the TU is AOE, which is NOT strictly exact (1312 vs 1328).
- docs/campaign_house_rules.md rule-6 exception + brief section 2: a TU may emit a shared-header __inline COMDAT ONLY when "its caller is
  strictly exact" (+ identical copy + zero-regression sweep + provider link). Identity and link pass, but the caller condition fails.
- This is the same class as the HELD actor_combat P1 item ("admit a January-owned header COMDAT ... whose caller is not yet strictly
  exact") -> owner-gated; A cannot be self-landed. Recommend B; hold A's AOE part with reopen = AOE strict exact or owner ruling on P1 class.
## R6. Amended patch B+ = B + worker item (4) (/Od re-read of object->object.region_damage[], drops non-/Od `byte region_damage` local)
- rev_Bplus.c: gate 33/1 (guard passed); body section_infos_equal January; AOE 1312/52/46952a11 == existing park base (no re-baseline).
- object_audit_split vs emulated split: 47 January symbols 0 differ; only AOE section differs; surplus == B == production (8 code COMDATs,
  all IDENTICAL); provider_link PASS; fake scan 0; /W3 0 TU-local; pdb_storage(emulated split, rev_Bplus) 0 disagreements (also rev_A).
- /Od 0x7702f1..0x770347: store `region_damage[i] = cvttss2si(255*actual + region_damage[i])` then RE-READS `movzx [edx+ecx+0x134]` for
  the threshold compare; no frame slot for a region_damage local -> removal is /Od-attested (the /Od divss-by-255 is a later-revision
  spelling; January bytes keep fmul 1/255, so `*(1.f/255.f)` stays).
- production_reviewed.patch (git diff of HEAD LF -> rev_Bplus LF): git apply --check clean on the 954eebd2 tree; applied in a throwaway
  repo + CRLF == rev_Bplus.c (cmp). It is production_minimal.patch plus exactly the item-(4) hunk.
- Strip test on the named child_unit local: strip_inlinecast.c (`TEST_FLAG(...) && ((struct unit_datum *)child_object)->unit.player_index
  != NONE`) is ALSO exact -> the named local is byte-inert; the load-bearing fact is "no second object_get_and_verify_type" (January and
  /Od both have none). The named-local spelling is kept because /Od gives it its own slot [ebp-0x30] (attested, not decoration).
## R7. Patch A /Od-faithfulness spot check (AOE clamp block)
- /Od 0x76c787..0x76c866: frame distance [-0x13c], scale [-0x140], radius_delta [-0x144] => declaration order distance, scale,
  radius_delta; scale is a `?:` (temp [-0x168]) then PIN (temps -0x16c/-0x170).
- rev_A keeps production's `real distance; real radius_delta; real scale;` and the if/else clamp (worker's v1_declorder/od2 variants had the
  /Od forms but final.c does not). So item (5) "AOE rewritten in the /Od-faithful form" is inaccurate for the clamp block (law (d) class),
  although byte-inert. Core block (x_axis, y_axis, collision_blocked=TRUE, axis, short ray_index, collision, offset, offset_point) DOES
  match the /Od frame (-0x24, -0x38, -0x3d, -0x50, -0x58, -0xac, -0xc0, -0xd4).
## VERDICT
- Patch A: REJECT for self-landing (rule-6 new COMDATs from a non-strict caller = owner-gated P1 class; clamp block not /Od-faithful).
- Patch B: APPROVE. Amended B+ (production_reviewed.patch = B + /Od region re-read) APPROVE, preferred. symbols.json copy APPROVE.
  parked.json: retire _object_damage_body only; AOE park unchanged (sha 46952a11 still the base under B/B+).
