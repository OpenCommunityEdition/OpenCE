# Rejected hypotheses and bounded reopening criteria

Research disposition for the frozen `47d2e7b69f1c1277123e979296b6f0a7de00c80b`
baseline and this campaign's reviewed candidates. Counts below identify fresh
measurements versus historical controls. They are evidence, not extra coverage.

1. **The historical 5,856-byte alias pool is remaining gain — rejected.**
   Fresh scan: 7,256 ordinary strict controls, zero read errors, 17 pure identity
   rows. All 15 fully resolved rows are already accepted: 5,856 meaningful /
   5,952 padded bytes; all 17 differing operands agree with the January image.
   The other two rows are accepted-but-globally-ambiguous `render_debug_decals`
   and the existing 224-byte `render_debug_obstacle_path` semantic veto.
   Reopen only for a genuinely new unaccepted body with independently resolved
   destinations and source ownership. The newly reconstructed one-wide profile
   body is such a separate candidate: all 70 destinations agree, all 40 strict
   siblings survive. Its one alias is corroborated by the January PDB and raw
   operand; it does not reopen or recount the old pool. Evidence:
   `scratch/astra-alias-census.json`, `scratch/astra-alias-onewide-review.json`,
   [alias packet](astra_90pct_alias_packet_20260920.md).

2. **Current helper copies prove a universal first-copy COMDAT law — rejected.**
   Fresh 125-helper census: 93 compatible, three counterexamples (`_power`,
   `_random`, `_set_real_quaternion`), 14 January-absent and 15 owner-unresolved.
   All 22,455 PDB contributions agree with configuration. Point has seven equal
   48-byte copies, six later/none earlier, and 17 undefined referrer units;
   cross_product has 46 copies but only 45 equal, with breakable_surfaces the
   counterexample. Compatibility is not recovered raw input selection or linker
   command order. Ordinary inline spellings in the prior VC7 matrix still emit
   the forbidden extra point owner; force-inline controls also emit and are
   inadmissible. Reopen only with an authenticated source/storage mechanism that
   passes the unchanged emitted-owner guard and all inherited exact controls.
   Evidence: `scratch/astra-alias-comdat-{summary,census}.json`,
   [point selection record](point_from_line3d_link_selection_evidence_20260919.md).

3. **The old 82,037-byte family is generally wrong inline pressure — rejected.**
   Historical research found equal helper-call multisets in 145/157 functions,
   covering 93.6% of 139,084 bytes. Its 30 micro-TUs support narrower spelling,
   TU-extern and frequency distinctions. Historical selftest 7,283/zero fires
   reads cached pairs; its board falsely flagged the already exact `_bink_alloc@4`.
   This is not a fresh 7,283-control campaign run. The axes2 respelling loses
   exact bitmap_copy (2,784 bytes) and bitmap_2d_uncompress_from_mipmap (880);
   the separate global cross_product trial lost 37 exact functions for one
   480-byte gain. Reopen only after a current per-function object demonstrates
   a wrong call multiset and independent source context supplies a specific
   admissible cause. Evidence: sections 2.1 and 5 of
   [prior research](opus5_next150_research_campaign_20260916.md), and the alias
   packet's retained global-helper rejection.

4. **January Halo PDB locals or lines remain recoverable by another query — rejected for this PDB.**
   Fresh raw enumeration of 848 DBI modules found all 467 Halo compilands have
   one object-name and one compile record, but zero procedure/local/block/line
   records, source files or old/C13 line bytes. All 19 priority modules have
   C/PentiumPro, FE+BE 13.0.9254 and NoDbgInfo. Those records do not encode `/Ob`,
   `/QIfist`, PCH or header order. Reopen only with a different authenticated
   January debug artifact or a demonstrated omitted record format. HCEA locals
   and later RTC records are separately labeled corroboration. Evidence:
   `scratch/astra-debug-evidence/january_priority_modules.json` and
   [debug packet](astra_90pct_debug_packet_20260920.md).

