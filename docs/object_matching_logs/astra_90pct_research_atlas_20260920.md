# Astra January exact-match research atlas — 2026-09-20

Verified cumulative gain: **+8,314 meaningful / +8,352 padded bytes / +6 accepted Halo functions**, with zero inherited regressions. Halo is **1,439,906 / 1,770,166 = 81.34299%**; **153,244 meaningful bytes remain** to the 90% threshold. Wave2 contributes 720 meaningful/padded bytes and one exact function. Three reviewed AI corrections span 34,010 target meaningful bytes at zero exact credit. The sections labeled wave1 below remain historical evidence; the wave2 section and schema3 latest fields carry current state.

## Baseline and isolation

Verified remote `refs/heads/jonas/exact-pilots` was exactly `47d2e7b69f1c1277123e979296b6f0a7de00c80b`. The similarly named remote `exact-pilots` is a different branch and was not used as the frozen baseline. Setup initially selected that name, corrected it before baseline/source work, and preserved its setup-only log separately.

Working branch: `astra/halo-90pct-breakthrough-20260920` in `C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\work\astra-90pct-breakthrough-20260920`. Its Git database is an independent local bare clone at the sibling `halo-campaign.git`; the dirty canonical checkout and its Git database were not modified. No push was performed. The toolchain, image and build tools were copied into this worktree, not linked to mutable canonical outputs.

