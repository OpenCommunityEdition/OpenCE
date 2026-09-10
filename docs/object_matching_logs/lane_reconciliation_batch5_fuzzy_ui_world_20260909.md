# Lane reconciliation batch 5: retained UI/world fuzzy bodies (2026-09-09)

## Verdict

This ledger records the current generated evidence for sixteen complete or
best-known natural bodies in four translation units.  **All sixteen remain
residual and receive zero exact code credit.**  None appears in the current
semantic-exact or accepted-ledger lists, and this document does not create a
semantic exception, mark an object complete, or convert objdiff similarity
into matching credit.

The bodies were inspected in current source and compared directly between
`build/split` and `build/base` with the hardened COFF reader and the
alignment-aware, relocation-normalized Capstone stream in
`tools/campaign/alndiff.py`.  The measurements below come from
`build/report.json` (SHA-256
`0a05100cdffccd34584a640ea209264c7e8db70c259826e401450093f30247a4`),
generated at 2026-09-09 21:33 local time.  Objdiff percentages are diagnostic
only.

Across this ledger the January owners contain 7,195 meaningful / 7,328 padded
bytes and 334 relocations.  The candidate sections contain 7,216 padded bytes
and 331 relocations.  Credited bytes: **0 meaningful / 0 padded**.

## `source/interface/hud_draw.obj`

| Function | Meaningful | Target / candidate padded | Target / candidate relocs | Objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `_hud_draw_bitmap_internal` | 544 | 544 / 544 | 16 / 16 | 86.40120% | `afc1044b43f5f168eed9a6f96553231667e861929533abbac26a2b229df5b972` | `fcfc4a4eebd7b00d6325212d2d737886467259b5e79b8bfada2142ea5908445d` |
| `_hud_draw_bitmap_with_meter` | 374 | 384 / 400 | 17 / 17 | 92.69841% | `e44c9d5bbd7d5279cc45630280f6b347dfc572ab2e635e4a5a84ad940d3ac4e8` | `3cc01a61674255de737e68476c3052952b668dafb47a0c135c1ecc92a4c54c8b` |

Current source supplies the full typed bitmap path: placement and clip
selection, per-corner rotation and texture coordinates, dynamic-screen-quad
parameters, meter hookup, multiplayer scaling, and the January stack-sentinel
diagnostics.  Both candidate functions have the same relocation-call multiset
as January, including their real render helpers and both assertion paths.

- `_hud_draw_bitmap_internal` aligns 106 of 167 target instructions against
  176 candidate instructions.  The remaining mismatch is distributed through
  register ownership and x87 scheduling in the four-vertex loop, the prologue,
  parameter loads, and sentinel epilogue; equal size and relocation count do
  not make it exact.
- `_hud_draw_bitmap_with_meter` aligns 99 of 136 target instructions against
  144 candidate instructions.  January uses a `0x22c` local frame while the
  candidate uses `0x230`; boolean materialization, local rectangle offsets,
  and the final private-call argument schedule account for the extra 16-byte
  candidate section.

The pre-existing naked `get_return_eip` helper is independently documented in
the accepted HUD stack-sentinel family.  These bodies merely call it; they add
no assembly or synthetic owner.  The exact bitmap retrieval pair is covered
separately by
`docs/object_matching_logs/hud_draw_obj_bitmap_owner_pair_reconciliation_20260909.md`.

## `source/interface/motion_sensor.obj`

