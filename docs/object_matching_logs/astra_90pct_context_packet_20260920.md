# Compiler/source context packet, 2026-09-20

## Outcome and admission recommendation

Fresh first-party unoptimized machine code authenticates the previously held branch-owned stores in `action_alert_next_position` and `action_obey_command_begin`. Replaying only the two function bodies against HEAD `47d2e7b69f1c1277123e979296b6f0a7de00c80b` gives **+5,090 meaningful / +5,104 padded bytes / +2 exact functions**. Alert gates 12/12 and Obey 26/27, with all inherited exact functions preserved. Both point-from-line emitted-owner guards pass, both relocation comparisons have zero differences, and the two candidate bodies have zero fake-scan review leads. The orchestrator independently reviewed and began integrating these changes while this report was being written; these are packet contributions, not a separate whole-tree coverage claim.

The independently justified `render_camera_mirror` threshold correction `0.0125` -> `0.0125f` repairs one real constant mismatch and carries **zero exact credit**. The final x87 keep-versus-consume residual remains. No physics/rasterizer source or shared header/config changes were made by this research worker.

## Provenance and limits

Primary debug executable: `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

This is a 2020/v140 cross-build. Its same-source-revision relationship to January is **unverified**. Older ledgers' phrase “same-revision executable” must not be carried forward as established fact. The evidence here is narrower: unique source assertions and complete control-flow/call correspondence identify a function, unoptimized code demonstrates particular source statements, and January bytes/relocations decide the admissible result. It is not a VC7 compiler-settings witness, and its SSE instructions do not authorize SSE or assembly in January.

## Reusable RTC atlas

`scratch/astra-context-rtc.py` mines the PE's frame-descriptor references and validates each count, descriptor range, negative frame offset, positive extent and identifier spelling. It recovered **2,528** descriptor records, preserved in `scratch/astra-context-rtc-all.json`, with the executable hash, function entry, descriptor-reference site, descriptor address, and named local offsets/extents. Entries are associated with the preceding Ghidra function-table entry, so these are candidate function boundaries, not inferred original names. Target functions below are independently anchored by source/assertion xrefs. The debug-evidence worker independently acquired a call-linked Ghidra descriptor census; the differing count reflects decoder coverage, not extra January debug information.

Representative packet-B/D records:

| Function identified from source/assertion topology | Debug entry | Authenticated aggregate-local information |
|---|---:|---|
| collision_move_point | 0x007A52B0 | position, velocity, clipped_position, clipped_velocity; clip_collisions[3] and new_clip_collisions[3], each6 bytes; clip_plane16; clip_line_point/vector and clip_point12 |
| collision_debug_render | 0x007AF800 | vector_result1048; features44040; rotated_points96; collisions704; repeated scoped vectors/features |
| biped_update_physics | 0x008C9B80 | three separate desired_velocity/desired_acceleration families, 8- and12-byte widths, collisions704, original/stick/adjacent/ground planes, tangential_velocity, features44040 |
| render_camera_mirror | 0x008402A0 | plane16, adjusted_plane16, adjusted_normal12, point12 |
| rasterizer_frame_statistics_draw | 0x0082A580 | frame_statistics_string12288, tab_stops12, cursor4, bounds8, three16-byte colors; later-build statistics192 and win_memory_status32 |

The frame-statistics record is not the January shared0x170 statistics owner and is not a type/layout transplant. Scalar declarations are generally absent from RTC descriptors; offsets/sizes do not by themselves recover full types or lexical blocks. All five disassemblies are retained under `scratch/astra-context-*-2020.disasm.txt`.

## Branch-owned stores: new decisive evidence

### action_alert_next_position

Source/assertion xrefs identify debug entry0x0042E650; the function retains January's `!actor->meta.swarm` assertion at line275. Fresh unoptimized code independently shows:

- 0x0042E67C / 0x0042E67F: initialize the short result at `[ebp-8]` to NONE.
- 0x0042E6A1 / 0x0042E6A6: assign NONE again in the passenger/disabled-order arm, then jump to the shared exit.
- 0x0042E910 / 0x0042E913: assign NONE in the no-position-available arm, then jump to the same exit.
- repeat, random and traversal paths also assign the same result and join at0x0042EA75.

These are real source-owned branch assignments, not a declaration lottery. The prior candidate's semantic algorithm is preserved. The old early-return body had removed the shared result topology and branch stores. The archive `aa_ec.c` is used only as a source patch carrier after independent authentication; its earlier exact verdict is not evidence on its own.

Current gate: **784 padded / 770 meaningful /18 relocations**, normalized SHA-256 `677fd6cc1a9e66de32123c4614de5a0cc1188501baee69ea20fd5bf74eac3fe4`. All11 exact siblings remain exact. The park's source-context reopen criterion is met by the combined new debug and January evidence, subject to orchestrator ledger update.

### action_obey_command_begin

Debug entry0x00438120 is identified by `ai/action_obey.c` assertions and full command dispatch/call topology. Four changes from the inherited fuzzy source are supported independently:

1. The three NONE indices are initialized at0x004383B7..0x004383C5, **before** loading duration from command+4 at0x004383CC..0x004383D4. This authenticates the declaration-initialization order.
2. The valid `_ai_atom_look` arm reloads the same command+4 and writes the same duration slot again at0x00438409..0x00438411. Invalid look edges and player/object branches retain the dominating duration initializer. This is defined behavior and directly refutes the former “must omit redundant source store” hold.
3. January's prop iterator address is `[ebp-0x1c]` at function+0x2CF, passed to external prop_iterator_new/next; its8-byte storage overlaps the12-byte head_position at `[ebp-0x20]`, passed to unit_get_head_position at+0x37D. Address-taken local storage passed to opaque external calls cannot simply be reused while its source lifetime remains active. This independently supports a prop-search block ending before the player fallback. HCEA PDB additionally records separate **prop_iterator iterator** and **data_iterator iterator**, both at frame0x50, corroborating distinct lifetimes and original local name. The PDB dump has no explicit Block records; literal brace placement is reconstructed from this lifetime evidence, not claimed as a recovered text file.
4. Debug0x00438F48 stores jump delay to simple_control+8; only then0x00438F54 stores pause_timer60 to simple_control+2. The same order is in January.

The no-extra-scope negative control grows the function from4320 to4336 while all25 inherited exact siblings remain exact. Thus the scope is material, with independent lifetime evidence for it; it is not inert declaration filler. The HCEA reverse-C file is explicitly a reconstruction and does not supply independent lexical-brace proof.

Current gate: **4320 meaningful/padded /167 relocations**, normalized SHA-256 begins `5b56fdd8b88a4456`; full metadata in `scratch/astra-context-candidate-review.json`. All25 exact siblings remain exact; action_obey_control remains zero-credit fuzzy. Both typed iterators' authentic name is `iterator`; the orchestrator can retain that spelling when applying the scoped body and confirm byte neutrality.

## Independent exact controls and bounded source rule

Four unrelated current exact controls were checked directly against January through the strict comparator: profile_frame_get_value928, render_ui_widgets560, render_ui_widgets_postgame384, encounter_test_rule656. Their source/debug provenance is recorded in the Sept19 profile, UI and ten-large-objects ledgers. All four remain strict exact in the current full build. Together with the two new functions, they support this bounded rule: **source-authenticated branch-owned assignments must be preserved even if a value-equivalence simplification would remove them.** They do not prove that arbitrary duplicated stores are valid, or that every VC7 entry initializer can be reverse-inferred without source evidence.

This rule is supported by multiple exact functions and primary records, unlike the old unsupported blanket ban on duplicate stores. User authorization explicitly permits preserving independently authenticated original behavior. No new UB, fake dependency, local facade, forced option, inline assembly or behavioral helper is introduced.

## Rendering experiment and rejected hypothesis

The current mirror threshold relocates to double `3f8999999999999a`, whereas January uses float-rounded-double `3f899999a0000000`. `0.0125f` reproduces January's constant and preserves all18 exact siblings. A second measured shape used the debug-authenticated unconditional cosine/sqrt declarations and ternary refraction offset; it compiled to the same corrected function and did not alter the final z-row residual. Do not land the byte-inert rewrite merely to reflect a later build. The minimal threshold correction is the recommendation.

Fresh alignment-aware frame-statistics diff confirms equal4176-byte envelope/322 relocations/1178 instructions but different physical-register choices in opening independent sums at+0x6E/+0x7E/+0x8D and+0x10F/+0x117/+0x197. Old donor layouts remain inadmissible. RTC later-build scalar sums do not authenticate January's shared header order, so no layout/declaration probe was made.

## Aggregate-copy conclusions and do-not-repeat

No new universal8/12-byte aggregate scheduling law was established. Existing exact helper owners, genuine vector assignments, and the current residual family show that load/store interleave is context dependent; archive experiments already cover scalar/aggregate/const/by-value/helper/scope permutations. Do not repeat those without an additional independent source distinction. Particularly:

- action_uncover_perform: load-all/store-all window already exhaustively studied; January PDB module has no local records.
- actor_aim_projectile:12-byte debug-vector copy is an allocation/interleave tie; pointer aliases alone are not evidence.
- action_obey_control: 8-byte pair's ECX/EDX permutation is downstream allocation, not proof of reversed member statement order.
- physics_update_old: Sept14 wrong axes2/rotate shared-helper conclusions predate the Sept19 authenticated header packet. They must not be reapplied blindly.
- collision_move_point and mirror: prior helper/declaration families stay exhausted; current debug records provide semantic/local corroboration without closing allocation differences.

Five bounded probe shapes were measured: mirror threshold, mirror RTC topology, alert authenticated body, obey authenticated body, and obey scope ablation. No production source/header/config was edited by this worker, no Ninja/commit/push was run, and no park was changed by this worker.

## Reproduction files

- `scratch/astra-context-{alert,obey}-authenticated.json`: exact find/replace body patches.
- Corresponding `.obj`, `.gate.txt`, `.diff`, `.reloc.txt`, and `*-authenticated-body.c` files.
- `scratch/astra-context-{alert,obey}-january.disasm.txt`: relocation-normalized January instruction streams.
- `scratch/astra-context-action_{alert_next_position,obey_command_begin}-2020.disasm.txt`: primary unoptimized evidence.
- `scratch/astra-context-obey-no-extra-scope.{json,obj,gate.txt}`: negative control.
- `scratch/astra-context-mirror-{threshold,rtc-topology}.{json,obj,aln.txt}`.
- `scratch/astra-context-candidate-review.json`: current integrated-baseline owner census, strict candidate metadata and exact controls. This census ran after the orchestrator rebuilt the integrated branch, so zero changed owners is **post-integration agreement**, not a frozen-before owner audit.
- `scratch/astra-context-candidates-fake-scan.txt`: zero review leads.
- HCEA raw evidence is owned by the debug worker under `scratch/astra-debug-evidence/hcea_action_obey_command_begin_{sym,lines}.txt`.


## Authorized follow-up: one-wide player-profile branch topology

After the five bounded shapes above, the orchestrator authorized one additional high-value branch candidate. The remaining identified branch-owned pool after Alert/Obey is `player_profile_1wide_list_update` (1,142 meaningful / 1,152 padded), `extract_sequence` (720 / 720), and `action_vehicle_desirable` (272 / 272), total **2,134 meaningful / 2,144 padded**. The old 7,344-byte estimate is not used as a current inventory; already accepted Profile/UI/Encounters controls are excluded.

The first of those now has fresh primary authentication. Unique qtr-screen-profile assertion literals identify the debug function at **0x00686170**, independently of shifted cross-build line numbers. The debug body explicitly stores the default-description terminator at 0x006865F7, jumps through 0x00686611 to the shared tail 0x006866AE, and writes the same `text[255] = 0` again at 0x006866C1. It also evaluates the joystick-description lookup before the button-description lookup and stores their results in real local slots before formatting. The retry at 0x0068674E directly calls 0x00686170 after filtering/clamping the selection. All these are ordinary source topology preserved by the unoptimized build; they are not inferred solely from an exact candidate.

A single function-only replay from the archived `p1_final.c` against current headers gives **1,152 / 1,152 bytes, 70 / 70 relocations, equal normalized code SHA**, and preserves all 40 strict-exact siblings. The only unresolved strict row is relocation +0x1A9:

- January: `_local_player_index_for_draw_string_and_hack_in_icons + 0`;
- candidate: `_cached_player_profile + 156` (one past three 52-byte cache records).

The alias worker independently validated the image-address identity: January PDB publishes the first symbol at RVA 5841532 and cached_player_profile at RVA 5841376; +156 is exactly 5841532. Raw January operands agree. Existing accepted controls are the cache updater and three-wide profile function, through the unchanged objdiff/objdiff-coff-shape admission path. Full alias evidence is `scratch/astra-alias-census.json`. **No credit is assigned by this worker: the orchestrator must run the normal report and verify admission for the new one-wide function.** Unlike the old lane, the current replay has no additional empty-wide-string/CRLF relocation mismatch. The point-from-line guard passes; fake scan finds zero leads.

Artifacts: `scratch/astra-context-onewide-authenticated.{json,obj,gate.txt,reloc.txt}`, the function body source, unique assertion xrefs, and `scratch/astra-context-player_profile_1wide_list_update-2020.disasm.txt`. No further variants were measured.

A final raw MSF/CodeView inspection by the debug worker confirms HCEA ActionObey has **no lexical Block records**. Its typed iterator/offset reuse remains corroboration of disjoint lifetimes, not literal original brace text. The January address-taken overlap and current ablation remain the independent scope evidence.


## Authorized second follow-up: vehicle desirable branch assignments

The orchestrator authorized one further bounded investigation of `extract_sequence` or `action_vehicle_desirable`. Vehicle had direct fresh evidence, so only **one compiler shape** was measured. `IDA_AUTOMATION_UNAVAILABLE`: no IDA automation was used; the user authorized proceeding with available primary binary evidence. The read-only PE/Capstone/RTC route remained sufficient.

The same authenticated cache PE contains an unoptimized function at **0x004417E0** with RTC descriptor0x00441940 and sole protected aggregate `origin`, size12 at EBP-0x20. Its identity is reconstructed from the distinctive seven-argument actor/vehicle/scripted/distance/inside/attempting topology and the neighboring action_vehicle functions; the RTC name alone is not treated as a function-name symbol. The body exactly follows the current semantic routine: fetch actor and unit type3, test dead bit4, test scripted, choose one of two maximum distances, fetch the vehicle origin, test squared distance unless already inside, veto excess translational speed unless already inside/attempting, then apply the upright0.5 test.

**Independent store evidence from that unoptimized body:**

- 0x00441827 stores zero to byte EBP-0x0D before inspecting `vehicle.damage_flags & 4` at0x0044182B..0x00441838.
- The dead branch explicitly stores zero to that same byte again at **0x0044183A**, then jumps to the common upright test0x004418F5.
- The scripted branch stores one at0x0044184B and joins the same test.
- The ordinary branch loads its maximum distance into EBP-0x28 at0x00441872..0x00441877 and immediately prepares `object_get_origin` at0x0044187C. It has **no separate FALSE store** before that call.
- The out-of-range edge at0x004418BB reaches0x004418F5 using the entry initialization. A qualifying range stores one at0x004418BD. The velocity veto0x004418F1 and upright veto0x0044190E each explicitly restore zero.

This independently closes the September14 admission objection. The optimized January object by itself shows entry `xor bl,bl` at+0x2B and the branch chain; the unoptimized body supplies the previously missing direct evidence of the redundant dead-arm assignment. The debug executable is a2020/v140 build; same-source-revision is unverified. Here the full narrow topology corresponds to January and current semantics, rather than making a build-wide source-equivalence assumption.

The minimal patch changes `boolean desirable;` to `boolean desirable = FALSE;` and removes the else-arm `desirable = FALSE;` immediately before `object_get_origin`. It preserves the existing dead-arm assignment and all other statements. The independently reconstructed body equals archived `vd.c`, but the archive's earlier exact verdict was not used as source authentication.

Fresh current-TU result: **15 strict exact /2 residual**, versus14 exact/3 residual. `_action_vehicle_desirable` is **272 meaningful /272 padded /7 relocations**, normalized SHA-256 `e3b1ecf64be4fb82e7bad92a4bce38d449f57b5b5acba6da82576f649dee1219`. All seven relocation rows agree; no alias admission is needed. All14 previous exact functions are byte-and-relocation unchanged. Owner census is50/50 with no added or dropped owners and no changed noncode. The only other changed owner is existing residual `_action_vehicle_setup_impromptu`, which inlines this routine. It remains464 bytes with176 instructions, now differing from January in the already known x87 squared-coordinate ordering (plus tool-rendered defined-vs-undefined identity notation); no exact function regresses. The `_point_from_line3d` guard passes and the source fake scan reports zero leads.

The four independent branch controls were checked afresh: `_profile_frame_get_value`, `_render_ui_widgets`, `_render_ui_widgets_postgame`, and `_encounter_test_rule` are all strict exact. This follow-up uses the same bounded source-authenticated branch-store rule, with no header, options, ABI or declaration-layout changes.

Landing recommendation: apply `scratch/astra-context-vehicle-authenticated.json` to the current source, then use the orchestrator's normal report and full verification. This worker changed no production source, header, config, park or shared build state, and ran no Ninja/commit/push.

Artifacts: `scratch/astra-context-vehicle-authenticated.{json,obj,diff,gate.txt,reloc.txt}`, `*-body.c`, `*-fake-scan.txt`, `scratch/astra-context-vehicle-review.json`, `scratch/astra-context-vehicle-exact-siblings.json`, `scratch/astra-context-action_vehicle_desirable-2020.disasm.txt`, `scratch/astra-context-vehicle-january.disasm.txt`, and the three setup-impromptu disassemblies plus candidate alignment diff.

`extract_sequence` was not probed in this follow-up. The cache executable's bitmap_extract source xrefs identify only two validation routines and do not authenticate the extraction state machine. Its archived exact shape remains blocked on direct evidence for `found_bottom_reference`/`single_sequence` condition topology, bounds copies and initialization order. A tools-executable source/function record is the appropriate next evidence route; replaying the old720-byte exact candidate without that evidence is not recommended.