January image SHA256: `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

Stage0 completed the full source build, semantic progress, strict snapshot, park validation, admission audit, tests, fake scan and owner census before production edits. Baseline: 7,281 Halo accepted functions / 1,431,592 meaningful bytes; overall 7,548 / 1,452,824. Parks187/0/0; admission candidates19/contradicted0/rejected8/revoked0; tests1,152 passed/5 skipped/26 subtests; fake findings22. Evidence is `scratch/astra-90pct-*-before.*`, with the stable snapshot specifically `scratch/astra-90pct-before.json`. The explicit `all_source` baseline snapshot had zero differences.

## Accounting corrections

The canonical 293 residual report rows /338,574 meaningful bytes are retained. They comprise 289 actual function-section owners and four embedded report labels, totaling436 meaningful bytes. Seven function sections really lack bodies (3,733 meaningful/3,784 padded); zero objdiff is not a reliable missing-body test. In particular, projectile_collision_test_line has a reconstructed496-byte candidate section despite a zero score (498 is its target meaningful size). The baseline section-owner split is112 written/unparked,169 parked,7 missing,1 veto; the four embedded records are separate.

The baseline >=95% pool remains142,077 meaningful bytes. The historical5,856-byte alias opportunity is already accepted and cannot be gained again. This wave's one-wide profile admission is a new source correction whose remaining symbolic alias is independently resolved, not reclaimed old credit.

Physical padded totals deduplicate accepted offset-zero `(unit, section)` owners. An embedded label does not add its parent's padding a second time. Baseline Halo1,489,642 and overall1,511,627 padded bytes are independently confirmed by direct accepted-ledger iteration. Five multiply-named offset-zero libcmt sections account for398 bytes that a named-function iterator can omit; the broad “14 embedded” diagnostic was corrected to nine actual embedded accepted entries. Wave1 padded totals: Halo1,497,274; overall1,519,259. Current padded totals: Halo1,497,994; overall1,519,979. Canonical meaningful-byte/function totals were not redefined.

Wave1 unaccepted pool (frozen; current pool below):

| Class | Report rows | Meaningful bytes | Physical padded bytes |
|---|---:|---:|---:|
| written-unparked | 108 | 181,364 | 182,560 |
| parked | 168 | 145,223 | 146,528 |
| unwritten | 7 | 3,733 | 3,784 |
| embedded-record | 4 | 436 | 0 |
| semantic-veto | 1 | 224 | 224 |

## Evidence hierarchy and tools

January split code, relocations and fresh PDB image identities are final authority. All467 Halo compiland streams in the supplied January PDB contain only S_OBJNAME and S_COMPILE2_ST; no procedure/local/lexical-block/line/source-file records exist there. All19 priority compilands report C, Pentium Pro, FE/BE13.0.9254 and flags0x200 NoDbgInfo. These do not prove the full optimizer command line, `/Ob`, `/QIfist`, QFE, PCH or original header order. Current configuration options are recorded as reconstruction settings only.

The supplied halo_cache_symbols.exe is a2020/v140 cross-build with an external, unavailable RSDS PDB and no embedded COFF symbol table. **Same source revision remains unverified**; a different compiler/date alone does not disprove it. Source-assertion paths, distinctive control flow and exact call graphs support individual correspondences. Its RTC records are protected locals, not complete scalar declaration order or literal lexical braces. HCEA2011 PDB types/lines corroborate semantics; PPC register spellings rendered by DIA must not be mistaken for January x86 allocation.

Fresh read-only Ghidra analysis recovered2,517 call-linked frame descriptors and6,350 locals; an independent PE decoder recovered2,528 descriptor records, with all2,517 overlapping frames agreeing exactly. Six priority AI functions were decompiled and disassembled. HCEA supplied typed locals and line records for five of the six; ai_debug_render_actor was absent by that name. Five additional physics/rendering correspondences have protected aggregate-local evidence. Full raw results and reproducing scripts are under `scratch/astra-debug-evidence/` and `scratch/astra-context-*`.

**IDA_AUTOMATION_UNAVAILABLE.** The user independently verified the only installation is IDA Free9.4 without a callable endpoint and explicitly authorized Ghidra+DIA with optional manual GUI corroboration. No successful IDA analysis is claimed. No admission needs IDA-only evidence. The failed fresh IDC/OSGi/PyGhidra attempts are recorded; the working Ghidra Java read-only launcher is documented in the debug packet.

## Wave1 admitted mechanisms

| Function | New meaningful | New padded | Relocations |
|---|---:|---:|---:|
| _action_alert_next_position | 770 | 784 | 18 |
| _action_obey_command_begin | 4,320 | 4,320 | 167 |
| _action_vehicle_desirable | 272 | 272 | 7 |
| _ai_communication_find_actor_to_reply_to_player | 1,090 | 1,104 | 65 |
| _player_profile_1wide_list_update | 1,142 | 1,152 | 70 |

1. **Narrow private communication ABI and natural reply graph.** January's word argument preparations, existing short animation fields/units.h owner, and three fresh HCEA named `animation_type: short` parameters authenticate narrowing the three private forwarders. No public/header change. Both already-exact private finder controls and all43 inherited exact siblings survive. The reply body is ordinary initialized, typed C preserving call/random/filter ordering. Finished/event remain fuzzy with zero credit.
2. **Source-owned branch stores and lifetimes.** The unoptimized cross-build explicitly repeats Alert's NONE result stores and Obey's duration store, demonstrates initialization order and jump-delay-before-pause order, and supports the shared result graph. January's overlapping address-taken iterator/head-position slots, HCEA distinct typed iterators and a meaningful prop-search boundary support the Obey scope. No lexical Block record was recovered; brace placement is an evidence-backed inference. One-wide's repeated description terminator, ordered real description locals and recursive retry are independently visible in the cross-build. Vehicle's entry FALSE initializer and repeated dead-branch FALSE are directly visible. These statements are preserved because of independent evidence, not because duplicate assignments are generally authorized.
3. **Resolved one-wide alias.** Normalized code, extent and all70 relocation positions/types/destinations agree. At+0x1A9, `_cached_player_profile+156` and `_local_player_index_for_draw_string_and_hack_in_icons+0` are the same PDB-proven RVA5,841,532; raw January operand10,035,836 agrees. The existing accepted-ledger pipeline admits the function without a policy/symbol/comparator change. The symbolic stable gate still reports it residual, so the two counters are disclosed separately.

Four unrelated exact branch controls (profile_frame_get_value928, render_ui_widgets560, render_ui_widgets_postgame384, encounter_test_rule656) and all touched-unit siblings support the bounded branch-store rule. Whole-tree checks preserve every inherited exact function. This is not a broad validation of arbitrary duplicated assignments, declaration reordering or vector-copy spelling.

Wave1: Action Alert receives new whole-object Matching status:12 code owners,6 data owners,2,120 meaningful/2,240 padded code and80 aligned data bytes; all target owners/data exact, no extra owner or veto. This object status adds no second function credit. Action Obey retains its existing ownership veto and14 target-absent math owners; its new function can be exact while the object remains incomplete.

## Priority atlas and remaining research

The JSON/CSV preserve all293 frozen residual rows and append `wave1_after` instead of replacing baseline measurements. `target`/`candidate` for embedded labels describe their parent section and are explicitly labeled. The JSON contains normalized hashes, full relocation tables, linkage/storage, target section, current reconstruction flags/include order, source hashes, park/reopen evidence, PDB compiland facts and separate HCEA/RTC context. Unknown January source/type/option fields remain null or explicitly unavailable.

| Packet | Function | Baseline meaningful | Baseline objdiff | After wave |
|---|---|---:|---:|---|
| A | _ai_debug_render_actor | 24,576 | 95.02% | written-unparked |
| A | _ai_communication_event | 8,064 | 84.64% | written-unparked |
| A | _actor_perception_update | 4,720 | 96.47% | written-unparked |
| A | _actor_look_update | 4,714 | 92.66% | written-unparked |
| A | _action_obey_command_begin | 4,320 | 95.61% | accepted |
| A | _actor_move_vector_avoidance | 4,130 | 74.87% | written-unparked |
| B | _biped_update_physics | 5,366 | 82.69% | written-unparked |
| B | _physics_update_old | 5,163 | 79.11% | written-unparked |
| B | _collision_move_point | 4,744 | 97.78% | written-unparked |
| B | _collision_debug_render | 4,182 | 87.58% | written-unparked |
| B | _biped_update_moving | 4,071 | 81.10% | written-unparked |
| B | _update_alien_scout_physics | 2,456 | 88.43% | written-unparked |
| C | _decal_new_from_collision | 6,162 | 92.72% | written-unparked |
| C | _projectile_update | 4,055 | 98.93% | written-unparked |
| C | _projectile_collision | 2,928 | 86.39% | written-unparked |
| C | _weapon_update | 2,788 | 78.20% | written-unparked |
| C | _render_weapon_hud | 2,645 | 81.55% | written-unparked |
| C | _decal_clip_to_surface | 1,768 | 98.88% | written-unparked |
| D | _rasterizer_frame_statistics_draw | 4,165 | 99.98% | written-unparked |
| D | __rasterizer_environment_lightmap_draw | 4,008 | 91.24% | written-unparked |
| D | __rasterizer_environment_fog_screen_begin | 3,901 | 79.21% | written-unparked |
| D | _render_camera_mirror | 861 | 98.95% | written-unparked |

PacketA: use the recovered94 ai_debug aggregate records and34 communication-event records to resolve specific meaningful scopes/local identities. Avoid copying the later frame offsets or suppressing point_from_line3d ownership. The private short chain gives one bounded type correction, not a general ABI rewrite. Perception's separate sense_position records and Moving's avoidance arrays/ray families are useful next sources of specific type/lifetime evidence.

PacketB: collision_move_point and biped_update_physics have protected aggregate-local families with exact widths; collision_debug has substantial collision/features arrays. The records do not yet recover January's scalar order or close vector-copy scheduling. Existing Sept19 authentic physics/header corrections supersede earlier wrong-helper theories. No physics production change was admitted.

PacketC: fresh PDB contribution census agrees on all22,455 records. point_from_line3d selects action_charge at RVA5504, DBI470, rank2;17 current undefined referrers and six later current emitted copies exist. All seven48-byte bodies agree. Current NODUPLICATES/ANY metadata is not proof of original discarded raw COMDATs. The broad helper census has93 first-copy-consistent cases, three counterexamples (_power/_random/_set_real_quaternion),14 absent names and15 unresolved owners. No universal owner-selection law or raw-COMDAT suppression mechanism is admitted.

PacketD: mirror's `0.0125f` repairs the exact float-rounded-double constant, preserving18 exact siblings, but final x87 keep/consume still differs;861 meaningful bytes remain at zero credit. The larger mirrored-control-flow rewrite was byte-inert and not landed. Frame statistics retains equal4176-byte/322-relocation/1178-instruction shape with register scheduling differences; later RTC statistics192 is not January's shared0x170 owner. The duplicate-layout donor stays prohibited. No new C/SSE/assembly boundary was authenticated for the large rendering functions.

## Wave1 validation and rejected hypotheses

Full source build and semantic progress pass. Stable snapshot gains4 functions/6,480 padded bytes with zero losses; canonical accepted ledger gains5/7,594 meaningful/7,632 padded, also zero losses. Tests1,152 passed,5 skipped,26 subtests passed. Parks186 active/0 stale/0 invalid. Admission19 candidates/0 contradicted/8 rejected/0 revoked. Fake findings22 before/22 after; changed sources0. Point emitted-symbol guards pass; the seven inherited emitters are unchanged. Runtime owner/data/COMMON census is unchanged; communication literal-section physical reorder was independently checked by semantic owner. `git diff --check` passes.

See `astra_90pct_rejected_hypotheses_20260920.md` for explicit do-not-repeat/reopen criteria and the three primary packet reports plus per-landing independent reviews. No universal8/12-byte aggregate-copy law was established. No target/symbol/comparator/admission-policy/header/compiler-flag change was made. No manual bytes, forced inline, fake dependencies, uninitialized donor, duplicate layout, negative unrelated-object indexing or credit exception was introduced.

## Wave2: authenticated extraction and AI corrections

`_extract_sequence` is now strict: **720 meaningful / 720 padded bytes, 28 relocations**, SHA `5f106d5c4f2bb3024df015b0b10b2236b3ffc3818cff7588e4fd3a7c7a810e56`. Fresh tools-build raw instructions authenticate its bottom>=top assertion, initialization, state fallthrough, independent repeated conditions, and copied working rectangle. The earlier cache-only search lacked this function. All 20 exact siblings and target data owners survive. Three wrong assertion literals are replaced by the January identities; this is an authenticated runtime-data correction.

Bitmap extraction is newly Matching as a whole object: 21 functions, 13,002 meaningful / 13,168 padded code bytes and 4,496 canonical aligned data bytes. The six inherited SELECT_ANY floating literals have existing unique strict January providers; no new behavior or ownership exception was introduced. Its satisfied park is retired. This object status supplies no duplicate function credit.

Perception recovers one initialized orphan index and an independently repeated hostility/attachment guard. Looking recovers the real[2]/real* private type, two equality evaluations and source-store order. AI debug restores a fifth marker point and the zero-vector/three-basis graph. January explicitly uses the point's storage before a later string table reuses it; the earlier inference deleting that point was false. These functions remain strict residuals, with 35, 14 and 57 exact sibling controls preserved respectively. Their target extents are 4,720 + 4,714 + 24,576 = 34,010 meaningful bytes at **zero credit**. Equal ai_debug padded extent/relocation count does not establish exact code. Looking's objdiff percentage decreases slightly despite stronger source evidence; percentage is not an admission criterion.

Collision move point received four source-backed probes without a justified landing. Entry copy, count-exhaustion placement and epsilon-first comparison were byte-inert; the indexed-copy/late-initialization variant worsened the result. Its 19 exact siblings survived. The specific preheader/backedge aggregate schedule and NONE lifetime remain unexplained. No fifth spelling was invented.

Actor movement avoidance was also examined without a source shape. Fresh raw calls resolve through a thunk to the same four-argument point-from-line operation, confirming zero initialization plus two scaled adds. Current named array and ray-local types already agree with HCEA/RTC. Enabling the helper body would reproduce a previously measured surplus point owner; manual expansion and pressure variants remain excluded. All six read-only controls pass. The exact owner/header mechanism and subsequent allocation remain unproved. See the separate avoidance packet.

The fresh cross-build file census found 13 supplied PE images, all without COFF symbols. Eight h1_tags images have external v140 RSDS references and no supplied paired PDB/map/rsp; the other five have no debug directory. This does not prove absence of every possible future artifact, but closes those supplied-image leads. Four tools-family images expose the extraction assertion graph. Full hashes and paths are in `scratch/astra-wave2/crossbuild-artifacts.json`; tools xrefs and independently verified instruction spans are retained.

Current unaccepted pool (287 original report rows):

| Class | Report rows | Meaningful bytes | Physical padded bytes |
|---|---:|---:|---:|
| written-unparked | 108 | 181,364 | 182,560 |
| parked | 167 | 144,503 | 145,808 |
| unwritten | 7 | 3,733 | 3,784 |
| embedded-record | 4 | 436 | 0 |
| semantic-veto | 1 | 224 | 224 |

Current full build passes; tests 1,152 passed, 5 skipped, 26 subtests passed. Stable cumulative gains are 5 functions / 7,200 padded bytes with zero losses; the accepted ledger gains 6 / 8,314 meaningful / 8,352 padded because of the unchanged, fully resolved one-wide alias. Wave2 stable gain is one function / 720 padded. Parks 185/0/0; admission 19 candidates/0 contradicted/8 rejected/0 revoked. Fake findings remain 22 globally and zero in the four wave2 source files. All seven inherited point emitters, owners and COMMON definitions remain intact. Only bitmap's corrected assertion literals change runtime data; independent object review covers that delta. Diff check passes.

Schema3 keeps Stage0 and wave1 fields frozen, adding `wave2_after` and `latest_summary`. Main admission manifest preserves wave1 summary and its resolved alias proof, adding only extraction. Four source files and two existing configuration files change in wave2; no header, symbols, target metadata, compiler setting, comparator or admission policy changes. All per-packet independent reviews and rejected probe records accompany this atlas. `IDA_AUTOMATION_UNAVAILABLE` remains explicit; Ghidra and DIA are authorized and working. No broad 8/12-byte copy, frame-layout or optimizer law has been established.

## Wave3: restore biped diagnostics through the authentic owner header

Wave3 adds **zero exact meaningful bytes, zero padded bytes and zero exact functions**. Coverage stays **81.34299%**, cumulative gain **8,314 meaningful / 8,352 padded / six functions**, with **153,244 meaningful bytes remaining**. The restored `_biped_update_physics` diagnostic spans a 5,366-byte target meaningful function at zero credit. Its candidate grows from 5,248/112 to January's 5,376 padded bytes/124 relocations, but normalized SHA still differs. The moving body is unchanged and is not counted as a changed fuzzy function.

January directly authenticates the omitted debug flag/player-owner predicate and stores of pre-collision position, crouch-adjusted input velocity, repeat, length1.0f, width and height after the collision branches merge. The canonical `object_get` macro is already typed and preserves January's all-object mask. The narrower biped-mask proposal was withdrawn. The existing `best_collision_index != NONE` guards remain; the unsafe historical indexing path is not restored.

Six extern declarations now live in collision_debug.h with genuine cseries/real_math dependencies. Their storage remains in linker_common or the existing collision_debug scalar definitions. The three original consumers and two new consumers were enumerated before editing. All five preserve every code/data/COMMON owner and 145 overlapping exact controls under header-only gates. HS replaces six byte-array facades and ten raw address expressions with typed field/scalar addresses, preserving all table bytes and relocations. The independently reviewed biped body preserves all 42 exact siblings, 193 named owners and 111 runtime data sections; it emits no point helper. No header/COMDAT workaround or admission exception was introduced.

The sole moving scratch hypothesis was withdrawn after raw instructions disproved a decompiler copy-direction inference. Its apparent evidence instead reproduces an already exhausted UMG6 shape. No moving source is landed. Future probes need a specific new source/header or scalar-lifetime distinction, not another frame-pressure variant.

Full build and stable/semantic checks pass with zero losses and zero wave3 gains. Tests: 1,152 passed, 5 skipped, 26 subtests passed. Parks185/0/0, admission19/0/8/0, fake22 globally and zero across changed sources/header. All inherited point emitters, runtime data and COMMON owners stay unchanged. Three C files and one owner header change; no configuration, target metadata, symbol, compiler-option, comparator or policy changes in this wave. Earlier two whole-object admissions and two retired parks remain unchanged.

Latest checkpoint files are scratch/astra-wave3/{after,summary,semantic-after,owner-census}.json and associated full-build/test/audit logs. Schema4 keeps Stage0, wave1 and wave2 frozen, adding wave3_after/latest_summary. Per-packet source/header reviews give full reproduction and evidence boundaries. IDA_AUTOMATION_UNAVAILABLE remains recorded, with Ghidra and DIA authorized. No general compiler scheduling law is established and the 90% campaign remains unfinished.

## Research packet4: zero exact gain, no production changes

**Packet4 is research-only.** Its communication, physics, decal and frame-statistics investigations produced zero new exact bytes or functions. The source candidates are not landed. Root restored the temporary allegiance-header declaration and verified source/config equality with production checkpoint7a6ae71e. The latest full production validation remains wave3; it is not relabeled as a new validation run.

Wave2 gained720 meaningful bytes; wave3 gained0; packet4 gained0. Cumulative coverage remains81.34299%, +8,314 meaningful bytes/six functions from the original baseline, with153,244 bytes still needed for90%. Improved source fidelity, frame agreement, fewer alignment differences, and preserved controls are not exact coverage progress.

The current investigations recovered two communication diagnostics/array types/actor refresh, decal private parameter order/failure-loop lifetime, and physics force-accumulation/snapshot boundaries. Independent gates preserve all inherited controls, but none closes a residual. Frame-statistics RTC statistics[192] is a separate memory-report table, not the shared0x170 counter owner. All candidates and negative results are preserved under scratch/astra-wave4, with disposition RESEARCH_ONLY_NOT_LANDED. The12 temporary owner-header consumers preserve462 exact controls; the proposal remains unapplied.

The campaign now prioritizes a falsifiable shared source/compiler-context mechanism across multiple named residuals, with an affected-byte total and already-exact counterexample controls. No more isolated fidelity edits are promoted as production progress. A source wave must deliver measurable strict gains. If evidence supports no new shared test, state that directly rather than running spelling/flag sweeps. Source revision identity for the supplied debug PE remains unverified; debug-only branch/local differences must not be imported into January merely to create activity.

Packet4 evidence does not modify existing parks, six admissions, two whole-object admissions, compiler options, targets, symbols or policy. Atlas latest_summary and wave3_after remain the current production state; research_packet4 fields describe unlanded candidates. IDA_AUTOMATION_UNAVAILABLE remains recorded.