5. **The 2020 symbol executable is proven to use January's source revision — unproven.**
   Its timestamp, v140 RSDS path and external GUID establish another build
   context, not sameness or difference of source revision. Fresh Ghidra found
   2,517 call-linked RTC descriptors / 6,350 local records, all agreeing with
   the raw extractor; its 2,528 raw candidates are not 2,528 authenticated
   function identities. There are 163 assertion-path xrefs across six AI source
   files; 46 anchor the six named target correspondences. Five HCEA AI compilands
   have no S_BLOCK32 records:
   flattened local slots cannot recover literal brace text or full declaration
   order. Reopen revision claims only with independent revision provenance;
   use narrower complete function topology and January gates meanwhile.
   Evidence: debug packet, `scratch/astra-debug-evidence/hcea_scope_records.json`,
   `scratch/astra-context-rtc-all.json`. Tool status is
   **IDA_AUTOMATION_UNAVAILABLE**; the user's later authorization permits
   Ghidra+DIA. No successful IDA analysis or unmet approval is implied.

6. **Redundant stores are universally forbidden, or universally authentic — both rejected.**
   Primary unoptimized branches now authenticate Alert, Obey, one-wide profile
   and vehicle candidates; their independent frozen-baseline controls preserve
   respectively 11, 25, 40 and 14 strict functions. Four unrelated exact controls
   also remain exact: profile_frame_get_value (928 padded), render_ui_widgets
   (560), render_ui_widgets_postgame (384), encounter_test_rule (656). Obey's
   scope ablation changes 4,320 to 4,336 bytes while preserving all 25 siblings;
   independent address-taken storage overlap supports distinct lifetimes, not
   arbitrary brace inference. Reopen another branch-store park only with its
   own primary assignment/topology witness, credible defined C, and complete
   January/ownership controls. An exact byte hit alone is insufficient.
   Evidence: [context packet](astra_90pct_context_packet_20260920.md),
   `scratch/astra-alias-actions-review.json`, `astra-alias-onewide-review.json`
   and `astra-alias-vehicle-review.json` under `scratch/`.

7. **One 8/12-byte aggregate copy/scheduling law closes the family — not established.**
   Fresh frame-statistics evidence has an equal 4,176-byte / 322-relocation /
   1,178-instruction envelope but differing opening physical-register choices.
   Mirror's authenticated float literal correction preserves 18 strict controls
   yet leaves the x87 residual; one additional RTC topology shape is byte-inert.
   The context packet measured five initial shapes (threshold, RTC topology,
   Alert, Obey, scope ablation), followed by one profile and one vehicle shape;
   it did not establish a universal allocation law. Reopen exhausted aggregate,
   helper, pointer-alias or scope permutations only with a new independent source
   distinction, not another spelling lottery. Evidence: context packet and
   `scratch/astra-context-mirror-{threshold,rtc-topology}.*`.

8. **An archived exact donor is admissible because it matches — prohibited.**
   User exclusions specifically cover `_dead_camera_update`'s uninitialized read,
   `_compare_profile_sections`'s uninitialized return, solo-level declaration
   filler, duplicate TU-local frame-statistics layout, manual helper expansion
   and forced code-generation substitutes. No fresh replay of these invalid
   donors was attempted. New communication, action, profile and vehicle reviews
   each found zero source fake-scan leads, but that lexical result alone is not
   source-authenticity proof. Reopen a function only with a new defined,
   independently authenticated source reconstruction that satisfies ownership;
   never re-admit the prohibited donor itself. Evidence: user's campaign request
   lines 198 and 230–252; the communication/actions/onewide/vehicle independent
   review documents alongside this file.

Accounting control: directly iterating the frozen accepted ledger yields 7,548
entries / 1,452,824 meaningful bytes and 7,539 distinct accepted offset-zero
sections / 1,511,627 physical padded bytes; Halo is 7,272 sections / 1,489,642.
There are nine accepted embedded labels. Five libcmt sections (67+85+44+158+44
=398 bytes) have valid offset-zero aliases and are deliberately omitted by the
narrow `function_sections()` helper, not missing from the report. The earlier
7,534/1,511,229 helper-based total undercounts them. Evidence:
`scratch/astra-alias-atlas-ledger-review.py` and `.json`. Canonical residual
293 report rows include 289 function owners and four embedded records; their
parent sections must not be counted again as new physical padding.

