# editor_flying_camera wave-2 worker ledger (claude-fifty-objects-20260925, base 931ed8dc)

Target: _editor_camera_set_position_and_roll (January 432 padded / 30 relocs).

## Baseline
- gate --all on unmodified source: 20 exact / 1 residual; residual `_editor_camera_set_position_and_roll [size 464!=432, sha]`.
- Prior ledgers read: static_split_recovery_20260923, opus5_150k_w1, opus5_150k_w3, unwritten_packet_20260912.
- Recorded negatives (do not repeat): named `real roll_sign = dot_product3d(...)` before angle (s1, inert);
  dot_product3d argument swap (spr_a2, inert); spr_a1 = euler prefix cast form -> 432/30 w/ 3 x87 blocks.

## Evidence gathered (wave 2)
- January bytes (tinfo/dis_range): frame 0x60; no-camera path is the fall-through, passes `angles` (esi) straight to
  editor_camera_set_focus, duplicated tail (initialized=1; reset_all=1; ret). Camera path: rotation -0x60 (up at -0x44),
  facing -0x2c, roll_reference -0x20, up -0x14, up_angles -8; dot spilled to [ebp+8] before angle_between call.
- M8 corpus (opus5-150k research mod8all.jsonl): this function is MOD8-INSENSITIVE (dead locals never move it) -> the
  3 x87 blocks are NOT an IL-local count residual; operands are frame-local aggregate fields.
- HCEX.pdb SHIP compiland does NOT contain set_position_and_roll (no locals); HCEX_Release.pdb has the symbol but no
  child locals. halocea/src/editor_camera_set_position_and_roll.c is an LLM decompile (names not authoritative).
- **/Od build (halo_cache_symbols.exe) function at 0x527a20** (aligned-frame prologue, missed by odbuild `file`;
  found via od_linear 0x5279f0..0x527de0 and the "hack_angles" string ref). RTC descriptor 0x527c9c names the
  address-taken aggregates in declaration (frame) order: `matrix`(52) `forward`(12) `left`(12) `up`(12) `diff`(12)
  `hack_angles`(8). Statement order: asserts (lines 0xad/0xae = 173/174 in that revision); `if (!editor_camera)
  { editor_camera_set_focus(point, angles); initialized = TRUE; } else { position copy; matrix4x3_rotation_from_angles(
  &matrix, yaw, pitch, roll); euler_angles2d_from_vector3d(&cam->facing, &matrix.forward); vector3d_from_euler_angles2d(
  &forward, &cam->facing); hack_angles = cam->facing; hack_angles.pitch += pi/2; vector3d_from_euler_angles2d(&up,
  &hack_angles); normalize3d(&forward); normalize3d(&up); cross_product3d(&up, &forward, &left); normalize3d(&left);
  cross_product3d(&up, &matrix.up, &diff); normalize3d(&diff); roll = angle_between_vectors3d(&up, &matrix.up) *
  dot_product3d(&forward, &diff); if (unit_focus != NONE) unit_offset = *point; } reset_all = TRUE;`
  `left` is dead in that revision and absent from January (only 3 normalize3d calls, frame 0x60) -> later addition.
  set_focus receives the euler3d `angles` pointer unchanged (no copy) in /Od too.
  Dump: od_spar.txt; names: rtc_at.py.

## Probes
- P0 c2.c (= w3 c2 reproduced: `if (!editor_camera)` first + `(real_euler_angles2d const *)angles` cast):
  432/30, sha-only; alndiff: exactly the 3 x87 blocks (cross i-term 2nd product; dot k and j products). Reproduces w3.
- P1 p1.c = c2 + authentic /Od local names (matrix, forward, up, diff, hack_angles): 432/30 raw sha c334539c5fd00fb7
  == c2 byte-identical. Names are INERT for the x87 order.