| Function | Meaningful | Target / candidate padded | Target / candidate relocs | Objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `_blip_begin` | 99 | 112 / 112 | 10 / 10 | 99.92000% | `02831c4d76978889bea0fbdb6c45941bc96272e613d3005cb0d7c7cdd73d0f5e` | `6a9a7351f54ec563c0c7be8deed0ac2f8b6bdf40a80f8b97aaa054aba73675e4` |
| `_blip_type_get` | 351 | 352 / 368 | 21 / 21 | 82.34782% | `cb65a91e416950514c828a2fbed685df031a289140a3218812a754128d8ad64e` | `96890d3f5ec91d4143be970194bfa3cfdeb3d43cc5a1e85c699db8d4b1b3c948` |
| `_motion_sensor_blip_set_type_and_size` | 87 | 96 / 96 | 4 / 4 | 89.84849% | `c28dfa90e22bd48348034ae8cb472a5b6d0ebe45476d8ff47bcdaa672c060a5d` | `9017714cf6af70be634c7df315274d181143f59c15e804a30813e339c331574e` |
| `_motion_sensor_update` | 1,154 | 1,168 / 1,136 | 43 / 44 | 60.59827% | `a8c62ddee9e7e5718949844c1e68489d08cc78c86da8f29f842b5d61f450b713` | `a500a22dabf0b552854bfa0f08476cc706ba2062cb3c0f3b59d89d706f99c0c6` |
| `_render_blip` | 388 | 400 / 384 | 18 / 17 | 67.72868% | `67347be9f029e06db7df599121540e3f2d416555d004f40e760cf1065dbbef09` | `2eeacc63ed8514975642035ea2087f3baced8d1db8ec0e99a655b2e4ecfc40d9` |
| `_render_motion_sensor` | 753 | 768 / 768 | 40 / 40 | 92.60455% | `954bdcc559c91d9e79948d1b28ce0a380e2b7cbe48030891ad5da5620dbf4cff` | `fe7bcd275ed517944e3ed18c3f8f940174e8d6697e8fbaf2b4a3e8c350dd5557` |
| `_should_draw_object` | 211 | 224 / 208 | 7 / 7 | 85.98824% | `13cd9e4469147c6d8d340c8f80647e653a8e0ef1bf957b9cfb7b09eef6360f66` | `8b893ff8a14a03489422f2a2bd1d6de7dc1ff67db399367a3949da5bb4fcd02d` |
| `_update_motion_sensor` | 849 | 864 / 800 | 39 / 37 | 74.21111% | `b37814ed8aaa3427a5d8e1fe1327e22cc80190383562143e94c35df03f5db06b` | `84b9c39937441c96077c365acc97afd3875f9cbd58bc70cee61dab9e7d421f0c` |

The current functions implement the HCEA/PDB-authenticated subsystem rather
than raw-offset scaffolding: typed unit/vehicle/tag access, self/friend/enemy
and dropship classification, blip sizing, movement/camouflage/weapon-state
eligibility, sensor-history population, range/yaw transforms, custom-goal
blips, and rasterizer submission.  Target relocation calls independently
confirm those responsibilities.

- `_blip_begin` is the closest body here: 36 of 38 instructions align.  The
  only normalized instruction differences load the player and multiplayer
  arguments from `[ebp+0x10]/[ebp+0xc]` in January versus
  `[ebp+0xc]/[ebp+8]` in the candidate.  This is an unresolved private
  caller/prototype stack contract, not permission to claim 99.92% as exact.
- `_blip_type_get` aligns 110 of 139 target instructions against 152 candidate
  instructions and has the same 21 relocations and complete call multiset.
  Extra owner-player loads and different branches/register lifetimes around
  self and occupied-vehicle classification remain.
- `_motion_sensor_blip_set_type_and_size` aligns 38 of 42 target instructions
  against 43 candidate instructions.  January lowers the invalid-size path as
  `jl` plus an `xor`-materialized zero; the candidate uses the logically
  equivalent opposite branch and an immediate zero store.
- `_motion_sensor_update` aligns only 175 of 360 target instructions against
  357 candidate instructions.  Both history-copy and full object-scan modes
  are present, but January directly calls `_blip_type_get` and the typed
  object/tag primitives where the candidate retains a call to
  `_motion_sensor_blip_set_type_and_size` and performs additional player-data
  lookups.  Its `0x270` frame also differs from January's `0x284` frame.  This
  is a substantive inlining/call-boundary and allocation mismatch.