No production source, configuration, comparator, guard or admission was edited
for this inventory. Reopening criteria do not themselves confer exact credit.

## Wave2 additions

- **The bitmap exact donor can be replayed without branch evidence:** not accepted on that basis. This wave obtains fresh tools-build instructions and two named RTC rectangles, verifies the complete target graph, then independently rebuilds all 21 functions and audits data/ownership. Its reopen criterion is now satisfied and its park retired. The recovered state names remain descriptive numeric states where no authentic enum names were found.
- **A later string table proves ai_debug had no fifth point:** disproven by January's earlier 12-byte copy and two address uses. Storage is reused across disjoint lifetimes. Restore the actual point graph; never infer a complete frame from simultaneous use of every observed slot.
- **Equal ai_debug size and relocation count imply exactness:** rejected. Candidate 24,976/1,905 still differs in SHA, frame size (0x81c versus 0x810), first table position and x87 lifetime. The MAX and all-marker-helper variants are excluded; only the five-point and vision graphs are retained at zero credit. Three later 80-byte tempbuf records do not prove January buffer extents; no fifth pressure probe is justified.
- **Perception aggregate scopes or a direct distance helper solve the residue:** fresh scopes are byte-inert; the direct helper also introduces two surplus COMDATs. Both are excluded. The selected one-index/repeated-guard correction remains zero credit. Five measured shapes exhaust this packet until new evidence resolves the remaining call-return and squared-distance schedule.
- **Looking requires copied rather than recomputed equality:** fresh unoptimized instructions independently evaluate both equalities, correcting the prior missing-evidence rejection. The final type/store/equality source is still residual after five shapes. Do not introduce a later unused flag, arbitrary address-taking, or declaration filler to move secondary_aim_valid into a January stack slot.
- **The four new collision local/control facts close aggregate scheduling:** three variants are byte-inert and one worsens size; no production change. New evidence must explain its specific preheader/backedge loads and NONE lifetime. Repeating those four shapes without a new distinction is not justified.
- **Avoidance needs another scalar/helper spelling sweep:** fresh RTC, typed arrays, raw thunk and two scaled-add calls corroborate the already known source form, but supply no admissible new owner/header mechanism. Zero shapes were tried; six strict read-only controls pass. Prior forced-inline, manual expansion, PIN, branch and adjacent-index forms remain excluded.
- **Supplied debug artifacts were overlooked by ignoring executable directories:** the fresh hidden-inclusive census records 13 images, zero embedded COFF tables, eight external v140 RSDS paths and no paired PDB/map/rsp. Original full January optimizer/header/COMDAT input evidence is still missing. Search another authenticated artifact only when there is a concrete new lead.

The bitmap, perception, looking, collision and ai_debug wave2 packets give full probe outcomes and independent reviews. None of these narrow mechanisms establishes a universal compiler law. Only bitmap receives new exact credit.

## Wave3 additions

- **Biped diagnostics cannot be restored because the globals lack declarations:** resolved through the genuine collision_debug owner header after full consumer enumeration and independent storage/type proof. No consumer-local foreign prototype or new storage was needed. All five header consumers retain code/data/COMMON equality; the biped diagnostic is restored at zero exact credit.
- **A decompiled moving copy direction is fresh evidence:** withdrawn. RTC field offsets and raw /Od instructions show new_velocity is zeroed then copied into velocity, the already tested UMG6 direction. The opposite scratch shape is inadmissible even though it preserves controls and reduces size. Always verify decompiler copy propagation against raw operands.
- **Using a narrower biped accessor is needed to satisfy typed access:** corrected. objects.h object_get already returns the typed object pointer and supplies January's all-object mask. The narrower mask is absent from the final source; no invented validation difference is retained.
- **Physics matching size/relocation count closes the function:** rejected. Candidate and target have 5,376 padded bytes and 124 relocations but different normalized SHA. The existing safe contact guard and scheduling differences remain; the 5,366 meaningful target bytes receive zero credit. No unsafe collision index is restored to obtain exactness.

## Research packet4 additions

