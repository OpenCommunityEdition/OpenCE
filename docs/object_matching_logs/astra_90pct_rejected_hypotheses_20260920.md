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