- P2 p2.c = p1 + /Od dot argument order dot_product3d(&forward, &diff): byte-identical to c2 (confirms w1 spr_a2).
- L1 (LAB ONLY, hand-copied helper = /Od 0x43e710 cross_product3d body: k,j locals, i stored directly): byte-identical
  x87 result (3 blocks unchanged). The cross_product3d body shape is not the lever. January action_obey's out-of-line
  `_cross_product3d` COMDAT is byte-identical to ours (current header body agrees with January's selected copy).
- Hypothesis H2 (from /Od): the later revision computes `cross_product3d(&up, &forward, &left)` BEFORE the diff cross.
  If January had that statement with `left` dead (no normalize3d: January has exactly 3 normalize calls, frame 0x60 has
  no left slot), its loads would CSE-age up.* and forward.* fields: predicted flips = dot j/k to diff-first (forward
  aged) and cross i-term 2nd product to rot.up.j-first (up.k aged). Directionally matches all 3 blocks. Test in lab.
- NOTE: my first bodies (c2/p1/p2/L1/L2 first builds) had single-backslash __FILE__ literals (heredoc escaping) -> the
  file-string relocation showed as reloc-identity; fixed in all .body files and rebuilt. The x87 conclusions above
  are unaffected (instruction bytes identical; only the string symbol differed).
- **L2 = p2 + `real_vector3d left;` (declared after forward, /Od RTC order) + `cross_product3d(&up, &forward, &left);`
  immediately before the diff cross product (/Od statement order), NO normalize3d(&left): gate --all = 21/21 EXACT.**
  All 3 x87 blocks flip to January's order at once, as H2 predicted (CSE aging of up.* / forward.* leaves by the dead
  cross product). `left` gets no frame slot (dead whole object costs 0; frame stays 0x60).
- Strip / sensitivity tests around L2 (all gate --all, raw function sha vs L2 71253260755d0770):
  - S1 no euler cast (`editor_camera_set_focus(point, angles)`): 21/21 EXACT, byte-identical -> the cast is BYTE-INERT
    (plain spelling only adds C4133 incompatible-pointer warning). Pass-through itself is /Od-attested (0x527ab6).
  - S2 old names (rotation/facing/up_angles/roll_reference) + left: identical -> names inert.
  - S3 dot args (diff, forward): identical -> inert.
  - S4 `left` declared last: identical -> declaration position inert.
  - S5 left cross moved AFTER the diff cross: NOT exact (cross i2 + dot k/j still wrong, dot i also flips) ->
    statement POSITION is load-bearing; only the /Od position (before the diff cross) matches.
  - /W3 census: L2 adds no warning vs base; S1 adds C4133 at the set_focus call.
- **F1.c (final candidate)** = base + set_position_and_roll rewritten in /Od form: `if (!editor_camera)` first with the
  byte-inert `(real_euler_angles2d const *)angles` prefix cast; /Od RTC names and declaration order (matrix, forward,
  left, up, diff, hack_angles); /Od statement order incl. `cross_product3d(&up, &forward, &left);` (no normalize, left
  never read); `dot_product3d(&forward, &diff)`. gate --all: **21/21 EXACT**, raw sha 71253260755d0770 (= L2).
- Specificity tests on F1:
  - T1 dead cross with swapped args `cross_product3d(&forward, &up, &left)`: 21/21, byte-identical (the dead cross ages
    the same six leaves either way).
  - **T2 = F1 + `normalize3d(&left);` right after the left cross (the /Od later revision VERBATIM): 21/21 EXACT,
    byte-identical to F1.** VC7 removes the dead inline-header normalize3d(&left) call together with the dead cross
    product. My earlier inference "January has 3 normalize calls so no normalize(&left)" was WRONG: January's bytes are
    fully consistent with the complete /Od statement pair. The whole function body is therefore the later first-party
    revision's statement list verbatim (names, declaration order, statement order, argument order), the only spelling
    difference being the unit_offset copy (see T3).
- T3 = T2 with /Od-attested aggregate `unit_offset = *(real_vector3d const *)point;` (the /Od copies with integer movs):
  21/21, byte-identical -> byte-inert; NOT adopted (keeps the existing component-wise copy, no new view cast).
