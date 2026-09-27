# Owner packet: compiler-application campaign (base canonical cc608036)

Corrected 2026-09-26 as the owner instructed:
- Q9 DOES emit new symbols;
- "no C4013" means no NEW C4013;
- scratch verification is separated from full-board gates.

## Two different levels of verification (do not conflate them)
1. **Lead scratch verification (every item below).** `scratch/campaign/review_patch.py` applies the patch to a scratch
   copy (never the worktree), compiles it with the unit's real flags, and diffs every named section against
   `build/base` and January's `build/split` object. For every item:
   - only its own object changes;
   - the named functions go from residual to EXACT;
   - 0 previously exact sections change;
   - newly emitted symbols: NONE for Q2-Q8. **Q9 emits three new COMDAT helper copies** (listed in its row);
   - the workers' /W3 census shows **no NEW C4013**. hs_compile.c keeps its two INHERITED C4013 (present at base,
     W1 `logs/FINAL_hs_compile_warnings.txt`); the other code multisets are unchanged.
   This is a single-object check, NOT an admission gate.
2. **Full-board gates (only items that land).** Full ninja build, whole-board stable zero-regression sweep, parks,
   admission/ownership audit, warnings, fake-match scan, pytest, `git diff --check`, plus provider links for any
   surplus symbol. Recorded per landing commit in LEDGER.md section 2d/2g.

Patch hashes below are sha256 prefixes.

| Q | function(s) | unit | meaningful B | object effect | class / held form | evidence | patch (sha256) | RULING 2026-09-26 |
|---|---|---|---:|---|---|---|---|---|
| Q2 | `_hs_compile_postprocess` | hs/hs_compile | 714 | 61/3 -> 62/2 | Original-bug form: the unparenthesised script test reads a stale/indeterminate `script` for an out-of-range index | J: January reloads [ebp-8] on both out-of-range exits. D: /Od /RTCu uninitialised-use check of `script` at that read. X: HCEX frame locals. Also B (/Od write-only `name` local) and C (/Od nested ifs). | W1/hs_compile_postprocess.patch (4f6bedac75931e42) | **HELD.** The evidence supports an original bug but it still introduces a stale/indeterminate-pointer read; aim_grenade does not authorize it. Keep the exact packet AND the safe zero-credit alternative (`hs_compile_postprocess_nobug_fallback.patch`). |
| Q3 | `_hs_parse_set` | hs/hs_compile | 554 | +1 row | Original bug: raw `short` type values passed to two `%s` conversions | J only; the 2020 /Od build has the corrected `hs_type_names[...]` form | W1/hs_compile_parse_set.patch (dc411c8b22c7c84d) | **HELD.** A reachable invalid-pointer error path; exactness alone does not authorize it. |
| Q4 | `_rasterizer_lights_reset_for_new_map` | rasterizer/rasterizer_lights | 48 | 11/2 -> 12/1 | OOB write: memset of (MAXIMUM_LIGHTS_PER_MAP+1) records, 34 bytes into the next .bss object | J: push 0x7722 vs a 0x7700 array; at cc608036 the next .bss object is `local_lens_flare_parameters`, as in January | W2/rasterizer_lights.patch (e17f1acf27dbcb23) | **HELD pending a corrected, bounded packet.** The typedef checks sizes, not adjacency. Needs an object-layout check of actual offsets/extents, a documented overwrite-before-use lifecycle and its assumptions, and removal of the unconditional "harmless" claim. |
| Q5 | `__rasterizer_environment_lightmap_draw` | rasterizer/xbox/rasterizer_xbox_environment | 4,008 | 42/2 -> 43/1 | Duplicated stage-0 filter statements in both texture arms, plus two load-bearing temps | The J corroboration (fog example) supports the compiler mechanism only | W2/rasterizer_xbox_environment.patch (bd61f0ab04cc123a) | **Duplicated-filter topology HELD.** The two live temporaries are NOT rejected for unknown names (descriptive names are acceptable). The fog example does not independently establish this function's duplicated topology. Candidate preserved. |
| Q6 | `_hud_update_weapon_local_player` | interface/hud_weapon | 1,471 | 13/3 -> 14/2 | Default arm leaves `result` unassigned; the arm is unreachable in defined execution | January + /Od (see batch 2 in LEDGER) | W2/hud_weapon.patch (16dadb1e6dec1517), disclosure comments corrected at landing | **APPROVED (this specific packet)** with corrected disclosure and full gates. LANDED 343a3f82 (lane-verified). The loop visits 0..18 and all 19 cases assign `result`. Not permission for reachable uninitialised reads. No nonreturn attributes. |
| Q7 | `_weapon_place` | items/weapons | 193 | 78/79 -> 79/79 | Return ABI (Lane D class F) | Two HCEA/HCEX PDBs type it `long` | W4/weapons.patch (b730ed2bafbcc394) | **Weapons-only patch NOT to land.** The long-return evidence is accepted. Prepare a coherent owner-header/declaration/callback packet: object_types.c also disagrees on the scenario-datum parameter type and the callback is void-returning. Disclose any remaining target-specific incompatible call; zero regressions; no byte-inert local declaration or cast to hide it. |
| Q8 | `_network_connection_connect` + `_network_server_close_client_connection` | networking/network_connection | 281 + 307 | 21/23 -> 23/23 | H: redundant `success = TRUE;`. A: N+1 loop bound | J; the 2020 debug build has the same bound | W4/network_connection_owner_HA.patch (53b1c88e5c80faeb) | **BOTH HELD, separately.** H: no new evidence justifies the redundant store. A: `client_list[4]` reads a pointer-sized word spanning the following boolean AND padding, not merely the boolean. |
| Q9 | `_decal_clip_to_surface` | effects/decals | 1,768 | 31/2 -> 32/1 | Combined named flag mask `!(surface->flags & (FLAG(two_sided) \| FLAG(invisible) \| FLAG(breakable)))` | **NEW SYMBOLS (three COMDAT helper copies):** `_point_from_line3d` 48 B (January selected copy: action_charge), `_project_point3d` 144 B / 10 relocs (path_obstacles), `_set_real_point2d` 32 B (path_obstacles). The lead's scratch check shows each identical, with the selected-provider duplicate-definition probe PASS in both orders. That probe is NOT a complete program-link proof. | W3/decals_OWNER_QUESTION_mask.patch (4b9635d246620abf) | **APPROVED** for this function: ordinary nonvolatile, side-effect-free byte flags; the mask expresses the same rejection condition; the /Od three-test spelling does not prohibit it. Land only with exact caller and helper identities, complete relocation/ownership checks, provider checks in both orders, full build and zero regressions. LANDED 42fa975e as the R3 reviewer's byte-inert P5b, with the 2026-09-12 redirect reversal disclosed in the commit (lane-verified). |