- All packet4 source candidates remain unlanded: four communication shapes, three physics shapes, two decal mechanisms/composition, zero frame-statistics shapes. None earned exact credit.
- A target immediate2.0f does not prove literal source when the current global is const; no such spelling probe was compiled.
- Private optimized register argument placement does not determine C argument order. Authentic decal parameter correction is byte-inert; no ABI-gain claim.
- Raw decal failure-index stores explain a concrete lifecycle effect, but restored frame/register allocation does not close strict matching.
- Physics must snapshot after translation; the earlier broad candidate's opposite cross-call ordering was not equivalent. The force-add graph was already in an old broad rewrite; isolated evidence is not a new general law.
- RTC statistics[192] is16 memory-report triples, not a larger shared counter owner. No duplicate layout or pressure correction is justified.
- Gate/regression success verifies preservation only. Source fidelity and target extents are not coverage gain; do not package further zero-credit source cleanup as campaign advancement.

## Research packet5 additions

- Coherently narrowing the sound pitch-range parameter does not preserve its exact owner: long->short changes mov ESI to mov SI, adds an operand-size prefix and shifts both relocations. Both files restored, no consumer shape, no cast-deletion or prototype-mismatch retry.
- Named lens-marker i/j/k fields are primary-type-backed but byte-inert, including all controls/data. The independent rasterizer_lights consumer is residual, not an exact control; the packet corrects that preliminary misstatement.
- Equal direct-call multisets on 104/106 high-match functions do not prove all boundaries/arguments correct, but cannot support a broad missing-helper fix. Earlier cached family sizes are not current gain ceilings.
- Current exact stopping-distances and collision-BSP vector controls supersede old residual labels. Mixed same-function operand ordering rejects uniform x87 operand reversal; the studied frame/j-k cases are outside M8's documented scope.
- Later projectile RTC inventory includes branches absent from January. It is not a January declaration-order witness. No new source shape follows.
- Debug-profile numeric switch/guessed enum donor remains inadmissible; fresh Ghidra and empty HCEA symbol queries do not supply enum identities. Whole-file donor import would also threaten inherited exact code.

## Wave6 additions and one satisfied reopening criterion

- January CG/instrumented variants are not alternative compiler witnesses for the measured residuals: 282 unambiguous extents, all instrumented-identical, and 28 CG detours exactly replaying original prefixes. One ambiguous name remains omitted. No source or private-name inference follows from instrumentation paths/strings.
- The alias opportunity census still leaves only the old obstacle-path veto code-equal but unaccepted. Snapshot identity remains unproven; byte equality does not reopen its admission. Do not count already accepted historical near-hits.
- ARGB has two authentic visibility probes, both restored: the owner becomes exact but the same screen-effect exact function regresses. The second probe legitimately preserves selected providers while removing three redundant copies; therefore surplus-copy removal alone does not cure the problem. Reopen only with new original context explaining both the converter owner and the screen-effect instruction schedule, not declaration/flag permutations.
- Collision-debug raw camera copies, frame records and absent DIA locals do not establish the proposed buffer-size/source-scope change. No new compiler shape was justified. Existing exhausted spelling history still applies.
- Hardware vertex park retired: first-party Unlock/result topology plus genuine unchanged XDK behavior supplies a new admissible mechanism. Independent review and full validation prove +354 meaningful bytes and no inherited losses. This does not admit the old whole-file donor or change the policy for behavioral helpers.
- Triangle remains parked: one merged January error block and a materially different later DX9 device-null branch do not authenticate duplicate source failure reports. No fresh triangle compilation. Reopen only with new source-level branch evidence or a different supported mechanism.
- Two tiny stock SDK no-op/constant-return copies are harmless, disclosed at zero extra credit, and do not uniquely name retained anonymous targets. No target metadata rewrite follows. The original CreateVertexBuffer failure bug is independently authenticated and unchanged; this is not permission to introduce UB in another residual.
- The optional local-artifact question is closed by the user's exhaustive confirmation. Continue with available January publics/strings, Ghidra, DIA and cross-build evidence; no repeated search for byte-identical archives.

## Packet7: exact scratch output is insufficient when a source requirement remains unproved

