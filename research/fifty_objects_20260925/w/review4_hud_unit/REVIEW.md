# Review 4: hud_unit OBJECT_COMPLETE_CANDIDATE (worker slug hud_stack_check), tree 16542e46

## Verdict

**approve = false as submitted.** One patch breaks owner ruling 20260921 #2 (law (i)), and the listed alternative is
contradicted by the /Od build. I have built an amended packet and verified it end to end. It is in `patches/` in this slug.
It lands hud_unit as a whole object, with one disclosure (P7).

| Item | Verdict |
|---|---|
| P1 hud_draw.h (`__inline check_stack_buffer`, the macro, the enum) | OK. Shared header: sweep below |
| P2 hud_draw.c | OK. Must land with P1 and 01RS (LNK2005 otherwise) |
| 01RS hud_unit.c (= review3's 01R + P3) | OK. Use this one |
| **01RaltS** | **Reject.** It keeps `goto update_finished`. The /Od build has three per-arm `last_shield_hit_time` stores and no goto (review3), so this is the invented-goto class |
| P4 hud_nav_points.c | OK (optional). Its emitted COMDAT has a strictly exact caller, `hud_update_nav_point_local_player` |
| P5 hud_weapon.c | OK. Keeps hud_weapon byte-stable, and removes invented gotos from two EXACT functions (`render_grenade_hud`, `hud_update_weapon`) |
| **P6 motion_sensor.c** | **Drop (owner-gated).** It newly emits `_check_stack_buffer` in motion_sensor.obj, and every caller (`_motion_sensor_update`, `_update_motion_sensor`, `_render_motion_sensor`) is residual AND parked. That is the damage-patch-A / actor_combat P1 class. Dropping it is raw-inert |
| P7 rasterizer_xbox_models.c drops a dead include | OK with disclosure. It is load-bearing only relative to P1 (count shielding); see below |
| 02 symbols.json / 03 config.json / 04R rejections | OK. 04R is required |

**Amended packet** (apply atomically, in this order): P1, P2, 01RS, P4, P5, P7, 02, 03, 04R.

- The files are byte-identical copies of the worker's patches (`patches/SHA256SUMS.txt`).
- `git apply --check` is clean as a set on 16542e46.
- P4 is optional. The strict fallback P1+P2+01RS+P7+02+03+04R is also measured: the only effect is that the unparked
  residual `_render_weapon_hud` changes its bytes at the same size and relocation count.

## Independent re-runs (all my own scripts and outputs, under this slug)

- **Patch applicability.** All 12 patches are clean alone and as the full set. `patch` replay onto `git show HEAD:`
  copies reproduces every worker candidate file.
- **Harness.** The strict raw COFF comparer `tools/rawcmp.py` checks section order, bytes, relocations by target name,
  COMDAT selection and the whole symbol table. Only `$L` label numbers are normalised. The control sweep (verbatim header)
  is 23/23 raw-identical to build/base.
- **23-consumer sweep.** The independent grep found 23 consumers: no path with spaces, and no header or `.c` include chains.
  - With the full packet, all 17 non-sentinel consumers and models are raw-identical.
  - Without P7, `__rasterizer_model_transparent_geometry_submit` goes from EXACT to residual.
  - With P1 and every TU left as production, hud_weapon's residual drifts.
  - In the sentinel objects, the owner-keyed comparison shows only the new ANY `_check_stack_buffer` (parked bodies
    byte-identical). hud_draw changes only from NODUP to ANY, plus the order noted below.
- **Emulated csplit.** Using this slug's config (4 in-place symbols.json edits, CRLF kept, byte-identical to the worker's
  copy):
  - The control split equals build/split for 833/833 files.
  - The emulated split differs only in hud_unit.obj.
- **hud_unit.**
  - 01RS is 22/22 EXACT against the emulated split, with no storage mismatches.
  - object_audit passes: 30 sections ok and 30 January symbols with 0 differences.
  - Surplus: `_fast_ftol` is IDENTICAL to actor_combat's copy and `_check_stack_buffer` to hud_draw's.
  - The object differs from review3's cand_final_amended.obj only by the new COMDAT.