Q10 (data, separate; not yet ruled): the pooled COMMON block `source/linker_common` holds 1,272,664 B, 95% of all
remaining Halo data. At HEAD our tree emits 66 of January's 242 pooled records as COMMON at identical size
(1,214,917 B). 173 records are absent from our tree, 1 differs in size and 2 are non-.bss records (56,773 B).
Crediting the pool needs two owner decisions:
- (a) an ownership standard for the missing tentative definitions (the COMMON rule: pool position alone does not
  prove an owner);
- (b) a reviewed comparison/credit mechanism for the synthetic unit.

Q10 RULING (2026-09-26): research approved; ownership rules unchanged. Pool adjacency, name/size agreement and an
extern declaration do not independently establish the defining TU. Produce per-symbol evidence packets and leave
uncertain owners unplaced. A synthetic comparison/credit mechanism needs a separate reviewed patch, complete January
coverage and negative regression tests. No scorer change and no COMMON credit are approved. The synthetic-base
builder is diagnostic only. Worker C1 is producing the evidence packets.

Q11 (NEW; data credit blocked by the frozen scorer): hs.obj (54,780 B) and actions.obj (2,404 B).
- The facts (workers B1/B2, lead-reviewed reports):
  - every byte, relocation (offset/type/target/addend) and owner name is identical;
  - our extra literal COMDATs are exactly January's undefined literal targets, each identical to its selected
    provider, with provider_link PASS in both orders;
  - objdiff 3.3.1 cannot credit them because of its `$` literal-name defect. January's own hs.obj scored against a
    byte-identical copy caps at .data 99.52 / .rdata 99.91; renaming `$` gives 100/100; objdiff 3.6.0 scores the
    production pair 100/100.
- The stock semantic_data_matches verifier FAILS CLOSED on these groups (extent-sum mismatch).
- Routes, both owner decisions, nothing applied:
  - (a) the reviewed verifier packet on branch claude/opus-data-verifier-20260925 (fc53d5f6, `extent_model`) plus
    the hs/actions entries (proposals in workers/B1/proposals, B2 card C3): +57,184 data B;
  - (b) moving the scorer binary to objdiff 3.6.0, which re-scores the whole board.
- Rule tension: rule 57 forbids extent-model changes "merely to remove a scoring obstacle"; rule 52 requires separate
  review for comparator fixes.

Q12 (NEW; four sections creditable through the EXISTING stock verifier, no verifier change):
- The sections: game_engine_king .data 136, bitmap_group .data 1,424, ai_communication .data 276, ai_debug .data 56.
  Total +1,892 data B.
- The same `$`-defect artifact: January and ours are byte/relocation-identical, and renaming `$` gives 100.0 under
  3.3.1 itself; bitmap_drawing stays at 99.70 as a negative control.
- Line-surgery entries with the existing `allow_incomplete_unit` opt-in (workers/B2/semantic_data_matches_B2_*.patch;
  variants with and without king). They were verified by emulating the ninja progress chain: 0 revocations, parks
  unchanged, code unchanged. All entry symbols are cachebeta publics.
- Canonical precedents: R2-2 (1cd5e4ed: projectiles, object_lights) and 03a05216 (periodic_functions).
- Disclosures:
  - `docs/objdiff_data_relocation_defect_20260922.md` says "do not add new adjudications";
  - adding king makes it the 11th admission candidate, but its Matching flip still needs fifty-objects owner queue
    #13 (the `_find_next_hill` uninitialised return);
  - rasterizer_xbox_profile's ready entry (132 B) stays tied to its recorded rejection (land only with the Matching
    flip);
  - bitmap_drawing (2,644 B) is a REAL difference: an unnamed 16-byte initialised static, HELD by the 2026-09-20
    "HOLD every unnamed global" ruling.