- **Environment:** the old claim that no intensity factor is evidenced is superseded by fresh raw first-party code showing an initialized 1.0 local and six real uses. The proposed complete match nevertheless needs duplicate filter statements with no independent per-branch witness. One common-tail ablation isolates that requirement. Hold the entire candidate at zero credit. Reopen only with genuine branch/source evidence or an independently authenticated context correction closing the common tail; no further filter/declaration spelling sweep.
- **Projection:** after authentic September19 context changes, the old bitmap/Decals losses no longer reproduce, but the full 272-consumer test loses `_unit_preprocess_node_orientations` through a two-load ordering change. Reject the leaf-face gain and restore the header. Reopen only with a concrete authentic context correction preserving this current falsifier and all consumers; no order/return/flag permutations or unrelated units-source rescue.
- **Infection:** raw unoptimized code proves initialized dword flag manipulation before the control-packet clear, so that boolean lifetime is expressible in ordinary source. The one new shape still has 3,600 bytes/104 relocations versus 3,616/104 and differs in registers, branches and x87 clusters. No landing or additional pressure probe.
- **Vehicles:** primary helper and RTC records support two in-place point_from_line2d calls and one shared branch accumulator. Both bounded shapes remain nonexact and unlanded. The naturally emitted helper copy has no credit and does not authorize an ownership exception or a new scalarization sweep.

## Packet8: fixed owner reconciliation and authentic helper/branch probes rejected

- Cone declaration at its fixed historical real_math.h location still loses the Decals reload order. One fixed composition with the authentic projection body preserves units but yields the identical Decals failure. Both restored; no further position/order/combination rescue. Typed aim setter remains an uncompiled proposal, not a new tested exact body.
- Sound-listener's fresh raw graph and parameter/type records supply no missing use or valid layout change; its two-reload transposition remains. Do not repeat previously rejected declaration/loop/TU-order variations.
- Avoidance's unchanged current fingerprints and fresh raw/RTC reread reproduce prior evidence. No new source mechanism or helper-ownership exemption is established.
- Action-charge's later melee-first whole-arm topology is primary-supported and fixes one historical flag allocation, but leaves a reload and aggregate-copy/scheduling gaps. No second shape is justified; later added normalization and lead-fraction operations cannot be imported into January.
- Damage's genuine collision_test_line helper spelling produces the same known best residual, plus a target-absent behavioral helper. Unreferenced emission does not make this behavioral body eligible for the narrow stock-XDK no-op/S_OK exception. No credit or further clamp/lifetime spelling sweep.
- Current compiler hashes match the existing 9254.1 toolchain; 9210 is a known prior alternative, not new QFE evidence. Obsolete C2 profiler bucket labels are not live pass entries.

## Wave9: weapons closure and HUD research

- Weapons reopening criterion satisfied by separately stored and used short quotient/remainder outputs in the raw later body, combined with the already authenticated trigger-array context. One shape is fully exact and preserves all inherited owners and controls. This does not establish a general rule for scalar declarations or admit prior speculative variants.
- Fresh HUD graph, typed locals, RTC and January relocation evidence add no distinct mechanism beyond the prior aggregate/flags/number-call/conversion history. Zero new shapes and no production changes; do not replay exhausted R3/distance or ARGB-header probes.

## Packet10 additions: no production change

- Actor-looking: primary-backed saved-major helper outputs compile identically. Do not repeat this paired return-assignment shape; a new mechanism must explain a remaining January difference.
- Communication-event: primary-backed direct indexing, nineteen store roles/order and previous-entry diagnostic reads produce 8112/335, not January8064/336. The known typed/P2/refetch composition is prerequisite history, not newly discovered work. The genuine header replay is fully restored. Do not turn this result into a declaration, field-order or inlining sweep.
- Projectile creation: fresh helper/RTC/DIA evidence confirms the already exhausted point-helper route. The archived exact function still emits an inadmissible48-byte point owner and predates the new weapon_update admission. No new probe or exception.
- Perception refresh: distance and optional outputs already have their consumed uses. Direct byte comparison also finds two short JP destinations, beyond the argument-scheduling window; alignment diagnostics do not erase them for admission. Dead initializers and unproven scope changes are not new mechanisms.

## Packet11 additions: no production change