- `_render_blip` aligns 80 of 141 target instructions against 133 candidate
  instructions.  The render call topology is present, but the literal pool is
  not identical: January owns `__real@3fe6666666666666` and
  `__real@3dd6774d`, while the candidate owns
  `__real@3fe6666660000000` and `__real@3dd67750`; January also relocates a
  pooled `1.0f` that the candidate materializes differently.  x87 expression
  order and locals remain unsettled.
- `_render_motion_sensor` aligns 193 of 235 target instructions against 236
  candidate instructions.  Padded size, all 40 relocations, history/custom
  loops, and render calls agree at the semantic level.  The remaining delta
  begins at the `_blip_begin` private-call contract and continues through
  register/spill and loop scheduling.
- `_should_draw_object` aligns 64 of 98 target instructions against 85
  candidate instructions.  It preserves the five target calls and the
  weapon/grenade, camouflage, velocity, and debug-override predicates, but
  January's object-access and boolean-materialization schedule is longer.
- `_update_motion_sensor` aligns 119 of 285 target instructions against 247
  candidate instructions.  The candidate is 64 padded bytes shorter, has a
  `0x2c4` rather than `0x2c0` frame, lacks two January zero-real relocations,
  and emits a different assertion-expression literal.  The camera, object,
  custom-position, compaction, yaw, and stack-sentinel semantics are present;
  x87/local scheduling and diagnostic spelling are not exact.

The 2026-09-09 house-rule audit removed two decompiler-style
`0.0f * 0.0f` products from `_update_motion_sensor`.  Both horizontal range
tests now use `magnitude_squared2d((real_vector2d const *)&displacement)`, the
typed helper already used by `_render_blip` in this translation unit.  The
January owner still has two pooled-zero relocations and a different live x87
schedule; manufacturing inert arithmetic to reproduce them would be fake
matching.  A fresh scratch gate retained all nine inherited exact functions,
kept the candidate at 800 padded bytes / 37 relocations, passed the forbidden
`_point_from_line3d` guard, and produced the candidate hash recorded above.
The unchanged 74.21111% objdiff result remains diagnostic and earns no exact
credit.

The same audit deliberately retained the standalone `game_engine_running()`
call in `_motion_sensor_update` and the standalone `game_time_get()` call in
`_update_motion_sensor`.  January emits each call, and HCEA's analysis of a
later symbol-build PDB assigns the calls their own source lines in
`motion_sensor_update`/`update_motion_sensor`; `game_time_get` also performs
initialized-state diagnostics.  Thus these are authenticated historical
source calls, not synthetic dependency pressure.  The 128-dword `0x62`
stack buffers, captured return EIPs, reverse scans, and diagnostic exits were
also retained: January emits the complete sentinel sequence, and the same
pattern is independently documented in the accepted HUD stack-sentinel
family.  Replacing it with an invented helper or deleting the source-authentic
calls would be less faithful.

Names, ordering, and signatures are authenticated by
`docs/object_matching_logs/motion_sensor_obj_jonas_census_tick_one_shot_20260829.md`;
the conclusions above come from the current bodies and current COFFs, not from
that earlier no-body intake.

## `source/items/projectiles.obj`

| Function | Meaningful | Target / candidate padded | Target / candidate relocs | Objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `_projectile_aim_ballistic` | 875 | 880 / 880 | 35 / 36 | 86.64310% | `fbc7e95f65b75fdae9a94e16b800e681c145c3449dcb9f70dcf87efb9eba370d` | `6e936a41aee4d8d56ae60e217127e8b8dff81b1e7229e2e2b2f70ffa868a39e6` |

The retained ordinary C body is a complete ballistic solve: it derives
gravity and the minimum-energy time, honors forced velocity and requested
ballistic fraction, selects the lob/non-lob quadratic root, falls back to the
minimum-energy solution, normalizes the aim vector, and fills each optional
result.  The target and candidate have the same 302-instruction count and the
same assertion/normalization call multiset.

