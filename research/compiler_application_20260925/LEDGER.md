# Compiler-application campaign LEDGER (Opus lead, 2026-09-25)

Assignment: canonical `research/astra_s3_handoff2_review_20260925/OPUS_COMPILER_APPLICATION_CAMPAIGN.txt`. The
companion `REPLY_TO_OPUS.txt` (C3 trace, one small transfer, VK seal hardening) is folded in as bounded sub-items.

## 0. Fresh base (verified 2026-09-25 22:07 -0700)
- Worktree `C:\halo-worktrees\claude-compiler-application-20260925`, branch `claude/compiler-application-20260925`,
  created with `git worktree add` from canonical `cc6080369a3a9dda3fb287880d15a681b391b9f4` (canonical clean,
  branch jonas/exact-pilots). Nothing is imported from the research branch; research is read from its own paths.
- Bootstrap:
  - `xbox` junction to `...\work\halo-exact\xbox`;
  - `cachebeta.exe` copied (sha256 4cc87b45...);
  - canonical's pinned `build/tools/objdiff-cli.exe` (3.3.1, sha1 3130e4288d483d259d1588092c8159f8e0230e08) and
    `csplit.exe` (sha1 7c37061a...) copied;
  - `python configure.py --objdiff build/tools/objdiff-cli.exe --csplit build/tools/csplit.exe` (canonical's exact
    configure_args; NO download edge is generated, so nothing was downloaded; 3.6.0 was never fetched or used);
  - `rule cl` patched to the absolute worktree CL path (environment-only).
- Build: `ninja` rc 0. **Halo 388/468 objects; code 1,584,418/1,770,166 meaningful bytes; 7,454/7,574
  functions; data 2,587,011/3,923,451; 77 validated parked compiler ties.** This equals the reference figures.
- Stable snapshot `scratch/campaign/base_stable.json` (sha256 eb04eba73f9da546): 8,252 owners; 7,626 E / 122 R /
  504 U (U is mostly vendor libs).
- Inherited checks (not caused by this campaign):
  - pytest 1161 passed, 5 skipped, 26 subtests (scratchpad basetemp);
  - admission audit: 10 review candidates, 0 contradicted, 1 rejected;
  - fake_match_scan: 26 review leads;
  - compiler warnings: C4133 x76, C4028 x72, C4003 x27, C4113 x11, C4716 x6, C4090 x4, C4700 x2.
- Batch gate `scratch/campaign/batch_gate.py` validated on a null batch: identical numbers, stable diff 0
  regressions, pytest and `git diff --check` clean.
- Reservations (checked live in `claude-fifty-objects-r3-20260924/scratch/campaign/claims.json`):
  - 9 units reserved for external Codex lanes. Their tips are unmerged and inactive since 2026-09-24 ~16:47. They
    are respected, not perpetuated: owner question Q1 below.
  - The old `opus-mechanisms` bipeds claim was released. This campaign claimed 30 worker units plus bipeds (W5,
    research only) as `opus-compiler-application`.

## 1. Triage (applicability map `APPLICABILITY_MAP.md`; mechanism index `MECHANISM_INDEX.md`)
- 120 remaining ledger rows (`tools/rank_object_closeouts.py`, 80 objects). One row is NOT a function
  (`_ai_debug_render_actor_jmptable`, a csplit table label inside `_ai_debug_render_actor`), and two are
  unwritten (`_main_crash`, `_fast_ftol_C`).
- Detector D1, C1-number residue (M6; `scratch/campaign/c1_sweep.py`, validated against the known
  update_speech_timers residues 1..16):
  - K = 0..63 lab tags after the code-section marker in 55 units, then again at the top of every unit (58 units;
    ~7,400 compiles total);
  - **exactly ONE residual responds:** `_solo_level_select_list_update_displayed_items` at K = 2, 3 (mod 64), with no
    loss. Its park records that it was exact until the 2026-09-07 PlayerProfile ownership migration;
  - every other residual is count-insensitive under both insertion points. A clean negative: the C1 lever is
    narrow.
- Detector D2, normalised relocation-identity deltas (M1; `sig.py`): 26 rows have real missing or extra
  references, and 14 of them are neither parked nor held.
- Detector D3, frame class (M5; `frameslot.py`, installed from the opus5 lane and repointed at this worktree):
  CLEAN 59, DECLARATION 17, BODY-FIRST 13, NOT-A-DECLARATION 10, PARAM-HOME 4, REGISTER-FAMILY 3,
  DECLARED-TYPE 3, UB-ORPHAN-READ 2, DEAD-SLOT 2.
- Holds applied from the owner rulings (15/09, Lane B 21/09, R3 24-25/09, campaign text): see
  `AGENT_BRIEF.md` and the map's hold column.

## 2. Workers (started 2026-09-25 ~22:30; patch-only; the lead integrates)
- W1 hs/ui (hs_compile, hs, ui_widget, ui_widget_game_data_input_functions incl. the C1 lead).
- W2 rasterizer/hud (env_fog, environment, rasterizer_lights, hud_weapon, hud_nav_points, xbox_lights, xbox_models).
- W3 ai/physics/decals (actor_moving, decals, observer, weather_particle_systems, physics, collisions, ai, damage).
- W4 misc (main, network_connection, weapons, progress_bar, structures, virtual_keyboard render only,
  player_effects, action_charge, interface, motion_sensor, players).
- W5 the single long-function mechanism investigation: C3 first order-setting event in `_biped_update_physics`
  (research only).
- 22:50 lead review of work in progress:
  - Card timestamps were NOT clock-read. W1 S1 claims "22:52" and "22:54", but its gate log is 22:41:37 and
    CARDS.md was saved at 22:42:25. W3 T1 claims 05:45Z (22:45), but CARDS.md was saved at 22:41:54.
  - Rule issued to all five workers: clock timestamp (`date`), one immutable card file per card written before its
    first compile, outcomes appended separately, and existing times corrected with an explicit note.
  - W1 S1 was negative, so there is no credit consequence. A card whose time cannot be shown to precede its compile
    supports no claim.
- W3 scope question (point_from_line3d, actor_moving), answered from the owner rulings of 2026-09-21:
  - #1: the guard is relaxed only for the 17 January objects that reference `_point_from_line3d` out of line.
    actor_moving is one of them.
  - #5: any landing needs a genuine shared-header `__inline`, a byte-identical COMDAT vs January's selected copy
    (action_charge, 48 B), strict-exact callers, a full sweep, zero regressions and no hand expansion.
  - bipeds stays out of scope.
  - Every newly emitted symbol must be listed.
- W2 lead (from the width census below): `_render_weapon_hud` does its state-flag work in 16 bits in January
  (`and dx,0xfffd`, `mov ax,[ebp-0x2c]; or ax,1`) and in 32-bit esi in ours. This fits the narrow validated M8 scope
  if SET_FLAG targets the 16-bit object itself. D evidence is required before any card.