- Actor movement: current consumed float/byte/short outputs already match the available evidence. Point/blend, crouch, hover, flags and declaration variants are exhausted; fresh confirmation is not a reason to recompile them.
- Biped movement: the 204-byte structure and flags are already represented. Full-width actor argument, UMG6 vector-copy direction, field/flag and radius variants are historical tests; do not replay them as new shapes.
- January PDB slack: the bounded listed-record scan finds no complete named candidates. Do not repeat this same scan or infer recovered locals from administrative bytes. Reopen only with a materially different supported record/fragment recovery mechanism and clear provenance requirements.

## Packet12 additions: real source facts, zero production gain

- AI debug: selected-color text / blue clear-ray and purple output-throttle length1.0f are primary-backed consumer corrections. Two shapes preserve controls but remain nonexact; no default-arm, declaration-order or old marker-helper replay. Critical marker FSTP/reload operations remain identical despite shifted stack homes. Reopen only for a distinct primary-backed mechanism.
- Weapon placement: the old HCEA-void claim is disproved by raw procedure records. The authentic long return yields a 193/208-byte strict scratch function, but its coherent real owner header regresses an inherited Units match. No false consumer-local prototype, header-only ABI mismatch or isolated body admission. Original generic-void callback binding is an authenticated quirk, not made language-defined by a cast.
- Placement plus projection: one fixed composition of two authentic corrections produces the same Units regression and stops at its first gate. All272 source/object snapshots and both headers are restored. No further combination, ordering or declaration-position sweep without a new independent causal fact.
- Argument locator: matching callees alone do not prove arguments. Preserve final coverage limits and unknown values. Initial stale-stack, partial-push and x87-store defects are fixed; initial output is not validated final output. The final scanner does not independently cover the AI length correction or swapped-color path joins.
- Network close: actual January error literal contains the parenthetical omitted in current source. The existing five-iteration versus four-element traversal veto remains. Literal fidelity alone is not a full-function admission; no source probe made.
- Perception and reload: available evidence confirms already-tested value webs and shapes. Do not rerun historical short-output, MIN/saved-total, desire-prop argument binding or aggregate/helper spelling variants without new provenance.

## Wave13: setup source criterion satisfied; bounded scouts closed

- action_charge_setup: the old repeated-store hold is resolved by fresh independent unoptimized instructions. The existing initializer is moved before both failure tests; one shape is fully exact and independently rebuilt, with no inherited runtime change outside setup. This does not license arbitrary duplicated statements or reopen perform's exhausted allocation variants.
- Ordered call differences:44 excluded residuals are not44 missing-call fixes. Read the per-function census before testing; widget tails, HS discarded function lookup, sound deletion, teleport else duplication and observer zeroing were already probed or vetoed. Inlining, indirect-call opacity and decoding boundaries must be resolved before treating a call delta as source evidence.
- Unwritten player stub: raw HCEA/DIA proves private void(long,long), correcting the old Boolean premise. Later dispatch invokes an empty body and ignores its result. January original name/linkage and authentic unreferenced emission are still missing; no public no-argument stand-in, synthetic linkage rewrite or unused-static emission trick.
- Aliases: prior15rows, decals and one-wide remain accepted. The delta-only check adds no new source-owner evidence for obstacle snapshots, fog statics or helper selection. Do not replay their old source forms or relax comparison.

## Packet14: evidence recovered without a full-match mechanism

- Aiming blockage: unoptimized returned-word flow now authenticates the old vertical zero assignment. Do not repeat that proof. The January reciprocal lifetime remains unexplained; old body3b is not a new source shape or exact donor. A later point-operation call does not authorize a surplus helper owner.
- Normalize2d: nine current exact controls, including both same-expression actor-looking validators, support the existing helper and consumed reciprocal. The historical all20-site claim is not a universal type/helper law. Reopen only for a new independently supported caller lifetime/use/type/call boundary that preserves these controls; no operand, scope, declaration or flag sweep.
- Branch assignments: Bink query70 is the sole qualifying strict hold in the bounded remaining roster. Separate START/BACK assignment arms still need independent evidence. Bink draw909 is a different index/aggregate hold; converse248 is asm-only; sound619 has no strict donor; progress349 is a spill issue. Do not treat these as seven thousand recoverable bytes or replay obsolete accepted candidates.
- ABI: raw hs_evaluate_begin_random return void and short/long/unsigned-char arguments agree with current source. No new signature probe follows. The other seven examined rows had explicit prior type evidence or no newly identified consumed-output mismatch. This is a bounded negative result, not proof that all residual signatures are correct.