- **Provider link.** With the packet's hud_draw.obj, 12/12 PASS in both orders. The control with production hud_draw
  (NODUP) gives LNK2005, so the check discriminates.
- **Strict emulation.** objdiff 3.3.1 plus the repo pipeline give 22/22 functions, 6176/6176 code and 152/152 data, with
  nothing contradicted. With the old rejections file the result is contradicted, so 04R is required.
- **Hygiene.** The /W3 warning multisets are unchanged in all 6 edited TUs. fake_match_scan leads are unchanged (hud_unit 0).
- **Evidence.**
  - January defines `_check_stack_buffer` only in hud_draw, and 0 references or relocations to it exist anywhere.
  - It is PUBLIC in cachebeta (C08F0).
  - The 26 macro sites equal January's 26 sites.
  - None of the four hud_unit statics is public in cachebeta.

## Rules

- **Strip test.** The plain goto-free loops (review3 S1/S2) are not exact. The inline function is attested by first-party
  evidence: the inlined return merge at all 26 sites, the PUBLIC symbol, and the fact that nothing calls it out of line.
  The macro only expands to the same statements. PASS.
- **New names.** The only new name is `match_assert_stack_frame`, a descriptive macro in the explicit-location `match_`
  family; Bungie's name is lost. `check_stack_buffer` is first-party. STACK_BUFFER_* already existed in production.
- **/Od.** The build has no sentinel, so no declaration-order evidence exists for the macro block.
- **No other blockers.** There are no floats, views, consumer prototypes or hand expansions. hud_unit.c has no goto left.

## Disclosures and advisories

1. **P7 (disclose in the ruling request).**
   - Of hud_draw.h's 26 names, models uses none except the struct tag `bitmap_data`, which bitmap headers already
     declare.
   - The include came from project commit 635bd83d. Removing it is raw-inert and warning-inert.
   - It is still count shielding: P1's inline definition costs about 9 name units in models.
   - It follows the precedent of removing verbatim duplicates. It is not the Layer-2/main header_swap class, which ADDS
     declarations chosen by count.
   - The P-A fallback (an invented focused header) conflicts with owner ruling 20260921 #4.
2. **Newly emitted symbols (ruling 6).**
   - An ANY `_check_stack_buffer` (48 B, identical to January's hud_draw copy) in hud_unit.obj, hud_nav_points.obj and
     hud_weapon.obj.
   - hud_draw.obj's own copy goes from NODUP to ANY.
3. **Emission order (advisory; not scored).**
   - VC7 emits a referenced header inline at its definition position. The measurement: it lands after the earlier
     headers' inlines and before the TU's own functions in hud_unit, nav and weapon.
   - The candidate hud_draw.obj therefore emits `_check_stack_buffer` BEFORE `_get_return_eip`. January's image has
     get_return_eip at C08E0 and then check_stack_buffer at C08F0.
   - January's arrangement is consistent with get_return_eip ALSO being a hud_draw.h inline defined first, which matches
     the Sept-2001 `i` flag.
   - This matters for hud_draw's own future admission, not for hud_unit. Moving get_return_eip now would emit
     `_get_return_eip` in motion_sensor from non-exact callers only.
4. **Composition.** Integrator: re-sweep all 23 hud_draw.h consumers and the parks on the live tree after applying.
   Isolated header tests do not compose.
5. **motion_sensor.** It keeps its three hand copies (pre-existing debt in parked functions). Revisit P6 once any of
   those callers is strictly exact, or the owner admits the P1 class.

Full probe log: `LEDGER.md`. Tools: `tools/` (sweep, rawcmp, keycmp, gate_split, object_audit_split,
surplus_identity_obj, provider_link_ovr, warn). Outputs: `out_*`, `split_ctrl`, `split_emu`, `objproj_*`.