## 2b. Bounded sub-items from the companion REPLY_TO_OPUS.txt
- (3) C3 first order-setting event: W5 COMPLETE (research only, 0 bytes, nothing committed). Files:
  `workers/W5/`, objects `scratch/campaign/workers/W5/`.
  - Event: the k-before-j order of the tied pair is set by the S1 merge sort (0x10710df3, comparator 0x10710ed9,
    site 0x1070d425) on the REBUILT product list at call 7482. Its keys are DISTINCT (Pk 0x0104c060 > Pi 0x01044060
    > Pj 0x01040060), and it is repeated at 7486/7490. All later rounds tie (R4 0x0103c047 for both), so the stable
    sort carries the order to emission.
  - Perturbation: all 6 input orders at 7482 give k,j,i; swapping k/j at the next round gives j,k,i.
  - The op261 wrap rule of the round model is FITTED (it missed 2 of 11 arms first), not decoded. This is disclosed.
  - Missing witness: an evidenced source difference that adds +1 (mod 4) to v (nv.k's temp class) and leaves u
    (n.j's) unchanged on the admissible lineage. None is in hand. Qdo + T1 + the held OOB guard removal is predicted
    to give January's C3, but it is held and untestable.
  - Pre-registration `PREREG_W5_C3.md` sha256 4787955e...: the frozen copy was written 23:06:37 and the first
    compile at 23:06:44 (file times verified by the lead).
  - Lead's independent check of the objects:
    - R0 is keyed-identical to the Opus lane's `C2_T1F1G.obj` (195/195 sections, 0/0/0).
    - X1 vs R0: only `_biped_update_physics` changes, 16 bytes inside +0xeb3..+0xecd. Its window reads j,i,k
      (FLD nv,nv,nv), January's @0xea2 reads j,k,i (FLD nv, n, nv), and R0's reads k,j,i (n, nv, nv).
    - X2 is keyed-identical to R0.
  - So the D-attested argument order F4 flips the order-setting comparison to January's direction for j/k, but on
    this lineage it places i between j and k. Both "F4 gives January" and "F4 is inert" are refuted prospectively.
  - Lead correction to W5's section 4: the `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` redirect in bipeds.c is NOT an
    owner hold. bipeds is one of the 17 objects of ruling 2026-09-21 #1. The +0xcc8 inlined-`point_from_line3d(...,
    -distance, ...)` question may be investigated in a CLEAN research arm with the redirect removed. It cannot land
    while `_biped_update_physics` is residual (ruling #5 needs strict-exact callers). The research lane's E3 showed
    that removing the redirect alone does not change the frame.
- (4) Small transfer brief. Census:
  - `scratch/campaign/width_scan.py` (width-marker counts, noisy);
  - `scratch/campaign/width_pairs.py`: aligned same-mnemonic pairs that differ only in operand width
    (`width_pairs.txt`). Eight rows have such pairs:
    - three are owner-held (player_profile_new, saved_game_files, glow);
    - one is biped (held/W5);
    - three are alignment noise across different fields/offsets (crosshairs_draw, env-fog screen_begin,
      convex_hull3d_expand parameter slot);
    - one is a clean width footprint: `_render_weapon_hud`, 2,645 B, not small, in W2's unit. It went to W2 as a
      lead.
  - Result: no SMALL eligible target exists outside holds and owned units, so item 4 is SKIPPED as the reply
    allows. Nothing was compiled for it.
- (5) VK:
  - Seal hardening DONE in the research lane (`claude/opus-mechanisms-20260925`, commit bb8252a7, not pushed).
    `runguard.load_sealed()/read_sealed()` is the single input boundary for all 10 decoders and the exporters'
    evidence reads.
    - Accepted: successful runs only, where the seal is seal()'s own record for that directory and every file
      re-hashes, with nothing added or removed.
    - Refused: FAILED/void, unsealed and tampered runs.
  - Proof:
    - a 134-cell reader x run matrix is identical before and after the edit;
    - `test_sealed_input.py` passes 66/66 (missing, tampered and unsealed inputs, the unfixed-harness contrast, and
      a static no-bypass check);
    - test_runguard passes 48/48 and test_attrib 20/20.
  - e17 NOT run. It would distinguish the pair route from the cross-jump into the fall-through copy, but it cannot
    guide a source witness: January's select has no pair arm falling into END (vS3_F3 FINDINGS section 4). That
    fails the reply's condition, so VK stays parked.

## 2c. Worker results (lead-verified on actual objects with `scratch/campaign/review_patch.py`)
- W2 (rasterizer/hud) COMPLETE: no admissible exact function. Three EXACT candidates are all in held classes. The
  lead applied each patch to a copy, compiled it and diffed it against build/base and January; each gives exactly
  1 gain, 0 losses and 0 added/removed symbols:
  - `_rasterizer_lights_reset_for_new_map` (48 B, 12/1): memset of (MAXIMUM_LIGHTS_PER_MAP+1) records. This is an
    OOB WRITE of 34 bytes into the next .bss object (UB; the campaign text forbids UB/OOB; bug-dependent class).
    New fact for the owner: at cc608036 the next .bss object is `local_lens_flare_parameters`, as in January, so the
    2026-09-22 exclusion's premise (a different array follows) no longer holds. HELD, owner question.
  - `__rasterizer_environment_lightmap_draw` (4,008 B, 43/1): the structural-lane candidate. Stage-0 filter calls
    are duplicated in both texture arms (J corroboration C07: the fastcall register-before-push signature occurs only
    at cross-jumped duplicates, 4 sites in January). It also has two load-bearing named temps that the /Od build
    does not name. HELD, owner question.
  - `_hud_update_weapon_local_player` (1,471 B, 14/2): a restructured candidate whose unreachable default arm leaves
    `result` unassigned, so January reads it only if system_exit returns (uninitialised-read class). HELD, owner
    question.
- W1 (hs/ui) COMPLETE: two EXACT candidates in hs_compile, both owner-gated. With both, hs_compile goes 61/3 ->
  63/1; the last residual is the held `_hs_parse_boolean`. Lead review (copy + compile + base/January diff):
  - `hs_compile_postprocess.patch`: 1 gain (`_hs_compile_postprocess`, 704 -> 720 padded, 714 meaningful), 0 losses,
    0 added/removed. Three factors, each required per W1's factorial:
    - A (bug form): the unparenthesised `a && b && (script = ...)->type == static || script->type == stub` reads a
      stale or indeterminate `script` for an out-of-range index. J: January reloads [ebp-8] on both out-of-range
      exits. D: the later /Od build carries a /RTCu uninitialised-use check of `script` at exactly that read. X: HCEX
      frame locals. A BUG comment is included.
    - B: a write-only `char const *name` from /Od [ebp-0x14]. It explains January's discarded `hs_function_get` call.
    - C: the /Od nested function-arm ifs.
    - Precedent for the owner: aim_grenade (2026-09-24), where a natural /Od original-bug form was admitted as a
      narrow, target-proven exception.
    - Fallback B+C without A: admissible but not exact (720 B / 48 relocations equal January; the script arm differs;
      lead-verified 0 gains/0 losses). Zero credit.
  - `hs_compile_parse_set.patch`: 1 gain (`_hs_parse_set`, 576 -> 560 padded, 554 meaningful), 0 losses, 0
    added/removed. The raw short type values go to the two `%s` conversions: UB on an error path, J-only evidence,
    and the 2020 /Od build has the CORRECTED `hs_type_names[...]` form. By the owner's 2026-09-25 principle ("matching
    bytes alone does not lift those house-rule holds"), this is a weaker case than postprocess.
  - `hs_compile.patch` (both together): 2 gains, 0 losses, 63/1 (lead-verified). The two packets do not interact.
  - W1's other ten targets are negative or blocked; the reopen criteria are in `workers/W1/CARDS.md`. W1 missed a
    prior negative recorded in `research/fifty_objects_20260925/w/ui_widget_game_data_input_functions/LEDGER.md` P1
    (it searched only docs/ and parks). S1 repeated it at no credit cost.
- W4 (misc, 18 rows) COMPLETE: no admissible exact function. Two object-completing packets sit in the open Lane D
  owner classes (claude_lane_d_refresh_HANDOFF_20260922.md, owner queue: A authentic bug/UB, F return ABI,
  H layout-only redundant assignment). All are unruled as classes. Lead review:
  - `weapons.patch` (class F): `long weapon_place(...)` + `return weapon_index;`, weapons.c only (no header, so the
    earlier units regression of the weapons.h route does not arise). 1 gain (`_weapon_place` 208 padded / 193
    meaningful), 0 losses, 0 added/removed. weapons.obj becomes 79/79 (data already 2052/2052), completing the
    object. The object-type table still calls it through a void-returning slot, as January and the 2020 build do.
    The optional `object_types_optional.patch` is byte- and warning-inert.
  - `network_connection_owner_HA.patch`: 2 gains, 0 losses, 0 added/removed. network_connection.obj becomes 23/23
    (data 3417/3417), completing the object.
    - `_network_connection_connect` (288/281, class H): single-exit success flag with a redundant `success = TRUE;`
      (layout-only).
    - `_network_server_close_client_connection` (320/307, class A): loop bound
      `MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1` reads `client_list[4]` = the adjacent `allow_client_connections` field
      (OOB read). The 2020 debug build has the same bound.
  - `_main_update_time` UT-6: 1,440 B / 117 relocations equal to January. The only difference is +0x3ff `mov esi,eax`
    vs January `mov si,ax`. W4's board scan: VC7 emits the 16-bit copy only from a register parameter, a return value
    or a post-increment temp, never local-to-local. Lead-verified 0 gains/0 losses. Zero credit; 9 shapes, stop rule.
  - W4's other rows are negative with reasons in `workers/W4/CARDS.md`. Note: "players RB2" is
    `_player_examine_nearby_device` (decoration-only parenthesis), not `_player_teleport_internal`.
- Zero-credit improvements recorded by W2:
  - fog_screen_begin c8p (26/1): exactness needs held rulings D and E plus frame +4;
  - lens_flares_draw c10 (12/1 combined): two open sites, +0x31c pool history and +0x738 scheduler.
- W2's mechanism reopen criteria are in its CARDS. `_render_weapon_hud`: the width lead was already covered by the
  held fidelity candidate (SET_FLAG on array elements gives January's byte/word ops); what remains is the ESI/EDI
  chooser tie plus the x87 add order.

## 2d. Batch 1: `_actor_destination_update` (W3 T10/T11, landed by the lead)
- Patch `workers/W3/actor_moving.patch` (sha256 4e814be36415cbdc...). It changes actor_moving.c only, in one
  function.
  - The hand-written 2D arithmetic is replaced by the genuine real_math.h `__inline` helper calls that the /Od build
    shows at 0x4627a0. The `step_point` pointers, the `offset` local and the `double t` are removed. The two vectors
    carry their /Od RTC names.
- Lead's independent evidence check, `tools/od_dis.py` on halo_cache_symbols.exe:
  - +0xd2..+0xef: `vector_from_points2d(actor+0xfc = &body_position, &steps[i].point, [ebp-0x1c])`;
  - +0xf7..+0x122: `vector_from_points2d(&steps[i].point, &steps[i+1].point, [ebp-0x2c])`;
  - +0x16e..+0x17c: `dot_product2d(actor+0x140 = &facing_vector, [ebp-0x2c])`;
  - +0x187..+0x18f: `dot_product2d([ebp-0x1c], [ebp-0x2c])`;
  - +0x1b1..+0x1cf: `point_from_line2d([ebp-0x1c], [ebp-0x2c], -distance (sign-mask xor), [ebp-0x1c])`;
  - +0x1d7 and +0x200: `magnitude_squared2d([ebp-0x1c])`, compared against 0.25^2 and 0.15^2;
  - RTC descriptor @0x462cd8: [ebp-28] 8 `actor_to_point`, [ebp-44] 8 `next_step`, [ebp-572] 512 `actorbuf`.
- Ruling #5 (all-inlined COMDAT class) conditions, each verified by the lead:
  - genuine shared-header non-static `__inline` bodies (real_math.h:1009/1020/1053/1109);
  - new COMDATs byte-identical to January's selected copies: `_dot_product2d` 32 B = action_charge,
    `_point_from_line2d` 48 B = action_vehicle (section_infos_equal True). January's actor_moving.obj neither
    defines nor references either;
  - strict-exact caller;
  - selected-provider link PASS for both, in both orders, on the review object and again on the built object;
  - zero regressions (batch gate);
  - the patch REMOVES hand expansions.
- Ruling #6 (rule-24 view casts) disclosure. There are seven cast expressions at four call sites:
  - `(real_point2d const *)` on &body_position, &steps[i].point (x2) and &steps[i+1].point (3D->2D);
  - `(real_vector2d const *)` on &facing_vector (3D->2D);
  - `(real_point2d const *)` / `(real_point2d *)` on &actor_to_point, for point_from_line2d's p and result
    (vector2d->point2d, identical layout).
  - Each is /Od-attested at its site (addresses above) and uses a prefix-compatible layout.
  - Byte-inert: the lead removed all seven and the object stayed keyed-identical (0 changed / 0 added / 0 removed).
- Batch gate `batch_batch1.json` (sha256 6ad956debfa527c7...):
  - ninja rc 0;
  - Halo objects 388/468 (unchanged);
  - code 1,585,386 (+968);
  - functions 7,455 (+1);
  - data 2,587,011 (unchanged);
  - parks 77 (none retired: the row was not parked);
  - stable diff: gained 1 (976 padded B), regressions 0 (snapshot `batch_batch1_stable.json` 903e855615d9cfb3...);
  - admission, fake scan (26 leads) and pytest 1161 passed: all equal to base;
  - new warnings 0 (W3's /W3 census: the two C4244 of the old `double t` disappear);
  - `git diff --check` rc 0.
- Built object `build/base/source/ai/actor_moving.obj` sha256 2cb34eeacbe9ecae...; keyed diff vs the pre-batch
  object: 1 changed (EXACT vs January), +2 COMDATs as above, 0 removed.
- Cumulative strict gain vs the frozen base: +1 function, +968 meaningful B, +976 padded B, 0 objects.
- STATUS: LANE-VERIFIED, pending independent canonical reconciliation (owner instruction 2026-09-26).

## 2g. Owner rulings 2026-09-26 on Q1-Q9 (full text: OWNER_PACKET.md "RULING" column)
- Q6 and Q9 APPROVED, each for its specific packet. Q2, Q3, Q5 and Q8 (H and A separately) HELD. Q4 is held pending
  a corrected bounded packet. Q7: the weapons-only patch must not land; a coherent owner-header/declaration/callback
  packet is required.
- Q1: the nine Codex reservations are RELEASED for investigation (this lane only, canonical and preserved packets
  first, holds kept, old Codex worktrees untouched).
- OWNER_PACKET.md corrected:
  - Q9 emits three helper COMDATs;
  - "no NEW C4013" (hs_compile keeps two inherited);
  - scratch verification is separated from full-board gates.

## 2h. Batch 2: `_hud_update_weapon_local_player` (owner-approved Q6, landed by the lead)
- Packet `workers/W2/hud_weapon.patch` (sha256 16dadb1e6dec1517...), from the fifty-objects after_p5 owner variant.
  At landing the lead corrected the disclosure comments (byte-inert; the object is keyed-identical to the reviewed
  packet object except the COFF timestamp):
  - Default arm: `BUG (preserved for exact matching)`. January (T+0x404) and /Od (0x638ac2) leave `result`
    unassigned there. The arm is unreachable in defined execution: `crosshair_index` takes only 0..18
    (`NUMBER_OF_CROSSHAIR_STATES` = 19 in the TU's enum) and each of the 19 states has a case assigning `result`.
    The lead verified this programmatically: 19 distinct case labels in the first switch, each segment assigns
    `result`. No nonreturn attribute was added.
  - Redundant ammo predicates `(remaining || loaded) && remaining ...`: first-party in both builds.
    - January T+0x205..0x21b: `mov ax,[edi+0x12]; test ax,ax; jne; cmp [edi+0xe],ax; je; test ax,ax; je`, where the
      second test of rounds_remaining can never change the branch.
    - /Od case 3 at 0x638534..0x638599 tests +0x12, then +0xe, then +0x12 again, then compares +0xe with the cutoff.
  - Primary-trigger correction in `_crosshair_state_fired_secondary_with_no_ammo`: both builds test bit 11 = 0x800 =
    `_unit_control_weapon_primary_trigger_bit` (units.h enum index 11). January: T+0x3b7 shared tail `test ch,8`.
    /Od: 0x638a07 `and edx,0x800` on control_flags +0x1cc.
  - Other /Od facts, lead-verified:
    - jump table 0x638c94 body order 16,17,0,1,7,3,4,5,6,2,18,8..15 equals the packet's case order;
    - `state` pointer local [ebp-0xc8];
    - `unit_index` local.
- Batch gate `batch_batch2_Q6.json`:
  - ninja rc 0;
  - objects 388/468 (unchanged);
  - code 1,586,858 (+1,472 over batch 1, +2,440 over the frozen base);
  - functions 7,456 (+1; +2 over base);
  - data 2,587,011 (unchanged);
  - parks 77;
  - stable diff: gained 2 over the frozen base (976 + 1,472 padded), regressions 0;
  - admission (10 candidates / 0 contradicted / 1 rejected), fake scan (26 leads) and pytest (1161 passed): all
    equal to base;
  - new warnings 0;
  - `git diff --check` rc 0.
- Ledger bytes: the semantic ledger credits 1,472 code bytes / 1,472 padded / 68 relocations, sha 0524ed47a94ce36e,
  proof source semantic-coff. The amap triage table had listed 1,471 meaningful; the ledger figure is authoritative.
- STATUS: lane-verified, pending independent canonical reconciliation.

## 2i. Batch 3: `_decal_clip_to_surface` (owner-approved Q9, landed by the lead)
- Landed variant: **P5b**, the R3 reviewer's amended packet
  (`claude-fifty-objects-r3-20260924/scratch/w/review_r3w2_C_decals__decal_clip_to_surface/`, copied to
  `lead/P5b_decals_clip_od_scope_authentic_names.patch`, sha256 4923f3a52ebc6944...), NOT W3's re-measured P5.
  - P5b = P5 + the reviewer's F1 fix: the working counts, collision_bsp, surface, surface_plane and surface_angle are
    declared first inside `if (surface_index!=NONE)` in /Od frame order. The /RTCu init flags are cleared after the
    surface_index test (/Od 0x566a05).
  - P5b also takes first-party local names from the /Od RTC descriptor 0x567690, the /Od assert string at 0x95d348
    and HCEX.pdb.
  - The lead verified P5b byte-inert against P5: the objects are keyed-identical, 0 changed. decals.c is unchanged
    between the reviewer's base b78b76c0 and HEAD, and P5b applies cleanly.
- Approved exception (Q9): the combined named flag mask `!(surface->flags & (FLAG(two_sided) | FLAG(invisible) |
  FLAG(breakable)))`. January tests it with one `test byte [..+8],0xb`. HCEX (PPC) and the 2020 /Od build spell
  three tests. The owner ruled the later spelling does not prohibit this January-compatible one.
- DISCLOSURE, added at landing:
  - The packet removes `#define REAL_MATH_EXTERNAL_PROJECT_POINT3D`. That redirect was added by the integrated
    owner audit of 2026-09-12 (`decals_obj_color_projection_packet_20260912.md` lines 71-79) specifically to stop
    decals.obj emitting `_project_point3d` / `_set_real_point2d`, under the old blanket ban. This landing reverses
    that decision.
  - Basis:
    - January's decals.obj holds `_project_point3d` as an UNDEFINED external (lead-verified: symbol section 0,
      storage 2), i.e. a referenced non-static header inline (ruling 2026-09-21 #1 reasoning);
    - `_point_from_line3d` and `_set_real_point2d` are in the all-inlined class that the folded-inline rule admits
      under its conditions (all met below).
  - The Q9 question (OWNER_PACKET) listed the three helper copies, but it did not state this reversal. It is
    disclosed here; the owner may ask for this commit to be reverted.
  - The packet also replaces the block-scope `convex_polygon2d_clip_to_plane` prototype with the owner header
    `math/geometry.h`, and calls the /Od-attested real helpers (`point_from_line3d` at the zoffset nudge, /Od
    0x567272; `vector_from_points2d`; `cross_product2d`; `project_point3d`).
- Newly emitted symbols (complete enumeration, symbol-table diff base -> P5b):
  - added external DEFINITIONS: `_point_from_line3d` 48 B / 0 relocations; `_project_point3d` 144 B /
    10 relocations; `_set_real_point2d` 32 B / 0 relocations;
  - removed: the UNDEFINED reference `_project_point3d`, now defined in the TU;
  - no new undefined externals, no COMMON, no data/rdata/bss change.
- Helper identities: each copy is section_infos_equal (bytes and all relocations) to January's selected copy:
  `_point_from_line3d` = action_charge; `_project_point3d` (10 relocations) and `_set_real_point2d` =
  path_obstacles. All three are cachebeta PUBLICs (pdb_storage: 112 symbols, 0 disagreements).
- Checks:
  - caller: `_decal_clip_to_surface` strict EXACT;
  - object_audit: base FAIL(4) -> FAIL(3). The remaining three are pre-existing: `_decal_new_from_collision`
    .text (residual) and the `.bss` offsets of `_decal_globals` / `_decal_points2d_temp`, so decals.obj stays
    incomplete;
  - selected-provider probe PASS for all three copies in BOTH orders, on the scratch object and again on the built
    object. This is a duplicate-definition probe between current objects, NOT a complete program-link proof
    (owner's wording);
  - /W3: 12 == 12, identical multiset, no C4013.
- Batch gate `batch_batch3_Q9.json`:
  - ninja rc 0;
  - objects 388/468 (unchanged);
  - code 1,588,626 (+1,768);
  - functions 7,457 (+1);
  - data 2,587,011 (unchanged);
  - parks 77;
  - stable diff: gained 3 over the frozen base (976 + 1,472 + 1,776 padded), regressions 0;
  - admission, fake scan (26 leads) and pytest 1161 passed: all equal to base;
  - new warnings 0;
  - `git diff --check` rc 0.
- Ledger: 1,768 code / 1,776 padded / 59 relocations, sha d875457fa2f02176, proof objdiff + semantic-coff. Built
  decals.obj sha256 8b2613d2b6b4d957...
- STATUS: lane-verified, pending independent canonical reconciliation.
- Cumulative strict gain vs the frozen base after batch 3: +3 functions, +4,208 code B (968 + 1,472 + 1,768),
  +4,224 padded B, 0 objects. All three are lane-verified, pending canonical reconciliation.

## 2e. Wave 2 (started 2026-09-26 ~00:10, base HEAD 455dffad)
- W6: the scheduler store-readiness family. `_actor_move_update` (3,128 B, W3 M4) and
  `_physics_compute_vehicle_collision` (1,160 B, LD2) are each ONE transposition from exact: the integer TRUE store
  into the byte local [ebp-1] issues one slot before the adjacent x87 op, where January issues it after. It is a
  readiness question (one missing predecessor edge, or one extra hidden 0x267 record). Units: actor_moving,
  physics. The read-only sibling check is `_actor_action_handle_vehicle_entry`.
- W7: `_main_update_time` (1,440 B), one instruction from exact (W4 UT-6: +0x3ff `mov esi,eax` vs January `mov
  si,ax`). W4's stop rule stands, so only a decoded C2 copy-width rule plus first-party evidence reopens it. Unit:
  main.

## 2f. GOAL (owner, 2026-09-26): byte-match all remaining Halo functions and objects and link all remaining data,
under `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\HALO_HOUSE_RULES_20260926.md`
- Census at HEAD 455dffad (`scratch/campaign/census_all.py`, which applies the progress report's own semantic credit
  layers; `data/census_all_head.json`):
  - Halo 388/468 objects; 119 functions (184,780 code B) left; data 1,336,440 B left.
  - 80 incomplete objects:
    - 70 have code and/or data gaps;
    - 9 are fully matched admission candidates (`tools/audit_object_admission.py`): game_engine, hs_runtime,
      render_debug, rasterizer_xbox, breakable_surfaces, path_obstacle_avoidance, object_lights (parked by owner
      answer), and path_structure_bsp and rasterizer_xbox_hardware_geometry (both Codex-reserved);
    - 1 rejected (rasterizer_xbox_dynavobgeom, candidate-only-comdat-owner).
  - Data: 1,272,664 B (95%) is `source/linker_common`, the pooled COMMON block. `tools/campaign/linker_common_base.py`
    at HEAD: our tree emits 66/242 pooled records as COMMON at identical size (1,214,917 B); 173 records are absent
    from our tree, 1 differs in size and 2 are non-.bss records (56,773 B). Under the rules on COMMON (pooled storage,
    ownership) and on synthetic comparison bases / verifier changes, this is an OWNER QUESTION (Q10), not worker work.
  - The other 63,776 data B sit in 12 units: hs 54,780; bitmap_drawing 2,644; actions 2,404; main 1,848; projectiles
    1,548; bitmap_group 1,424; ai_communication 276; game_engine_king 136; rasterizer_xbox_profile 132;
    periodic_functions 96 (reserved); s3tc 76 (held assert trap); ai_debug 56.
- Object audit of the candidates (fifty-objects lane battery, installed in `scratch/tools/`): game_engine,
  hs_runtime, render_debug and path_obstacle_avoidance PASS; breakable_surfaces FAILs 2; rasterizer_xbox FAILs 22.
  The fifty-objects adversarial review (wave2 review_admit2) refused game_engine and render_debug on source grounds
  (opaque caller-local views, consumer-local prototypes, hand-expanded helpers, an invented aggregate).
- Wave 3 (started 2026-09-26 ~00:30, brief `AGENT_BRIEF_v2.md`; with W6/W7 this is 8 agents, the owner cap):
  - A1: game_engine admission;
  - A2: render_debug and breakable_surfaces admission;
  - A3: hs_runtime and path_obstacle_avoidance admission;
  - A4: rasterizer_xbox and dynavobgeom admission;
  - B1: hs data (.rdata 51,284 / .data 3,496);
  - B2: misc data (bitmap_drawing, actions, projectiles, bitmap_group, ai_communication, game_engine_king,
    rasterizer_xbox_profile, ai_debug).

## 2k. Published at canonical a8854940 (owner, 2026-09-26): measurement base moves
- The owner independently reconciled and PUBLISHED the three lane functions at canonical `a8854940`:
  - `b91402af`: `_hud_update_weapon_local_player`;
  - `7272341b`: `_decal_clip_to_surface`;
  - `a8854940`: `_actor_destination_update`.
  Their 4,208 code bytes are canonical now and are NOT counted again. Q9 needs no revert: canonical's reviewed patch
  also removes the project_point3d suppression, and its helper copies passed independent identity/provider checks.
- Lane vs a8854940 differs only in decals.c (P5b /Od scope + first-party names) and hud_weapon.c (corrected Q6
  comments); actor_moving.c is identical. The lead compiled canonical's three sources at a8854940: each object is
  keyed-identical to the lane's built object (0 changed / 0 added / 0 removed).
  - So `scratch/campaign/base_stable_a8854940.json` (= batch3 snapshot, sha256 1ed770ffc7c5f63c...) and the
    a8854940 progress figures (388/468; code 1,588,626; functions 7,457; data 2,587,011; parks 77) are the
    measurement base from now on. `batch_gate.py` and `account.py` point to it.
- Zero-credit follow-up (owner: preserve separately; do not reapply the donor commits), in `followups/`:
  - `zero_credit_decals_clip_P5b_od_scope_names_vs_a8854940.patch` (sha256 3933d912600c255e...);
  - `zero_credit_hud_weapon_q6_comments_vs_a8854940.patch` (sha256 fe0ba7f96668fac0...).
  Both produce identical runtime sections.
- Active workers are NOT rebased mid-wave; their units are unchanged between the lane HEAD and a8854940.

## 2l. Batch 4: `_main_update_time` (W7, first gain measured against a8854940)
- W7's `workers/W7/main.patch` (sha256 3d2ad57ed7379493...) makes these changes:
  - the debug arm assigns `main_globals.vblank_interval_current = best_interval;`;
  - a new else arm assigns `main_globals.vblank_interval_current = requested_interval;`;
  - the join reads `target_index += main_globals.vblank_interval_current;`;
  - the local `selected_interval` is renamed `requested_interval`. The rename is byte-inert (W7's
    `main_F1_minimal.patch` is byte-identical); it is kept because the local now holds only the requested
    60/rate value.
  Behaviour is identical on every path: the field ends with the same value and the add reads it back.
- Evidence (facts vs inference):
  - FACT (J): January's false-edge copy is 16-bit (`66 8b f0 mov si,ax` at +0x3ff) with one field store at +0x40b.
  - FACT (C2 9dbf908b, traced stock-equal, seals validated): the pre-marker widening pass 0x1071a490 refuses to widen
    an assignment whose destination is a field at a non-4-aligned offset (0x1071a204 `test bl,3`; flag
    [0x10894d6c]). The else-arm node's destination is the `main_globals` field at 0x3b6, so its type stays 0x1002 and
    it encodes with the 0x66 prefix (encoder 0x10743f2a).
  - INFERENCE: January's else arm assigns the misaligned field directly. No /Od text exists for this function, and
    HCEX's main_update_time is a different 0x174-byte implementation; HCEX only corroborates the field types
    (`short vblank_interval_current`, `short vblank_interval_minimum`).
  - No new locals, casts, helpers or names beyond the byte-inert rename.
- W4's UT-1..UT-9 stop rule was reopened by this NEW fact (the decoded copy-width rule). The first compile after
  the evidence was exact; 1 of 3 allowed forms was used.
- Park retired: `config/parked.json` entry `_main_update_time` (unclassified) removed by line surgery (11 lines,
  CRLF preserved, JSON validated). Parks 77 -> 76, validated by the progress gate.
- Batch gate `batch_batch4_W7.json` vs the a8854940 base:
  - ninja rc 0;
  - objects 388;
  - code 1,590,066 (+1,440);
  - functions 7,458 (+1);
  - data unchanged;
  - parks 76;
  - stable diff: gained 1 (1,440 padded), regressions 0;
  - admission, fake scan (26) and pytest 1161: all equal;
  - new warnings 0 (W7's /W3: 25 == 25, the 4 C4013 inherited);
  - `git diff --check` rc 0.
  - main.obj keyed diff: 1 changed (EXACT vs January), 0 added / 0 removed. The only other change is the
    compiler-internal label `$L29500 -> $L29502` in `_halt_and_catch_fire`, which no relocation uses.
- Ledger: 1,440 code / 1,440 padded / 117 relocations, sha b32677fb6c4cc6bc, proof objdiff + semantic-coff.
- Lane gain vs published canonical a8854940: +1 function, +1,440 code B, +1,440 padded B, 0 objects.
  Lane-verified; not yet reconciled.

## 2o. Owner rulings 2026-09-26 (third set) and landing L1
- Rulings:
  - Q14: descriptive names are allowed for the seven render_debug globals, marked inferred. The ruling does NOT
    approve invented storage boundaries/types or the load-bearing zero initialisers; independent storage/layout
    evidence is needed before D1 lands. D0 is inadmissible. Admission is held.
  - Q15: separate the repairs.
    - (i) The two evidenced symbol-storage corrections are APPROVED, after a fresh split and whole-board
      verification.
    - (ii) Restoring the TU's default flags plus the genuine designator helper is APPROVED as a coupled repair, if
      every exact row stays exact and the emitted helpers pass identity/ownership. Record it as reconstruction, not
      proof of January's flags.
    - (iii) The -3-function regression is NOT accepted.
    - (iv) The local plane-distance helper is NOT admitted.
    - breakable_surfaces stays NonMatching.
  - Q16: the two specific first-party-attested point2d/vector2d view casts are APPROVED (not a blanket exception),
    after verification of size, alignment, member offsets, const correctness, byte-inertness, strict caller
    exactness, helper identity and provider checks in both orders.
  - Optional prototypes: the genuine owner-header corrections are APPROVED with consistent declarations, house
    style, a full consumer rebuild and zero exact losses. Re-measure the affected park, preserve its previous
    evidence and disclose the fuzzy regression.
  - G1: continue, but mechanisms are diagnostic. /Od evaluation order alone does not authenticate parentheses or a
    macro; it needs meaningful source AND January corroboration.
  - Q11-Q13 unchanged.
- L1 = Q15(i), landed. `workers/A2/LEAD_breakable_surfaces_symbols_json.patch` (sha256 46f714aaae9c0865...), two
  in-place line edits in config/symbols.json: `_breakable_surface_effect` and `_globals` gain `"static": true`.
  - Lead evidence: neither name is a cachebeta PDB public (`scratch/tools/cachebeta_publics.txt`: 0 matches), while
    the sibling `_breakable_surface_*` functions are publics. January's own assert strings in the split object
    attest `globals`.
  - Fresh split: ninja regenerated build/split. Of 833 split objects, exactly ONE changed
    (`source/physics/breakable_surfaces.obj`), 0 added, 0 removed (lead-hashed before and after).
  - breakable_surfaces object_audit: FAIL(2) -> PASS. pdb_storage: 26 symbols, 0 disagreements.
  - Full gate `batch_L1_bs_symbols.json`: ninja rc 0; code / functions / data unchanged; parks 76; stable diff 0
    regressions; admission, fake scan (26) and pytest equal; 0 new warnings; diff-check clean.
  - Zero credit. breakable_surfaces stays NonMatching.

- L2 = Q15(ii), landed as the owner-approved COUPLED repair (RECONSTRUCTION, not proof of January's original
  flags).
  - `workers/A2/LEAD_breakable_surfaces_config_flags.patch` (sha256 210f706b36fea775...) removes the per-unit
    `/Ow /QIfist` override from config/config.json (added by PR #35 / 369b71e7 without evidence). The unit returns
    to the project default flags.
  - `workers/A2/breakable_surfaces.patch` (sha256 b5e3e0df8804261a...) replaces the TU-private hand copy
    `breakable_surface_get_plane_from_designator` with the genuine `bsp3d_get_plane_from_designator` (/Od calls the
    shared helper 0x5666f0) and drops a dead `#undef`.
  - build.ninja was regenerated with canonical's configure args, and the lane's absolute CL path was re-applied.
    The only build.ninja difference is this unit's cflags (the `download_tool` rule is defined but unused, as
    before).
  - Exact rows: breakable_surfaces 12/12 before and after. object_audit PASS.
  - Surplus identity: 26 of 27 candidate-only COMDATs are identical to January's selected copies (pre-L2: 20 of
    26). Six helpers become identical under the default flags (`_cross_product3d`, `_plane3d_from_point_and_normal`,
    `_plane3d_negate`, `_project_point2d`, `_project_point3d`, `_vector_from_points3d`). The new
    `_bsp3d_get_plane_from_designator` (112 B) is identical to decals' copy, and the file-private helper is removed.
  - The one remaining non-identical copy, `_real_local_random` vs effects.obj, is INHERITED and unchanged by L2. Its
    fix is the Q15(iii) header change the owner declined (-3 functions).
  - Provider probe: the new surplus passes in both orders. The full-surplus probe fails once, on
    `_real_local_random`, with the identical LNK2005 before and after L2 (inherited).
  - pdb_storage 0.
  - Full gate `batch_L2_bs_flags_helper.json`:
    - ninja rc 0;
    - code / functions / data unchanged;
    - parks 76;
    - stable 0 regressions;
    - admission, fake scan (26) and pytest equal;
    - 0 new warnings (/W3 identical);
    - diff-check clean.
  - Zero credit. breakable_surfaces stays NonMatching: (iii) was declined, (iv) was not admitted, and the
    `_real_local_random` identity/link item remains.

- L3 = Q16, landed. The two owner-approved first-party-attested point2d->vector2d view casts in render_debug's
  `_build_circle_points` (NOT a blanket cast exception), plus comment-only placeholder-name corrections in the file
  header. Taken from `workers/A2/render_debug.patch` (sha256 3b87976628adccd9...). Its two owner-header includes are
  held back for L4.
  - /Od attestation (lead-verified): 0x8450c0 +0xa6..+0xcc pushes `&points[index+1]` (result), cosine, sine and
    `&points[index]` (v), then calls 0x4016bd -> 0x4c9a90. The body at 0x4c9a90 is rotate_vector2d (j = sine*v.i +
    cosine*v.j; i = cosine*v.i - sine*v.j), argument order as real_math.h:1040.
  - Types (real_math.h): `union real_point2d { real n[2]; struct {x, y}; struct {u, v}; }` and
    `union real_vector2d { real n[2]; struct {i, j}; }`. Both are 8 B, 4-byte aligned; the first member is at +0
    and the second at +4, so the layouts are identical.
  - Const correctness: the input cast is `(real_vector2d const *)`, the output cast `(real_vector2d *)`.
  - Byte-inertness: removing both casts gives a keyed-identical object (0 changed / 0 added / 0 removed); /W3 gains
    exactly 2 C4133.
  - Caller: `_build_circle_points` strict EXACT (112 B). render_debug 36/36.
  - Helper: `_rotate_vector2d` (48 B) is newly emitted. It is section_infos_equal to January's ONLY definer
    (path_obstacles.obj) and is a cachebeta public. January's render_debug.obj neither defines nor references it
    (all-inlined case of the folded-inline rule). Selected-provider probe PASS in both orders.
  - object_audit PASS; pdb_storage 63 symbols, 0 disagreements.
  - Full gate `batch_L3_rd_casts.json`:
    - ninja rc 0;
    - code / functions / data unchanged;
    - stable 0 regressions;
    - admission, fake scan and pytest equal;
    - 0 new warnings;
    - diff-check clean.
  - Zero credit. render_debug admission stays HELD (Q14: D1 needs independent storage/layout evidence; D0
    inadmissible).

- L4 = the optional owner-prototype corrections, landed (owner-approved: genuine owner-header corrections, a full
  consumer rebuild, zero exact losses, the park re-measured and disclosed).
  - From `workers/A2/LEAD_owner_prototypes_c4013.patch` (sha256 0883fb5802a1a791...), the prototypes, each in the
    owner header of the unit that DEFINES the function:
    - `render_debug_object_damage` in objects/damage.h (definition damage.c:664). House style (`void` on its own
      line) was applied by the lead; A2's packet used the legacy one-line form.
    - `render_debug_recording` in cutscene/recorded_animations.h (definition recorded_animations.c:379);
    - `render_debug_fog_planes` in structures/structures.h (definition structures.c:573).
    - damage.c's duplicate local prototype is removed.
  - render_debug.c includes its genuine owner headers: ai/ai_debug.h and physics/collision_debug.h (held from
    `render_debug.patch`) plus cutscene/recorded_animations.h and objects/damage.h
    (`render_debug_includes_after_LEAD_prototypes.patch`, sha256 db2e15f9af25446f...).
  - Result: render_debug C4013 5 -> 0 (/W3, lead-measured); damage.c has no C4013; the render_debug object is
    keyed-identical.
  - Full consumer rebuild (batch gate, full ninja): 0 exact losses; stable diff 0 regressions; code / functions /
    data unchanged.
  - DISCLOSED FUZZY REGRESSION, park re-measured by the lead: parked `__rasterizer_model_draw` (tu-context-
    optimization) moves normalized sha 89b0d7ea... -> cbfa85852c836af1 at the same 5,168 padded B / 348 relocations,
    objdiff 3.3.1 95.08453 -> 95.07629. The cause is the structures.h prototype shifting this TU's declaration
    context. The park entry is re-baselined by `workers/A2/LEAD_parked_model_draw_rebaseline_IF_C4.patch`
    (sha256 c45cc661b45b5932...): the previous evidence text is preserved verbatim, with an appended 2026-09-26
    note, and the progress gate validates all 76 parks.
  - Full gate `batch_L4_prototypes.json`: ninja rc 0; admission, fake scan (26) and pytest equal; 0 new warnings;
    diff-check clean. Zero credit.

## 2p. Lead error: K1/K2 brief leverage; K2 result
- LEAD ERROR, recorded: the K1 and K2 briefs cited "missing" relocation identities (`__real@3dcccccd` x30,
  `""` x16; `""` x3, `3d088889` x3) read from raw, TRUNCATED `sig.txt` rows without normalising csplit's
  `symbol:X` against our `defined-noncode:.rdata:X`. After normalisation, both functions' reference multisets are
  equal to January's. The applicability map's count of 26 real-delta rows (which normalised correctly) is
  unchanged, and neither function is among the 26.
  - K2 found this. K1 was told to recompute on 2026-09-26.
  - `scratch/campaign/sig.py` now normalises the spelling (K2's patch).
- K2 (`_ai_debug_render_actor`, 24,964 B) COMPLETE, negative:
  - 14 /Od-sourced shape edits each leave their own block's bytes unchanged; they only flip whole-function-state
    x87 operand-order sites.
  - Width-normalised residual +43 B by family:
    - gun cross-jump from the def-stand magnitude term order: +22;
    - marker fst/fstp: +15;
    - vehicle-avoidance `t` mov vs fld/fst plus push swaps: +7;
    - alignment / vision: +4 / -7;
    - prop term order: +2.
  - /Od is proven NOT January at the aiming p0 (/Od calls point_from_line3d(-0.04f); January uses fsubr).
  - Reopen criteria are in K2/CARDS.md.
  - Zero-credit hygiene patch E13 (an unreferenced local, C4101 1 -> 0, object identical) is offered (Q-K2-2).
  - K2 recommends NOT landing the /Od fidelity set E1-E12/E14 (Q-K2-1): no byte gain, and /Od differs from
    January here.

## 2n. Batch 5: `_physics_compute_vehicle_collision` (W6)
- Patch `workers/W6/physics.patch` (sha256 86e88ac96dd02b7a...). Four hunks, each attested in the /Od build
  (lead-verified with `od_dis.py`, /Od body 0x7bb7d0):
  - the four accumulators declared bare, then `set_real_vector3d(&x, 0.0f, 0.0f, 0.0f)` x4 (/Od +0x96/+0xbc/+0xe2/
    +0x108, calls to 0x405fc4 -> 0x42e2b0, three xorps zero arguments each). This separates declaration from
    initialisation; it is disclosed under the "combine declaration and initialisation" rule and justified as
    independently /Od-evidenced and load-bearing;
  - `force_magnitude = 2.0f * mass_scale * global_gravity / global_physics_collision_depth * penetration`, verbatim
    left-to-right as /Od computes it (+0x27c..+0x299: 2.0 * [ebp-0x14] * [0xa35d38] / [0xa35d44] * [ebp-0xbc]);
    the earlier redundant grouping `(gravity / depth)` is dropped;
  - the real `point_from_line3d(&point0, &direction, mass_point0->radius - penetration, &collision_point)`, with no
    casts (/Od +0x320, 0x409ab1 -> 0x42e0d0), replacing a hand expansion;
  - `collision = TRUE;` as the last statement of the if-block (/Od +0x3f8, after the fourth add_vectors3d).
- Mechanism (W6, a compiler-state explanation, not a source proof):
  - each parenthesised FP subexpression gets a hidden 0x267 temp record, which takes an integer issue slot in the
    scheduler;
  - scheduling regions cap at 81 records (0x1074e31e);
  - the extra grouping's hidden temp pushed `fstp [ebp-0xa0]` out of window A;
  - the /Od initialisation shifts slot IDs so the operand sort matches January.
  The source claims rest on the /Od build.
- New symbol `_point_from_line3d` (48 B, 0 relocations) is section_infos_equal to January's selected copy
  (action_charge). January's physics.obj neither defines nor references it: an all-inlined TU, which the
  folded-inline rule explicitly admits under its conditions:
  - strict-exact caller;
  - identical copy;
  - selected-provider probe PASS in both orders (scratch and built object);
  - ownership (pdb_storage 46 symbols, 0 disagreements; the helper is a cachebeta public);
  - zero losses.
  Symbol diff: +1 external DEF, nothing removed, no COMMON or data change. object_audit FAIL(4) -> FAIL(3); the
  remaining three are the other residual functions.
- Batch gate `batch_batch5_W6_physics.json` vs a8854940:
  - code 1,591,226 (+2,600 vs a8854940; +1,160 this batch);
  - functions 7,459 (+2);
  - objects 388;
  - parks 76;
  - stable +2 over the base, 0 regressions;
  - admission, fake scan (26) and pytest equal;
  - 0 new warnings (/W3 12 == 12);
  - diff-check clean.
- Ledger: 1,160 code / 1,168 padded / 14 relocations, sha 5aae4029c87d4654. Lane-verified; not yet reconciled.
- W6's other result: `_actor_move_update` (3,128 B) is EXACT only with a TU-private descriptive macro
  `actor_move_distance_squared(d) ((d)*(d))`. It is load-bearing (hidden 0x267 temp), and its precedent is the
  owner-admitted actor_perception_distance_squared macro (an EXACT donor with the same schedule). Owner question Q13;
  the same question is open for actions.c `_actor_action_handle_vehicle_entry`.
  RULING 2026-09-26: Q13 HELD, no macro exception granted. The actions.c sibling stays held. Both candidates are
  preserved at zero credit.

## 2m. B1 (hs data) COMPLETE: blocked on a scorer/verifier decision (owner question Q11)
- hs.obj data needs no source change. `.data` (3,496 B, 492 relocations) and the main `.rdata` (13,500 B, 1,715
  relocations) are byte- and relocation-identical, and all 908 January literal COMDATs are present and identical.
  Ours has 20 extra literals, exactly January's 20 undefined literal targets, each identical to its selected
  provider (provider_link 20/20 PASS both orders).
- objdiff 3.3.1 cannot credit them because of its `$` literal-name defect. January's own hs.obj scored against a
  byte-identical copy caps at `.data` 99.52 / `.rdata` 99.91. Renaming the `$` names makes it 100/100; objdiff
  3.6.0 scores the production pair 100/100.
- The stock semantic_data_matches verifier fails closed on the hs group (extent sum 51,328 vs objdiff's 51,284). The
  reviewed verifier patch on branch `claude/opus-data-verifier-20260925` (fc53d5f6, `extent_model`) plus B1's
  910-member entry credits +54,780 data B with every other stage identical. B1 proposes no landing. Proposals are
  in `workers/B1/proposals/`.
- Rule tension (B1's disclosure): an extent-model change for grouped entries is what rule 57 forbids "merely to
  remove a scoring obstacle"; rule 52 requires separate review for comparator fixes. This is an owner decision.

## 2j. Queue for the next free worker slots (after rulings 2026-09-26)
- Q7 coherent packet (weapons + object_types + owner header). It must include:
  - `long weapon_place(...)` in its genuine owner declaration;
  - the object_types.c scenario-datum parameter type made consistent;
  - the void-returning callback typed consistently;
  - disclosure of any remaining target-specific incompatible call;
  - a zero-regression consumer sweep;
  - no byte-inert local declaration or cast used to hide the disagreement.
- Q4 corrected bounded packet (rasterizer_lights reset). It must include:
  - an object-layout check of the ACTUAL offsets/extents (not only sizes);
  - a documented overwrite-before-use lifecycle with its assumptions;
  - no unconditional "harmless" claim.
- Released units (Q1), investigation only, holds kept, old Codex worktrees untouched:
  - their 11 function rows;
  - the admission candidates path_structure_bsp and rasterizer_xbox_hardware_geometry;
  - periodic_functions `.data` (96 B).
  Read canonical and the preserved Codex packets first.
- Q10 (COMMON pool): awaiting the owner.

## 2q. Fourth-set (bounded) rulings: landings 2026-09-26 (lane-verified, pending independent canonical reconciliation)
Base for every figure: canonical d890c2db (batch_gate BASE; stable base = scratch/campaign/base_stable_d890c2db.json).
Each packet ran `python -B research/compiler_application_20260925/tools/batch_gate.py <label>`:
- a fresh ninja build;
- stable_verdicts snapshot plus a diff vs the base AND vs the previous packet's snapshot;
- park validation (from the progress step);
- audit_object_admission;
- fake scan and pytest;
- warnings (line-stripped multiset);
- diff --check.
Unit-level checks as listed. Objects were compared with scratch/campaign/objhash.py (TimeDateStamp masked) and
secdiff.py (--norm-labels) for shared-header packets.

| commit | packet | class | effect vs previous packet | key evidence |
|---|---|---|---|---|
| 700ca0d1 | wave-3 research records (split out of old 453bdbb5) | records | none | COMMIT_SPLIT_20260926.md |
| 83e1921e | E13 ai_debug unused string local | zero credit | object identical | (split; tree unchanged) |
| 219c2c72 | periodic x-table builder | CODE +244 | +1 function, park retired | (earlier gate) |
| eb7270b3 | periodic_functions.h style | zero credit | none | |
| c5366682 | periodic_functions Matching | OBJECT +1 | 389/468 | complete object audit |
| f4a13922 | measurement tools on d890c2db | records | none | |
| 465cfd36 | K1 packet records | records | none | |
| 5901f1eb | E1-Q2 find_or_add_edge | CODE +240 | +1 function | gate 8/2; keyed 1 changed; sha 5fbffc0e.. = January |
| 08200b26 | path_structure_bsp /Od loop | zero credit | object sections identical | audit PASS; admission held |
| 2717b687 | hardware_geometry Unlock stubs + 11 symbols.json rows | zero credit | 1/833 split objects changed | 17/17; audit PASS; pdb 2 (held pair) |
| bd68541c | files volume names [N][256] | zero credit | only files.obj differs (COMMON 256 -> 512) | 16 includers swept |
| 387bab37 | Q12 three data entries | DATA +1,756 | only 3 units' data | gates/Q12_EVIDENCE.md |
| b30042ba | Q12 king data entry | DATA +136 | only king's data; audit candidates 10 -> 11 | gates/Q12_EVIDENCE.md |
| 602846b8 | A3 pao helper repair (hunks 2-5) | zero credit | +4 COMDATs identical to action_charge's | provider link both orders PASS |
| 91f4824b | A3 hs_runtime type corrections (H-1c, H-2, H-8) | zero credit | object identical; C4090 1 -> 0 | 66/66 |
| be21ae0a | HDR-1 gc prototypes | zero credit | 17 consumers: code/data identical | C4013 hs_runtime 2 -> 0, hs_compile 2 -> 1 |
| 0abc5d2c | HDR-2 hs_external_global_definition in hs.h | zero credit | 13 consumers: code/data identical | |
| 76ca44cd | A4 D1 dynavobgeom dead global | zero credit | ?warned keys to January | audit FAIL(2) -> PASS |
| 250a8f4b | A4 R-A/R-B rasterizer_xbox + 24 symbols.json rows | zero credit | 1/833 split objects changed | 95/95; audit FAIL(22) -> PASS |
| 32140e5c | A4 W0 window views (10 files) | zero credit | 2 objects differ only in $-labels | /W3 identical in all 10 |
| 12a714a4 | K1-E polarity | zero credit | event 8128 -> 8096 | review_patch 0/0 |
| ea3b848e | K1 structural (dead initializer removed) | zero credit | hunks 96 -> 72; frame 0x1270 disclosed | review_patch 0/0 |
| 7c0d73f2 | K1-A protagonist_invalid flag | zero credit | hunks 72 -> 67 | == K1 copies/P3.c |
| 9c350e37 | COMMON doc correction | docs | none | |

Every full gate: 0 regressions, no new warnings, fake scan 26 board leads (0 in every touched file), pytest 1161
passed / 5 skipped, parks 75 (after the periodic retirement), admission contradicted 0 / revoked 0.
Held and preserved, NOT landed:
- landing/HELD_A3_pao_od_shape_names_remainder.patch;
- landing/HELD_A3_hs_runtime_remainder.patch;
- A4 LEAD_RA_rasterizer_xbox_internal.h.patch + LEAD_RA_parked.json.patch (deferred);
- A4 OWNER_* and CONDITIONAL_* patches (OQ-1..4);
- A3 hs_runtime_OWNER_hcex_abi*.patch (Q-A3-2);
- K1 held forms B/C/D (held_forms_delta_adm_to_lab.diff).

## 3. Owner questions (accumulating)
- Q1. The 9 Codex-reserved units (path_structure_bsp, xbox_sound_cache, dead_camera, cinematics,
  periodic_functions, lightning, hardware_bitmaps, hardware_geometry, connected_geometry) hold 11 remaining rows.
  Their lanes are unmerged and inactive since 2026-09-24. Are the reservations still current?
- Q2-Q6 are lead-verified EXACT candidates in held classes. For each: 1 gain, 0 losses, 0 added/removed symbols,
  only its own object changes. The full texts will be assembled for the handoff.
  - Q2. `_hs_compile_postprocess` +714 B (A: unparenthesised script test, /RTCu + J; aim_grenade-like).
  - Q3. `_hs_parse_set` +554 B (raw type shorts to `%s`; J-only; /Od has the corrected form).
  - Q4. `_rasterizer_lights_reset_for_new_map` +48 B (34-byte OOB memset into the next .bss object; the layout
    premise of the 2026-09-22 exclusion changed).
  - Q5. `__rasterizer_environment_lightmap_draw` +4,008 B (duplicated stage-0 filter statements with J cross-jump
    corroboration; two load-bearing named temps the /Od build does not name).
  - Q6. `_hud_update_weapon_local_player` +1,471 B (unreachable default arm leaves `result` unassigned;
    uninitialised-read class).
  - Q7. `_weapon_place` +193 B, class F (return ABI), completes weapons.obj.
  - Q8. `_network_connection_connect` +281 B (class H) and `_network_server_close_client_connection` +307 B
    (class A, capacity loop OOB read). Both together complete network_connection.obj.