Only 165 target instructions align, however.  January uses a `0x34` frame and
a byte result local; the candidate uses a `0x30` frame and keeps that result in
`BL`.  The magnitude/quadratic x87 expression tree, temporary ownership, and
branch schedule differ throughout, and the candidate carries one extra
`__real@3f800000` relocation.  Equal padded size is therefore not a closure.
The earlier runtime-cache ledger explicitly left this owner unwritten;
`docs/object_matching_logs/projectiles_obj_runtime_cache_callbacks_20260909.md`
remains useful provenance, not evidence for this newer body.

## `source/structures/structures.obj`

| Function | Meaningful | Target / candidate padded | Target / candidate relocs | Objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `_render_debug_fog_planes` | 350 | 352 / 368 | 20 / 20 | 70.92500% | `2aa37cc3d43bb69853a07c8693e3c4ef83389a26910a64ae4619010e09d52ea8` | `a8adaa33cd572e879ba65d3b5c042368b8d5d29e90739dc418b65cadb1652474` |
| `_structure_cluster_mark` | 127 | 128 / 128 | 12 / 12 | 77.94872% | `c426fbafcf634a63373bab944083c26aaebda5a181121a993ed0e2c0e644921a` | `6a2ecd6bf009e8daca71c82a97e74f82e02a4c69336e61acabbfffff357af60c` |
| `_structure_cluster_unmarked` | 115 | 128 / 128 | 11 / 11 | 90.58823% | `98b5502fb6c8cb58f48c07e07ae161eb7434b246a299c669a8eb61d33efbed6a` | `3b867b30927311d97aed7094ec07a3b176faded2b3bd27c5280dc4e6f734ea4a` |
| `_structure_clusters_in_cone` | 476 | 480 / 480 | 26 / 25 | 84.54967% | `7dace3a28da0af6b4ba088102aea9d2cac40a2f8c43ae0a86d7ee0d4fe8b0735` | `771335c67292ccfe8f04ecaa7bb5b27615bfd70b652087e19fc7589004a557d9` |
| `_structure_get_planar_fog` | 442 | 448 / 416 | 15 / 14 | 38.43919% | `a5aa0f690e82204d70cb108e143047be76871fd5cf1a979119ce80e8163c097b` | `7a670604363eebcb3d91e0a6a9a2ba90aa5983e1c5e9e062407b4f78f7ff07bf` |

Current source uses named structure, cluster, portal, fog, render, color, and
tag-block types.  The target call graph confirms the same debug-draw,
marker/portal traversal, fog lookup, and fog-offset responsibilities.

- `_render_debug_fog_planes` walks the current cluster's fog polygon, builds
  the plane-normal offset pair, and submits the two solid and two shaded edge
  calls found in January.  It aligns 56 of 122 target instructions against 135
  candidate instructions; the `0x20` versus `0x28` frame, x87 point
  construction, registers, and loop lowering remain different.
- `_structure_cluster_mark` aligns 31 of 40 instructions.  The current natural
  equal-first body returns false when already stamped, then stores and returns
  true.  January branches to the false return after the compare; candidate
  control flow emits an early return on equality in a different layout.
- `_structure_cluster_unmarked` aligns 37 of 47 target instructions against
  45 candidate instructions.  January returns a direct `setne al`; the
  candidate materializes the same comparison through additional registers.
  Both marker helpers also retain fail-closed relocation-ownership differences
  for the target-owned cluster-marker BSS and one pooled assertion literal.
  The previously measured cast/opposite-branch forms that happened to close
  are rejected in
  `docs/object_matching_logs/structures_obj_fable_cluster_pair_intake_20260906.md`;
  they are not used here.
- `_structure_clusters_in_cone` performs the complete bounded depth-first
  traversal: marker begin/end, portal adjacency, sphere/cone rejection,
  duplicate suppression, bounded stack assertion, and result collection.  It
  aligns 112 of 155 target instructions against 152 candidate instructions
  and has the same padded size and call multiset.  Register/control scheduling
  differs and the candidate lacks one January relocation to the current
  cluster marker.
