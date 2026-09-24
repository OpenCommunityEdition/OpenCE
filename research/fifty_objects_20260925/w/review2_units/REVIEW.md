# REVIEW: source/units/units (OBJECT_COMPLETE_CANDIDATE from units_ownership), base 931ed8dc

## Verdict

**approve = false as submitted.** One fixable defect blocks it. Every byte, storage, provider and scorer
claim reproduced independently. The worker's patch set would freeze a misleading, unattested
function name in the admitted object, and it leaves a false comment directly above one of the
functions it renames.

With the amended patches in this directory, all gates pass and the object is admissible:

- `patches/01_units_c_names_static.AMENDED.patch` replaces the worker's 01.
- `patches/02_symbols_json_names_static.AMENDED.patch` replaces the worker's 02.
- Alternatively, keep the worker's 01 and 02 and apply `patches/07_incremental_on_top_of_worker_01_02.patch` after them.
- The worker's 03, 05 and 06 are unchanged. Do not apply optional/04.

## Blocking finding

**`_unit_verify_inventory` (0x19ef10, symbols.json line 7004) is the wrong name.**
- **The body:** it checks that desired_facing, desired_aiming, desired_looking, aiming and
  looking are normal vectors and that forward/up are valid axes. It does not touch inventory.
- **Evidence for the right name:**
  - The January atlas (4cc87b45, 2002-01-14 cachebeta_exe) names 0x59ef10
    `_unit_vectors_are_valid` at the exact tier.
  - The Sept-2001 cachebeta_xbe map (7eacac85, ground truth) has `unit_vectors_are_valid` at
    0x1ada40, directly followed by `unit_verify_vectors` at 0x1adad0. The gap is 0x90, the same
    extent as January's 0x19ef10 -> 0x19efa0.
  - The 1749betap and 2002 cachebeta_xbe atlases agree.
- **Evidence for the current name:** `unit_verify_inventory` appears in no map, atlas, HCEX PDB or
  doc. HCEX and HCEX_Release carry neither name, because the function is debug-only.
- **Why it blocks:** patch 02 edits this exact line (adds `"static": true`), and the admission
  text says the object's names are first-party. House rules 7 and 21 require authentic or
  descriptive names, not misleading ones.
- **The false comment** is at candidate line 2485: "HCEX identifies this private helper as
  unit_create_initial_weapons". HCEX.pdb and HCEX_Release.pdb both name it
  `unit_add_initial_weapons(long)`, and so do both Sept-2001 maps. The amendment rewrites it in
  place (two lines become two lines, so assert `__LINE__` literals cannot shift).

## Independently re-run (all on my own artifacts under this directory)

1. `git apply --check` (also with `--whitespace=error-all`): all five worker patches apply,
   individually and together. Applying them to a copy reproduces all five candidate files byte
   for byte, CRLF included. The units.c diff is 286 lines, all renames or `static`, and the line
   count stays at 11,700. The symbols.json diff is exactly 49 in-place lines.
2. **csplit.** A control run with unchanged config gives `split_ctrl`, identical to build/split
   (833 objects). The candidate config gives `split_emul`, where only units.obj differs.
3. **gate.** gate.py against the production split: production 189/189; candidate 142/22/25
   (renames only). My `gate_emul.py` against the emulated split: 189/189 EXACT (54,864 B), with
   storage 142 external and 47 static on both sides.
4. **object_audit** against the emulated split: PASS, 321 January symbols, 0 differing.
5. **Candidate vs production object:** 447 of 447 sections match; only .debug$S differs.
   Relocations are equal after the rename map, and the symbols are equal apart from the renames
   and two external-to-static changes. This is a names and storage change only.
6. **Storage:** 144 externals are cachebeta publics with the same name at the same RVA
   (RVA == file_offset, e.g. `_unit_update` 0x1A2F80). The 50 statics have no public, either at
   their RVA or under their name. Name-based pdb_storage finds 0 disagreements; production has 49.
7. **provider_link:** all 97 surplus externals PASS in both orders. **surplus_identity:** all 31
   helper COMDATs are IDENTICAL to the single January definer. The three D3D SDK tables are
   already surplus in 57 Matching objects (objects, files_windows, scenario and others), so they
   are admissible by precedent.
8. **`_point_from_line3d`:** January's units.obj holds it UNDEF, with relocations at +265 and +325
   in `_unit_cause_continuous_melee_damage`; ours has the same two. Our 48-byte COMDAT
   (0x60501020) is identical to the action_charge copy. Owner ruling 20260921 #1 (Lane B handoff
   section 3) names units explicitly, so retiring the rejection is justified. Without retirement,
   the emulation gives `completion-label-contradicted`.
9. **.data:** `section_info_resolved` is equal for both the production and emulated splits and
   under both symbols.json versions: 1564 / 7 / 6ed61c39..., which is exactly patch 05's measurements.
10. **objdiff 3.3.1** (own single-unit projects):
    - The production control reproduces build/report.json's units entry byte for byte: 169/192, data 5188.
    - With `complete=true`, $L7451, $L7450 and $L8061 all score 100. This matches the 4 complete
      units with 9 $L labels already on the board.
    - The strict pipeline (same order as project_x86.calculate_progress) gives 192/192, code
      53,478/53,478 and data 6,752/6,752, with nothing revoked or contradicted.
11. **Other gates:**
    - No units entries in parked.json or symbol_ownership.json.
    - The only units semantic_matches entry is `_unit_is_busy`, which is not renamed.
    - No symbol in any of the other 1,452 split or base objects references any old or new name.
    - None of the new names collide in source/, xbox/ or config/, and none is a cachebeta public.
    - fake_match_scan: 0 leads.
12. **House rules / strip test:**
    - The only non-rename construct is `static` on unit_ping_animation and unit_add_initial_weapons.
    - It is byte-inert for code (production without it is 189/189). It is load-bearing only for
      storage, which is proven January truth, so it is not decoration.
    - No headers change, and no owner-gated class from brief section 7 is involved.
13. **Name evidence for the 25 renames:**
    - All 25 are genuine.
    - 15 are atlas exact-tier by address. The worker said 16; unit_ping_animation is fuzzy tier,
      but both Sept-2001 maps and HCEX_Release have it.
    - 21 are in at least one Sept-2001 map: 20 are in both, and unit_animation_update is in
      7eacac85 only.
    - The 4 predicates in no map are HCEX_Release statics in January's order with matching
      signatures. I re-queried two of them with DIA2Dump.

## Amendment verification (amend/, amend.obj, split_amend, objproj_amend)
- csplit with config_amend changes only units.obj.
- 189/189 EXACT on the emulated split, and object_audit PASS (321 symbols, 0 differing).
- amend.obj vs cand.obj: 0 non-debug section differences; only the one symbol name differs.
- Storage check 144/50/0, pdb_storage 0, provider_link PASS, fake_match_scan 0.
- Strict emulation: 192/192, 53,478/53,478, 6,752/6,752, nothing revoked or contradicted.
- The AMENDED 01 and 02 plus the worker's 03/05/06 pass `git apply --check` together on
  931ed8dc and reproduce the amended files byte for byte.

## Non-blocking follow-ups (not required for admission)
- HCEX names the plan struct `euler_axis_plan`; ours is `unit_acceleration_plan`.
- HCEX types `unit_animation_state_get_aiming_screen_index` as returning short; ours returns long
  and callers cast. Both functions are exact, so this is pre-existing.
- The single-line static prototypes at units.c:1010 and 1026 predate this lane (house rule 2).