## Packet15: improved bounded diagnosis, no new source lead

- The extended argument scan is complete for its declared scope. Its only hit is the already-authenticated network parenthetical; do not re-probe the literal while the four-versus-five array-bound hold remains. No new production change is justified by this run.
- Exact controls establish covered representation consistency, not complete x86 soundness, source equivalence or ABI/branch-role identity. Unknown tail/indirect exits, spill reloads and unsupported FP arithmetic remain unknown. Do not add synthetic cleanup/no-return overrides merely to recover purple/circle positives.
- Selected and real-witness coverage receipts predate the final full-result SHA. Preserve their version labels; final counts come from result.json and the independently bound review. The setup control's seven words reflect an earlier production admission, not this diagnostic's gain.
- Biped physics: the four inspected output/argument boundaries expose no new source premise. The wave3 debug restoration is already present. Do not repeat 39 RTC records or remove the required best_collision_index != NONE guard to chase equality.

## Packet16: current source boundaries and HUD qualification

- Models, lightning/lens flares and the three player-control owners reproduce the documented frontiers. Do not replay the same helper visibility, operand, declaration, scope or argument spellings as new research.
- HUD messaging: old normalized equality is historical, not a current source/admission proof. Fresh unoptimized x0,x1,y0,y1 rectangle stores do not authenticate the old24-order selected y0,x0,x1,y1. Buffer256/local/branch corroboration alone does not close this gap. Do not call a permutation-selected body independently authenticated by this capture.
- HUD aliases: the existing objdiff-coff-shape route is conditional; no comparator rewrite is inherently required. Equal addresses alone and a parked alias class do not grant code credit. A current admissible source body, complete bytes/relocation geometry and independent static-owner binding remain necessary.
- Public metadata is a lead locator. Do not call a PDB full-symbol merely from its extension, confuse announced prototype existence with a usable archive, or repeat the closed local-copy search.

## Wave17: private player owner proved; data/topology holds retained

- player_examine_nearby_unit: original earlier private-owner records plus anchored leaf correspondence, prior raw void(long,long) typing, authentic dispatch and natural emission satisfy the bounded source test. Only one meaningful byte is admitted. The old public/Boolean premise remains rejected; no forced-emission workaround or same-source claim is authorized.
- Fog: the new MAP Static symbols tables contain function records only, not21private data declarations. Original function names do not prove the proposed storage partition.
- Obstacle: public debug-flag names do not identify the held private snapshot owners; semantic veto remains.
- Environment: September Xbox exposes the actual vector-test path but retains January's common filter tail. Identical optimized scheduling does not prove duplicated lexical statements; the old exact candidate remains held.
- Public inputs: advertised external PDB identities and archive listings are not embedded type/local/line evidence. Preserve the August patched-filename caveat and the closed local archive question.

## Packet18: original private-header evidence tested without gain

- object_update: both original private identities/body/calls justify the linkage-only test. All150consumers produce no new function bytes or matches. Do not repeat via prototype order, padding declarations or compiler flags.
- ai_conversation_new: complete original private bodies and HCEA type corroborate the single correction, but all21consumers remain byte-inert. No gain and no extra source-context variants.
- Three private biped owners: current ABI and bodies preserved; only necessary snap forward declaration added. The23consumer test loses inherited unit_preprocess_node_orientations and gains nothing. Rejected/restored. Do not trade that exact control, copy later snap ABI or sweep private-prototype positions.
- Private-declaration correction is not a transferable exactness law: two tests are byte-inert and one moves loads in an unfavorable direction. The wave17 collateral change did not predict a useful broader gain.
- Earlier AI windows reproduce known call/lifetime/arithmetic boundaries. Newly original-authenticated path-renderer names are distinct evidence, not permission to replay the historical alias-based stub cleanup or change target metadata for credit.