- `_structure_get_planar_fog` initializes fog off, falls back from cluster fog
  to sky screen fog, distinguishes planar and fully-fogged modes, copies the
  fog definition fields, and submits the offset vector.  It aligns only 89 of
  154 target instructions against 147 candidate instructions, is 32 padded
  bytes short, and lacks one January tag-block lookup.  January directly proves
  the disabled animation operation (`fld [definition+4]` then
  `fmul __real@00000000`).  The independent Halo CE PC reconstruction records
  the same literal-zero multiply, and the HCEA reconstruction preserves the
  same operation; Marathon has no corresponding planar-fog animation source.
  January's stripped `structures.obj` PDB module contains no local-variable
  records, so no original phase-variable or macro name can be recovered.
  Replacing the zero with a guessed phase, or inventing a name for one, would
  therefore be less honest than retaining the proven operation.  The source
  now labels it under the project's original-bug policy: planar-fog motion is
  disabled in this build, and a corrected build would require an authenticated
  time-varying phase.  The remaining tag lookup, branch order, registers, and
  x87 stack schedule are still unresolved.

The literal-zero provenance was checked against these independent artifacts:

- HCEA reconstruction
  `_research_cache/halocea/src/blam/structures/structure_get_planar_fog.c`
  (`ca080e373bd4e82f4eac3739f4d3ab90dc64ff0f5e1d33adde48c737ffdaa70d`)
  uses `definition->animation_distance * 0.0f`.
- Stian's PC reconstruction
  `_research-stian-halo/src/halo/structures/structures.c`
  (`cfde0054edd72ab1766666f5907bafee73f0b812e68c9714a526d2eef27ef80e`)
  independently records the same `* 0.0f` operation in `FUN_00198f10`.
- January's stripped `cachebeta.pdb`
  (`8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`)
  identifies the `structures.obj` module but exposes no local-variable record
  for this function.  The available `_symbols.exe` files likewise refer to
  external PDBs rather than embedding the missing local names.

The definition-index and fog-offset dependency evidence remains in
`docs/object_matching_logs/structures_obj_jonas_planar_fog_definition_index_boundary_20260829.md`
and
`docs/object_matching_logs/structure_render_obj_jonas_fog_offset_leaf_20260821.md`.

## Admission boundary and reproducibility

These bodies are readable typed reconstructions and are worth retaining as
future research baselines.  This audit found no raw address-named private
function, manual byte emission, volatile/register steering, forced inline,
synthetic anchor, representation pun, or invented branch used to manufacture
a match in the listed bodies.  It also found no emitted
`_point_from_line3d`.  Similarity, matching call sets, and even equal padded
sizes remain supporting evidence only; the differing normalized hashes and
the mismatches above keep every owner fail-closed.

Frozen raw COFF identities for this measurement:

| Unit | January split SHA-256 | Current candidate SHA-256 |
| --- | --- | --- |
| `source/interface/hud_draw` | `c64ed0a8c0adc1c618aa5ef7e4f243a814c1010c4fb02f6a91aad0278a1dd891` | `053e344fd2b51c31a0cb94868596ee352103ba2500721d55f91e61c92c6cee18` |
| `source/interface/motion_sensor` | `d46d27f29b9d5249c4c12e33e7fc63f6d21e06d6879b54afc571015947695adb` | `90be27a230d64513c3fb6cedc8f1d56d254cf1513e045564bf3627a33409f501` |
| `source/items/projectiles` | `e4d5697dcce32e90b71e240b27d55363c85eca2bb0fb15e5074daa0c7b609c6c` | `3099c6b68dcb0948fe524cb2b93d12656c5d7d0141317e1580ce25d3cc28b28b` |
| `source/structures/structures` | `0e0875524b3580c272bb51b1d5630540d58ce9e1e1ae6c3ef52ca5f30669a4d7` | `7a73c3efc00bd283f4872609c3fa929d6d446ea18920607c3fc13eef5fe9e4a2` |

The only source change made by this follow-up audit is the codegen-neutral
original-bug comment beside the proven literal-zero expression in
`source/structures/structures.c`.  The matching `config/parked.json` evidence
was expanded without changing its measurements or awarding credit.