- **F2.c (FINAL)** = F1 + `normalize3d(&left);` (T2, /Od verbatim) with an explanatory comment. 21/21 EXACT, raw sha
  71253260755d0770. object_audit PASS (40 January-owned sections ok, 47 symbols 0 differ); surplus list and every
  surplus COMDAT section identical to production (34 compared, 0 differ; production surplus_identity: 17 code COMDATs
  0 not identical); provider_link PASS (35 rows incl. strings); every data section identical to production.
- pdb_storage (production, unchanged by F2): 2 PRE-EXISTING disagreements: `_editor_camera_speed` (.data anchor) and
  `_editor_camera_speed_steps` (.rdata anchor) are external in split+ours but absent from cachebeta publics
  (`_editor_custom_render` at .data+8 IS public). HCEX names the .data+0 static `speed`. Not introduced by F2.
- Control C1 (LAB): F2 with the dead cross over UNRELATED vectors `cross_product3d(&matrix.left, &matrix.position, &left)`
  (same local/inline-temp count as F2): NOT exact, the 3 original blocks return unchanged. So the closure is
  content-specific (the dead cross reads exactly up.* and forward.*, the leaves whose load order flips), not a
  name-count/ordinal artifact (consistent with the corpus MOD8-INSENSITIVE verdict). Mechanism = CSE leaf aging
  (x87 research S2.1-S2.3: an operand whose address tree was already evaluated loads SECOND).
- fake_match_scan: F2.c 0 leads (production file 0 leads).
- /W3: F2 adds no warning vs production.
- production.patch (base -> F2) generated; `git apply --check` clean against 931ed8dc.
- Packet B (optional storage fix, separate): T4 = F2 + `static` on editor_camera_speed and editor_camera_speed_steps
  (cachebeta: not public; HCEX: .data+0 is File Static `speed`). gate 21/21 vs real split; object_audit vs real split
  FAILS only on those 2 storage classes (expected until symbols.json `"static": true` + csplit regen); against an
  EMULATED split (split_static_emulated.obj = real split with only those 2 storage bytes patched 2->3):
  gate_emul 21/21, object_audit PASS. Risk: objdiff 3.3.1 may undercredit the static-anchored .data/.rdata sections;
  the camera-static-sections grouped entry pins owner storage and must be re-measured.

## Final result (wave 2)
- `_editor_camera_set_position_and_roll` strict EXACT (432/30) -> editor_flying_camera 21/21 functions EXACT.
- Integrator packet A (required): `scratch/w/editor_flying_camera/production.patch` (git apply --check clean at 931ed8dc;
  reproduces F2.c exactly). Retire park config/parked.json `_editor_camera_set_position_and_roll`
  (source/camera/editor_flying_camera). config.json line 741 status NonMatching -> Matching once the full chain passes.
  config/semantic_data_matches.json camera-static-sections `reason`: drop the stale sentence "The remaining camera code
  function stays residual." (allow_incomplete_unit becomes unnecessary but harmless).
- Packet B (optional, separable): `storage_optional.patch` + `config/symbols.json` copy (lines 14089 and 22443 get
  `"static": true`), then csplit regen and re-pin the grouped entry's `_editor_camera_speed` owner storage 2 -> 3.
  Fixes the 2 PRE-EXISTING pdb_storage disagreements. Emulated-split evidence only; objdiff 3.3.1 data credit must be
  re-measured by the integrator.
- Policy disclosure for the reviewer:
  (1) euler3d->euler2d prefix view cast at the set_focus call: /Od-attested pass-through (0x527ab6..0x527abe pushes the
      angles parameter unchanged), prefix-compatible unions (yaw@0, pitch@4), byte-inert strip test (S1), strictly exact
      caller -> meets brief section 2 criteria for a new view cast.
  (2) `left` + `cross_product3d(&up, &forward, &left); normalize3d(&left);`: dead in January (VC7 discards both; T2), but
      it is the later first-party build's statement pair VERBATIM (RTC name `left`, declaration position, statement
      position and operands). Load-bearing (strip -> P2 not exact); position-specific (S5 not exact) and
      content-specific (C1 not exact); not a count/oracle effect (MOD8-INSENSITIVE, C1). This touches the section-7
      "dead named local" wording; I classify it as attested source, not a filler, and the reviewer should rule.