- Question: may the lead land the four (or three, without king) stock-verifier entries?

Q13 (NEW): `_actor_move_update` (3,128 B, actor_moving). EXACT only with a TU-private DESCRIPTIVE macro:
`#define actor_move_distance_squared(distance) ((distance) * (distance))`, used at the sideslip square.
- Patch: workers/W6/actor_moving_OWNER_QUESTION_squaring_macro.patch (sha256 e8f814474b3fecbc). Lead scratch
  verification: 1 gain, 0 losses.
- NEW SYMBOLS (two COMDAT helper copies), both identical to January's selected copies, with the selected-provider
  probe PASS in both orders (a duplicate-definition probe, not a full program-link proof):
  - `_negate_vector3d` 48 B (action_obey; all-inlined class);
  - `_point_from_line3d` 48 B (action_charge; actor_moving is among ruling #1's 17 objects).
- The patch also carries W3's M4 hunks: the actor_moving `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` redirect removed;
  point_from_line3d at the blend, with two byte-inert /Od-attested `(real_point3d *)` view casts; one
  `crouch = FALSE` after the switch per /Od; the /Od if/else-if structure; /Od negate_vector3d.
- Evidence for the macro: January's bytes only. A parenthesised FP operand gets a hidden 0x267 temp that takes the
  integer issue slot, which reproduces January's store/fmul order. /Od shows an inline mulss, which is consistent
  with a macro but does not prove one. Strip test: the plain `x*x` leaves the one transposition, so the macro is
  load-bearing.
- Precedent: the owner-admitted actor_perception_distance_squared macro (2026-09-20). actor_perception_update is an
  EXACT donor with exactly this schedule (+0x9a4, +0xa8d).
- The same question is open for actions.c `_actor_action_handle_vehicle_entry` (954 B; fifty-objects
  owner_gated_exact.patch).
- Question: extend the 2026-09-20 macro ruling to actor_moving.c (and actions.c)?
- RULING 2026-09-26: **HELD. No macro exception was granted.** The candidate is preserved at zero credit
  (workers/W6). The parallel actions.c `_actor_action_handle_vehicle_entry` form stays held too (same class). A
  reopen needs first-party evidence for the macro itself, not only January's schedule.

Q14-Q16 (NEW; admission packets from worker A2). WORKER-REPORTED, LEAD VERIFICATION PENDING: the lead re-verifies
on rebuilt objects before any landing. Details are in workers/A2/CARDS.md and its patches.
- Q14 render_debug (+1 object, 0 bytes). Review items (a), (b), (c) and (e) are already in canonical (1cd5e4ed).
  The remaining item (d) is the invented aggregate `render_debug_globals_definition`; January had separate file
  statics whose original names are unrecoverable (both HCEX PDBs and the atlas were re-checked). Options:
  - D0: keep the struct plus a disclosure comment (byte-identical);
  - D1: seven `= 0` file statics with descriptive names plus a symbols.json patch. This conflicts with the
    2026-09-15 preference against descriptive static names.
- Q15 breakable_surfaces (+1 object, but -3 exact functions / -7,632 padded B at HEAD). Pieces:
  - (i) a lead-owned symbols.json fix: two January symbols are static per PDB publics (clears both object_audit
    findings; csplit emulation changes only this object);
  - (ii) remove the per-unit `/Ow /QIfist` override (PR #35, no evidence). All January-owned bytes are unchanged
    and 6 helper COMDATs become identical;
  - (iii) the real_local_random header fix (January + /Od named-local body). It loses 3 exact functions by
    declaration count alone: `_rasterizer_frame_statistics_draw`, `_bitmap_copy`,
    `_ai_communication_update_speech_timers`;
  - (iv) a TU-local plane-distance helper with no January counterpart, or hold.
  Under the zero-loss default, (iii) is blocked unless you approve the debit.
- Q16 view-cast class: render_debug's real `rotate_vector2d` call (/Od 0x8450c0 -> 0x4c9a90) needs two
  point2d->vector2d casts. They are byte-inert (stripping them leaves the object identical, with 2 C4133), the
  layouts are identical and the function is strictly exact. The admitted class is 3D->2D. Does it extend to
  same-dimension point/vector views? New symbol `_rotate_vector2d` (48 B), identical to path_obstacles' copy.
- Optional, lead-owned, zero credit: three missing owner prototypes (damage.h, recorded_animations.h, structures.h)
  that clear C4013 in render_debug. The 447-unit sweep shows 0 exact-row changes, but parked
  `__rasterizer_model_draw` moves sha 89b0d7ea -> cbfa8585 at the same size, which needs a park re-baseline. Not
  landed.

## Third-set rulings applied (2026-09-26)
- Q14 held pending independent storage evidence (worker H1 researching).
- Q15(i) LANDED 217bad07. Q15(ii) LANDED 328b6148 (reconstruction; one inherited `_real_local_random`
  identity/link item remains).
- Q16 LANDED db7da71a.
- Optional prototypes LANDED fe283cc5, with the model_draw park re-baselined and the fuzzy regression disclosed.
- Batch-5 disclosure against the new grouping rule (G1 ruling): `_physics_compute_vehicle_collision` (09f5208f)
  REMOVED a redundant `(global_gravity / global_physics_collision_depth)` grouping in favour of the plain
  left-to-right expression. No parentheses or macros were added.
  - Meaningful source: the plain arithmetic reads naturally, and /Od evaluates it left to right.
  - January corroboration: the exact bytes after the change.
  - Please say if you want this re-reviewed under the stricter standard.

## Fourth-set (bounded) rulings applied (2026-09-26). Lane-verified, pending independent canonical reconciliation
Measurement base: canonical d890c2db (`_main_update_time` and `_physics_compute_vehicle_collision` are already
reconciled there and are NOT counted again). Lane-only effect at tip vs d890c2db: code +484 B (+2 functions),
objects +1 (389/468), data +1,892 B (2,587,011 -> 2,588,903; reported separately), parks 76 -> 75. Every packet
below is its own commit with a fresh full build, whole-board keyed comparison (stable_verdicts), park validation,
admission audit, /W3, fake scan, pytest and diff --check (logs: `scratch/campaign/gate_*.out`).
- Item 1 (baseline): applied; see the figures above. Donor-only work stays separate.
- Item 2 (K1):
  - K1-E LANDED 12a714a4 (positive `target_really_alive`).
  - Structural repairs LANDED ea3b848e. The dead `short protagonist_look_priority = 0;` is removed (byte-inert)
    and not restored. DISCLOSED: frame 0x126c -> 0x1270 (4 B over January), hunks 96 -> 72; production's
    inherited `boolean near_player = FALSE;` is also dead and stays as your call (removing it: 74 hunks, 8,096 B).
  - K1-A (narrow) LANDED 7c0d73f2 as the FALSE-initialised `protagonist_invalid` flag (67 hunks). The full lab
    candidate is not landed.
  - K1-B/C held; K1-D held until a strict-exact caller exists.
  - `_ai_communication_event` stays residual at 8,096 vs 8,064 B. Zero credit throughout.
- Item 3 (periodic_functions): the minimal `real_random()*0.25f + 0.25f` patch LANDED 219c2c72 (+244 B) and
  retired its park. The object was admitted (c5366682) only after the complete object audit. The alternative helper
  route was not used.
- Item 4 (E1-Q2): LANDED 5901f1eb (+240 B). The BUG comment records the target-build proof:
  - the only unwritten path needs edges.count < 0 (deletion named as one origin, not the only one);
  - on that path, dynamic_array_get_element's unconditional `array->count>=0` assertion runs first;
  - match_assert's failure path (display_assert, system_exit -> halt_and_catch_fire) never returns: it loops on
    the error screen, or calls exit on re-entry.
  No noreturn annotation. Fresh whole-board gate: 0 regressions.
- Item 5:
  - K2 E13 LANDED 83e1921e (object identical). The larger /Od rewrite stays held.
  - path_structure_bsp loop cleanup LANDED 08200b26 (object identical). Admission stays HELD.
  - hardware_geometry Unlock-wrapper repair + 11 symbols.json rows LANDED 2717b687. Exactly 1 of 833 split objects
    changed; object_audit PASS; pdb_storage 13 -> 2 (the held MoveResourceMemory pair).
  - dead_camera, realcmp_epsilon and MoveResourceMemory: held, untouched.
- Item 6 (G1): applied as stated. No parenthesis sweep, no nav-point strip, no render_cameras rebaseline; W6
  stands. The /Od-shape remainders of A3 (below) are held, not landed under G1.
- Item 7 (COMMON):
  - Doc correction LANDED 9c350e37: csplit UNDEF does not prove an import; the owner stays undetermined. No
    definition was restored and no ownership test was touched.
  - files.c/files.h volume names LANDED bd68541c: `[NUMBER_OF_FILE_REFERENCE_LOCATIONS][256]` (two rows; COMMON
    256 -> 512 = January), tentative storage kept. Consumer sweep: 16 includers rebuilt, only files.obj differs.
    Zero credit; no owner established.
  - C1's vendor "PLACED" is PROVISIONAL until relocation identity is verified (noted in C1/COMMON_EVIDENCE.md).
    No denominator, synthetic credit or owner change.
- Item 8 (data):
  - Q11: the V1 review packet is prepared for independent review (workers/V1/). objdiff is not upgraded, the
    verifier is not changed, and the 57,184 B are NOT claimed.
  - Q12: three entries LANDED 387bab37 (+1,756: bitmap_group 1,424, ai_communication 276, ai_debug 56); king
    evaluated separately and LANDED b30042ba (+136). Checks: fresh independent identity (tools/data_identity.py,
    own COFF reader; 197 literal targets checked), relocation, ownership and PDB-storage checks, and the EXISTING
    fail-closed verifier. Hashes are pinned in gates/Q12_EVIDENCE.md.
  - King side effect: the admission audit now lists game_engine_king as a candidate (10 -> 11). The object stays
    NonMatching, and its code and whole-object holds are intact.
- Item 9: Q4 and Q7 held; the Q7 coherent packet is preserved unchanged.
- Item 10 (object admission):
  - Q14/render_debug held.
  - game_engine NOT admitted: shared-header exact losses and storage/type ownership still block it. The
    pointer-alias alternative is noted as preferred; no punning exception was used.
  - A3/A4 zero-loss packets LANDED, one per commit:
    - A3 helper repair 602846b8 (+4 identical COMDATs, provider link both orders);
    - hs_runtime type corrections 91f4824b;
    - HDR-1 be21ae0a and HDR-2 0abc5d2c (every consumer swept);
    - dynavobgeom dead global 76ca44cd;
    - rasterizer_xbox R-A/R-B + 24 symbols.json rows 250a8f4b;
    - W0 window views 32140e5c.
  - HELD, preserved, NOT landed (outside the ruling's helper/owner-declaration/type/window-view classes):
    - `landing/HELD_A3_pao_od_shape_names_remainder.patch`: /Od flag-and-break, desired_direction, factor order,
      header-comment names;
    - `landing/HELD_A3_hs_runtime_remainder.patch`: H-1 enum_value assert, H-3 HCEX constants, H-4 /Od
      hs_can_cast.
  - DEFERRED: A4's rasterizer_xbox_internal.h stale-prototype hunk with its parked `__rasterizer_model_draw`
    rebaseline (95.08453 -> 95.07629; a declaration-count effect).
  - Still held: Q-A3-1..3 and OQ-1..OQ-4. No exact-function debit anywhere.
- Item 11 (process): commit 453bdbb5 had carried 677 staged research-record files with E13. Since the lane is
  unpushed, it was split (700ca0d1 records + 83e1921e E13; COMMIT_SPLIT_20260926.md). Tip tree identical;
  backup ref kept.

## New packets awaiting rulings. WORKER-REPORTED; the lead re-verifies the actual patches on rebuilt objects
## before any landing.
- Q7 (revised): D1's COHERENT packet `workers/D1/Q7_coherent_packet_PK.patch` (sha256 af756294...).
  - Contents:
    - weapons.c: `long` + `return weapon_index;`;
    - weapons.h (owner header): the prototype plus a `struct scenario_weapon_datum;` tag;
    - object_types.c: the consumer-local stand-in declaration is removed, the table binds weapon_place directly, and
      a line-count-preserving `BUG:` comment is added;
    - object_types.h: the slot `void (*datum_place)(long, void *)`, the type BOTH HCEX PDBs record.
  - Remaining target-specific incompatible call (disclosed): January calls the long-returning weapon_place through
    the void-returning `void *` slot. VC7 does not diagnose it. The `void *` parameter is shared by all 9 place
    callbacks; the long return is weapon_place-specific.
  - Effect: +1 function (+193 B). Worker sweep: 156 consumer TUs, 0 exact losses, /W3 identical.
  - +1 object is NOT automatic: weapons keeps an inherited `_data_00307140` placeholder static (held under the
    2026-09-15 descriptive-names ruling).
  - Fallback without the slot change: one visible C4028 remains.
- Q4 (revised): F1's corrected bounded packet `workers/F1/rasterizer_lights_Q4_corrected.patch` (sha256 3179e2c7...)
  plus `workers/F1/tools_layout_check.patch` (a tools test, 12 tests).
  - The layout test pins the actual offsets/extents in BOTH January's split and the built object:
    - results at +0x40020, extent 0x7700;
    - parameters at +0x47720, stride 0x28;
    - clear 0x7722;
    - nothing else in the span.
    It rejects 9 synthetic perturbations and 3 compiled ones (N1-N3). NOTE: the function gate alone still says EXACT
    for all three compiled perturbations, which is why the layout test is needed.
  - Lifecycle: P1-P7 are proven from January's bytes (count bounds, every read bounded, the full-record copy in
    submit before any exposure, no other file, the one external reader). A1-A6 are ASSUMED (single thread; no
    re-entry, noting the reset is also the hs/console command; no non-local exit; no stray pointers; COFF .bss
    contiguity; nothing interprets these bytes).
  - The "harmless" wording is removed.
  - Question: admit under these conditions (the test lands with the source)?
- A3 (path_obstacle_avoidance, hs_runtime), zero-credit admission-prep patches, each 0 gains / 0 losses:
  - path_obstacle_avoidance.patch:
    - the genuine header inlines for valid_real_point2d and normalize2d replace a memcpy bit-test and a macro
      redirect;
    - the /Od flag-and-break shape replaces two gotos;
    - /Od names;
    - 4 new identical COMDATs.
  - hs_runtime.patch:
    - a real `enum_value` assert;
    - const correctness;
    - HCEX constants;
    - the /Od shape replaces a goto;
    - an invented pad member is removed.
  - Lead-owned headers:
    - HDR-1: hs_node_gc / object_list_gc prototypes;
    - HDR-2: one owner definition of `struct hs_external_global_definition`, replacing 3 .c copies.
  - Q-A3-1: admit path_obstacle_avoidance with the descriptive statics `debug_path`/`debug_obstacles`? The split
    itself is January-proven, and the names are layout-coupled via the name hash.
  - Q-A3-2: hs_runtime, keep the TU-invented `union hs_conversion_result` ABI (inactive-member reads, incompatible
    function type), or take the HCEX `long` form with 5 BUG-commented January partial writes?
  - Q-A3-3: `error_heap`'s block-local real/long union for "%x": within the `_wind_variance_get` precedent?
- A4 (rasterizer_xbox, dynavobgeom), zero-credit stage-1 repairs, which fix every object_audit finding in both:
  - dynavobgeom: remove a dead global left by merge f6d00a8c (object_audit FAIL(2) -> PASS);
  - rasterizer_xbox:
    - 24 symbols.json rows made static (PDB publics);
    - HCEX static-local blend tables;
    - direct D3DDevice calls per January's relocations;
    - genuine debug-options type;
    - object_audit FAIL(22) -> PASS against the regenerated split;
  - optional: 10-file global_window_parameters view cleanup (8 of the files in Matching objects; 64/64 includers
    identical).
  - Rulings needed:
    - OQ-1: 9 descriptive .bss names;
    - OQ-2: the single owner declaration costs `_rasterizer_frame_statistics_draw` wherever placed (A: accept the
      debit; B: waive; C: keep the rejection);
    - OQ-3: whether SELECT_ANY identity (134 definers, 0 LNK2005 on surplus) suffices when a complete ordinary link
      is structurally unavailable;
    - OQ-4: the family-wide type debt.

## Q10 research RESULT (worker C1, research only; details in workers/C1/COMMON_EVIDENCE.md)
- 242 pooled records:
  - 2 PLACED linker-generated. These are not COMMON: the image debug directory and the NB10 CodeView record.
  - 27 PLACED vendor records (xapilib / libcmt / dsound / basedll). Each has exactly one defining library member
    at January's size, the member's code is identical to January's, and there is PchSym path evidence.
  - 1 CONSTRAINED (2011 HCEX only).
  - 212 Halo records UNPLACED (1,270,476 B).
- Why nothing Halo can be placed. The pinned-toolchain labs showed that no existing artifact names the defining TU
  of a COMMON:
  - the VC7 map prints `<common>`;
  - section contributions are attributed to `* Linker *` (module 847);
  - PDB module streams never hold COMMON globals;
  - csplit objects show definers and importers identically as UNDEF.
- What pooling does prove: every January definition was a bare tentative definition with external linkage, since
  an initialiser would pull the symbol out of the pool. It does not prove which TU held it, or how many did.
- Correction to canonical wording: the `rasterizer_frame_statistics` "owner test proved import" reads csplit's
  UNDEF form, which a definer shows too. The owner is actually undetermined.
- Our-tree defect found: `_file_location_volume_names` is 512 B in January (2 x 256; January's code indexes row 1)
  and 256 B in ours, so OUR build writes past the end. This is a reconstruction defect, not an original bug. The
  fix earns 0 B, and its hold status needs checking (Lane C memory says HELD; no canonical ruling found).
- C1 questions:
  - OQ1: accept only EC-LNK / EC-LIB / EC-PCH as placement evidence?
  - OQ2: the 213 Halo records: (a) stay UNPLACED, so linker_common is never credited, or (b) a labelled
    "owner-approved reconstruction, ownership unproven" standard?
  - OQ3: change canonical's "proved imports" wording to "undetermined" (docs only)?
  - OQ4: draft the fail-closed synthetic mechanism for separate review? The design note is in C1 §9, with negative
    tests T1-T11.
  - OQ5: classify the 2 linker records as outside source matching?
  - OQ6: move the 27 vendor records out of the Halo category?
  - OQ7: the `_file_location_volume_names` size fix?

## game_engine admission packet (worker A1; WORKER-REPORTED, the lead re-verifies before landing). +1 object
## (388 -> 389), 0 bytes: all 180 functions and 3,792 data B are already credited.
- Base: the fifty-objects lane's owner-queue item 6 (game_engine_clean -> review3 -> finish -> review4), rebased to
  HEAD fe283cc5.
- A1 additionally fixed:
  - the nav-point prototypes moved into the genuine hud.h (no one-consumer header);
  - a kill-count comparison against a message enum that merely equals 4 (rule on NONE/masks/counts) -> `>= 4`;
  - `_netgame_flag_race_vehicle` replaces a raw 4;
  - HCEX `multiplayer_player_info` member names in players.h;
  - HCEX domain constants;
  - 14 unused duplicate prototypes dropped;
  - the invented `debug_player_color` aggregate becomes a short hs global plus HCEX-named static locals;
  - the HCEX `void *custom_data` callback signature (removes a function-pointer cast);
  - boolean TRUE/FALSE, the /Od single-exit comparators, and other cleanups.
- Patches 01-06, which must land together:
  - game_engine.c (option A or B);
  - game_engine.h;
  - owner prototypes in player_control.h / hud_messaging.h / sound_classes.h;
  - the hud.h nav-point trio;
  - a duplicate prototype removed from hud_nav_points.c;
  - players.h HCEX names.
  Consumer sweep (84 distinct TUs, per header): 0 differences. Full board 447/447 SAME. object_audit PASS;
  pdb_storage 0/269; surplus 11/11 identical; provider PASS; protoscan 18 -> 0 consumer-local; /W3 -4 warnings,
  none added; fake scan 0.
- Owner questions (full text: workers/A1/OWNER_QUESTIONS.md):
  - GE-Q1, motion-sensor copy: (A) an in-loop alias or (B) the /Od-attested load-bearing view copy (the rule on 3D-2D
    casts excludes load-bearing view copies unless approved), or hold. Both give identical objects; without either
    the function is residual.
  - GE-Q2: accept the complete HCEX layouts hudg / itmc / netgame kept in game_engine.c (their only consumer, with a
    consumer-local `hud_globals` extern), or hold for a board-wide header consolidation? Each genuine home was
    measured and breaks a distant exact row today.
  - GE-Q3b: keep two descriptive .bss static names? As HCEX static locals they land at the wrong .bss position
    (measured).
  - GE-Q4: `game_engine_playlist_next(parameter0, parameter1, playlist_type)`. January ignores all three
    parameters, HCEX has none, and the names are unrecoverable.
  - GE-Q5, acknowledge the count couplings:
    - the HCEX constant sets were adopted together, because the multiplayer-sound set alone breaks
      `_populate_statistic_buffer`;
    - the units canary's margin shrinks from 12 names to 8;
    - any concurrent packet editing these headers must be re-swept together with this one.

## E1 (released Codex units). The lead re-verified the one exact candidate; everything else is worker-reported.
- E1-EXACT, awaiting your confirmation under the G1 grouping rule: `@periodic_function_build_variable_period_x_table@4`
  +244 B (256 padded). Patch `workers/E1/periodic_functions.patch` (sha256 bdd1e79c...), one line:
  `(real_random()+1.0f)*0.25f` -> `real_random()*0.25f + 0.25f`.
  - Lead check: 1 gain, 0 losses, 0 added/removed; periodic_functions 7/7.
  - D (lead-read): /Od 0x6c8460 +0x53..+0x68 is `call real_random; mulss [0.25]; addss [0.25]`. That is the
    expression TREE (a multiply, then an add of the same constant), not only an order.
  - J: January's `fadd [1.0]; fmul [0.25]`. E1's lab shows VC7 /O2 factors `c*x + c` into that form. The
    parenthesised form yields the same two operations, so January's bytes corroborate the tree only through the
    exact schedule: the parenthesised form adds a hidden barrier node that takes the integer slot and delays `inc`.
  - Meaningful source: yes. It is the later build's own spelling of a random-to-[0,0.5] map.
  - This is a grouping REMOVAL toward the first-party spelling (the same class as batch 5), with no parentheses or
    macro added.
  - Consequence: periodic_functions would have no code or data left, making it an admission candidate (E1 battery
    PASS). Its park entry (entries[18]) would be retired; periodic_functions.h has a two-prototypes-on-one-line style
    item.
  - Question: land it under the stricter rule?
- Zero-credit cleanups (worker-reported):
  - path_structure_bsp: the /Od `next_surface_index = NONE` loop replaces a goto;
  - hardware_geometry: four hand-written `code_` Unlock stubs are removed, plus a symbols.json proposal (11 lines:
    XDK wrapper names + static per PDB). Its object_audit FAIL(7) -> PASS; the 2 remaining pdb rows are the held
    MoveResourceMemory stubs.
- Owner questions:
  - E1-Q1: path_structure_bsp admission. ray2d/line2d read `pathfinding_surfaces[neighbour]` before any NONE test
    (January and /Od; in bounds if every BSP edge has two valid surfaces, which the 2020 tool asserts). BUG comment,
    or rule as an original bug? 9 /Od-attested byte-inert view casts disclosed.
  - E1-Q2: connected_geometry find_or_add_edge, an uninitialised `direction` (/Od `_RTC_UninitUse("direction")`).
    New reachability fact: the only path to the read passes asserts that halt first, the Q6 class. A safe
    zero-credit fallback is provided.
  - E1-Q3: find_or_add_vertex, the `realcmp_epsilon` macro again (no first-party string found).
  - E1-Q4: dead_camera refreshed packet (Codex reachability proof + the Q6 precedent). Your 2026-09-20 exclusion is
    disclosed.
  - E1-Q5: hardware_geometry MoveResourceMemory stays held (no new evidence).
- Mechanism notes (diagnostic): C2 opcode 0x267 is a zero-byte FP barrier created by parenthesised FP
  subexpressions AND by named-float assignments. VC7 factors `c*x + c` into `(x+1)*c` without one. cinematic_render's
  remaining OR-operand order depends only on a symbol count; E1 stopped (count tuning is forbidden).

## K2 (`_ai_debug_render_actor`), negative. Two small questions.
- Q-K2-1: adopt K2's /Od-sourced zero-credit fidelity edits E1-E12/E14? K2 recommends NO: no byte gain, /Od is
  proven to differ from January elsewhere in this function, and E5/E6/E9 flip condition forms to follow /Od.
- Q-K2-2: land the object-identical hygiene patch E13 (removes an unreferenced `char const *string;`, C4101 1 -> 0)?
- Pre-existing rule-15 debt noted: the placeholder names v424-v427 remain in this function.

## G1 (grouping lever under the stricter rule), negative
- 38 of the 50 eligible x87/scheduler rows already perform exactly January's FP arithmetic (`fpops.py`); their
  residue is order, allocation or scheduling. Where our grouping differs from /Od, January usually agrees with OUR
  source (frustum, crosshairs, camera shake, add_continuous): /Od is a later revision there.
- OQ-G1-1 (FYI; G1 recommends HOLD): `_custom_render_nav_point` becomes strict exact if the decorative outer
  parentheses of ONE of two identical `bitmap_extent` lines are stripped. Stripping both is not exact. It is a
  hidden-record count coincidence: January cannot say which line, and the choice would be made for bytes. The object
  is blocked anyway (consumer-local prototypes; `_object_get_bounding_sphere` copy LNK2005).
- OQ-G1-2: load-bearing decorative parentheses already exist in canonical source of zero-credit residual rows
  (ai.c 2624, rasterizer_xbox_lights.c 535, player_effects.c 837, ui_widget.c 5285, bipeds.c 2730, actor_moving.c
  1666). Strip them (bytes may move away from January)? Run a strip sweep over EXACT rows (878 decorative groups
  board-wide, mostly integer or single-operand)?
- OQ-G1-3: keep render_cameras C04a (two named far extents; January's fchs count 27 reproduced, schedule slightly
  worse) as research only, or re-baseline the park with it?
- OQ-G1-4: confirm or correct G1's reading of "January corroboration". G1 reads it as a January byte fact the
  grouping itself determines: a folded constant, a CSE, an operation count or type. A post-change schedule never
  counts. NOTE: under that reading, E1's periodic candidate and batch 5's grouping removal rest on schedule-level
  corroboration; both are disclosed above for your decision.

## Q14 evidence RESULT (worker H1; research only; details in workers/H1/CARDS.md)
- PROVEN or supported independently of our invented names:
  - seven separate objects: the later /Od build keeps them at separate addresses, with hs toggles placed between
    game_time and entry_count;
  - sizes, types and widths from January's own accesses: strings 0x400 char; entries 512 x 0x38; three signed
    shorts; two 8-bit booleans;
  - static linkage: no cachebeta public anywhere in the span;
  - January's exact offsets;
  - January's 2-byte gaps are exactly VC7's slots for separate short statics. An aggregate needs two unaccessed pad
    members (D0) to reproduce them.
- NOT established: the `= {0}` on the two arrays, and the file-vs-function scope of the two flags.
- INFERRED only: `= 0` on the five scalars. Cross-build test against HEK Sapien (2004, VC7.1, the same
  two-bucket law): every line-neutral 2004 edit turning January's layout into Sapien's needs the scalars to be
  `= 0` in January, unless an array was renamed.
- The lab confirms your objection by measurement: seven UNINITIALISED statics whose names happen to sort in
  January's order reproduce January byte for byte. January's bytes alone never prove the initialisers.
- Missing witness:
  - the original declaration text (before render_debug.c line 219) settles everything;
  - a partial substitute is the original names of entries and the five scalars (VC7's name hash would then prove
    `= 0` from the first order break);
  - strings' initialiser and the flags' scope need the text itself.
- A2's D1 packet re-checked at fe283cc5: 36/36, object_audit PASS, pdb_storage 0/69 (against the D1 split). H1
  proposes corrected, non-overclaiming comment wording (in H1 CARDS). No better-supported storage form exists, so
  there is no alternative packet.
- Question: with the arrays' initialisers and the flags' scope unestablished, does Q14 stay held, or will you accept
  D1 with that exact disclosure?

## K1 (`_ai_communication_event`, 8,064 B): improved, not exact (WORKER-REPORTED)
- Admissible patch `workers/K1/ai_communication_event_admissible.patch` (sha256 4774a689...): 8,128 -> 8,096 B
  (delta +64 -> +32), differing hunks 97 -> 72, talk_weight byte-identical, 0 gains / 0 losses, zero credit.
  - It contains the r3 fixes 02-04, including the actor-arm `enemy_status[5]` POLARITY fix (K1-E). You left that out
    on 2026-09-20 as unproven; K1 now proves it from January bytes 0x534-0x5f2. Confirm E before this lands.
  - It also contains the /Od-attested facts K1-06/07/09/10/13/14/16: the boolean animation_impulse, the caller-side
    guard, `continue;` failures, declaration/store orders, and field reads at use.
- Held lab candidate (information only): 8,064 padded, every frame slot and the relocation multiset equal to
  January's, TWO hunks from exact (a deferred `add esp,0xc` position; the j*j/i*i order inside an inlined
  magnitude_squared2d). It needs held forms:
  - K1-A: a FALSE-initialised protagonist-validity flag set in six places. /Od [ebp-0xf45] and January (0xf09
    `xor bl,bl` / 0xf54 `mov bl,1` / 0xf75 `test bl,bl`) are new first-party evidence against the 2026-09-20
    "invented" ruling.
  - K1-B: an uninitialised `play_type` on the reply path. January reloads the previous iteration's value at 0x17a9;
    replies never use it.
  - K1-C: an uninitialised `recipient_look_data` (/Od RTC name). It is only copied, and consumers test `look_type`
    first.
  - K1-D: `normalize2d` at the alignment site (folded-inline rule; needs an exact caller; 4 helper copies identical,
    provider PASS).
- Reopen: a decoded rule for VC7's deferred stack-pop flush, or for the x87 term order of two indirect products
  through an inlined pointer temp.

Q1 (RULED 2026-09-26): the nine inactive Codex reservations are **released for investigation**. Work only in this
lane, check current canonical and the preserved packets first, keep every existing policy hold, and do not modify
the old Codex worktrees.
