# Astra 90% campaign handoff — 2026-09-20

**Cumulative verified gain: +8,314 meaningful / +8,352 padded bytes / +6 accepted functions, with zero regressions.** Halo is **81.34299%**, 1,439,906/1,770,166 meaningful bytes and 7,287/7,574 functions. Overall: 7,554 functions / 1,461,138 meaningful bytes. **90% is not reached: 153,244 meaningful bytes remain.** No universal compiler law or impossibility conclusion is claimed.

## Location and immutable checkpoints

- Worktree: `C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\work\astra-90pct-breakthrough-20260920`.
- Branch: `astra/halo-90pct-breakthrough-20260920`; independent sibling Git database `halo-campaign.git`. No push; dirty canonical checkout untouched.
- Original remote baseline: `47d2e7b69f1c1277123e979296b6f0a7de00c80b`, verified against `refs/heads/jonas/exact-pilots`. The other remote branch `exact-pilots` is different.
- Wave1 commit: `905e5e3b41591af98c82accf3f57fc38118c2e7f`, +7,594 meaningful / 7,632 padded / 5 functions.
- Wave2: +720 meaningful / 720 padded / 1 function. The output receipt records the commit containing this handoff; `git log -3 --oneline` gives local IDs.
- Stage0 `scratch/astra-90pct-before.json` and all `*-before.*` reports remain frozen. Wave1 `scratch/astra-90pct-wave1-after.json` and `astra-90pct-wave1-summary.json` remain frozen.
- Wave2 stable snapshot, semantic ledger, summary, tests, audit and owner census are frozen under `scratch/astra-wave2/`; latest verified files are under `scratch/astra-wave3/`. Do not rerun a baseline builder against changed objects. Schema4 atlas fields retain Stage0, wave1 and wave2 separately from `wave3_after`/`latest_summary`.

## Admitted exact code

| Function | New meaningful | New padded | Relocations |
|---|---:|---:|---:|
| _action_alert_next_position | 770 | 784 | 18 |
| _action_obey_command_begin | 4,320 | 4,320 | 167 |
| _action_vehicle_desirable | 272 | 272 | 7 |
| _ai_communication_find_actor_to_reply_to_player | 1,090 | 1,104 | 65 |
| _extract_sequence | 720 | 720 | 28 |
| _player_profile_1wide_list_update | 1,142 | 1,152 | 70 |

The admission manifest retains full target/candidate fingerprints and independent reviews. One-wide has one symbolic alias at +0x1A9: `_cached_player_profile+156` and `_local_player_index_for_draw_string_and_hack_in_icons` resolve to the same January address. All 70 relocations were independently resolved. The existing accepted ledger admits it; the symbolic stable gate remains residual. Cumulative stable gain is therefore **5 functions / 7,200 padded**, versus accepted-ledger **6 / 8,314 meaningful / 8,352 padded**. No comparator, target, symbol or credit exception was added.

Wave2 extraction is supported by freshly recovered tools-build instructions for assertion bounds, initialization, state fallthrough, independent repeated tests, and an actual copied working rectangle. January normalized code and all 28 relocations agree. All 20 inherited siblings survive. Three wrong assertion literal owners are replaced by their January identities. Its documented park is satisfied and retired.

Whole-object admissions: Action Alert in wave1 (12 functions, 2,120 meaningful / 2,240 padded code, 80 aligned data bytes); bitmap_extract in wave2 (21 functions, 13,002 meaningful / 13,168 padded code, 4,496 canonical aligned data bytes). These statuses do not duplicate function credit. Bitmap's six inherited pooled floating literals retain authentic unique providers under the existing rule; no new helper or exception. Action Obey retains its existing whole-object ownership veto.

## Best-source corrections at zero credit

Wave2 changes three residuals spanning **34,010 target meaningful bytes**, all at zero exact credit:

- Perception update, 4,720: one initialized orphan index and an independently repeated hostility/attachment guard. Five shapes measured; 35 exact siblings preserved. Scope spelling and a direct distance helper were excluded; the latter introduced two surplus COMDATs.
- Look update, 4,714: authentic `real[2]`/`real *` private type closure, two independent equality evaluations and flag/vector statement order. Five shapes measured; 14 exact siblings preserved. The helper and other callers retain their machine code. Remaining secondary-aim flag and desired-facing register homes are not explained. No dead flag or address-taking was invented.
- AI debug render actor, 24,576: restore the fifth marker point and zero-vector initialization followed by three basis accumulations. January uses the point before later string-table slot reuse. Four shapes measured; 57 exact siblings preserved. Its 24,976 padded bytes / 1,905 relocations equal January, but SHA, frame, table start and x87 scheduling still differ. No fifth pressure probe or unproved buffer-size change.

Wave1's four changed fuzzy extents remain separately historical: communication event 8,064; finished 1,583; mirror 861; vehicle setup_impromptu 454, totaling 10,962 meaningful bytes. None was counted as exact. Finished retains its original reopen criterion and incomplete diagnostic status. Target extents are not amounts of new source or coverage gained.

Collision move point received four new evidence-backed scratch probes, preserving 19 exact siblings, with no source landing. Three were byte-inert and one worsened size. Its preheader/backedge aggregate-load schedule and NONE lifetime still need a specific new source distinction.

## Completed verification

Full `ninja -j4 all_source progress semantic_progress` passes. Tests: **1,152 passed, 5 skipped, 26 subtests passed**. Stable and accepted ledgers lose no inherited exact function. Parks **185 active / 0 stale / 0 invalid**; admission **19 candidates / 0 contradicted / 8 rejected / 0 revoked**. Fake findings stay **22 globally**, with **0 in the four wave2 source files**. All seven inherited point helper emitters, runtime owners and COMMON definitions remain intact. Bitmap's corrected assertion literals are the only wave2 runtime-data delta; independent whole-object review covers them. `git diff --check` passes.

Current physical padded accepted code: Halo **1,497,994**, overall **1,519,979**, deduplicated by accepted offset-zero `(unit, section)`. Current residual inventory: **287 original report rows / 330,260 meaningful bytes**, including four embedded labels and seven actually unwritten functions / 3,733 bytes. Embedded labels never add parent padding again.

Wave2 source files: actor_perception.c, actor_looking.c, ai_debug.c and bitmap_extract.c. Root changes config/config.json only for bitmap Matching and config/parked.json only for extraction retirement. No header, symbols, target metadata, optimizer options, comparator or admission policy change. Independent reviewers rebuilt every candidate against frozen wave1 source before integration.

## Evidence and next concrete work

Read the research atlas, rejected-hypothesis inventory and per-packet independent reviews before reopening a residual. No original January local/line/lexical-block records, full optimizer options, QFE or header order were recovered. All 467 Halo PDB compiland streams lack locals and lines. FE/BE13.0.9254 and NoDbgInfo are proven; `/Ob` and `/QIfist` are not proved by those records.

The cross-build census records 13 supplied images with zero COFF symbol tables, eight external v140 RSDS references and no paired PDB/map/rsp. The 2020 executable's source revision remains unverified. Fresh tools-family source assertions nevertheless supplied the extraction graph. RTC protected aggregates and HCEA typed locals support narrow mechanisms; they do not give complete January scalar declaration order or lexical braces.

Actor movement avoidance is now documented with fresh raw two-scaled-add/helper evidence but zero source shapes and zero credit; its six strict read-only controls pass. Its existing point-owner/header boundary remains unresolved. Biped physics/moving and its owner-header review are complete in wave3 below. Continue with a new bounded source-context packet after reading its prior rejection/reopen history; no pending worker source candidate remains. Missing authentic shared-header declarations can be root-reviewed after enumerating every C consumer; missing declarations are not a reason to invent consumer-local foreign prototypes. Never restore the historical collisions[-1] path or any uninitialized donor to force a byte hit.

Point ownership remains a cross-cutting blocker. The current first-copy census has counterexamples, and current copies do not reveal discarded raw January COMDAT inputs. Preserve the seven inherited emitters and reject surplus copies. Require an authenticated source/header/storage mechanism, not forced inlining or manual helper expansion. No broad 8/12-byte aggregate-copy scheduling law is established. Do not repeat exhausted declaration/scope/pressure shapes without new evidence.

## Tool status and optional IDA checklist

**IDA_AUTOMATION_UNAVAILABLE**, independently verified by the user. The only install is `C:\Program Files\IDA Free 9.4\ida.exe`, without IDAPython, RPC or another callable endpoint. Continue with Ghidra headless and DIA2Dump/PDB tooling as authorized. IDA GUI is optional; no finding in this wave requires it and no approval remains pending.

Optional manual corroboration: use the supplied `halo_cache_symbols.exe` (SHA74086968...) in a new scratch IDA database; inspect function 0x49C470, marker copy/line calls 0x49DBF9..0x49DD43 and vision zero-copy/basis calls 0x4A3648..0x4A3768. For bitmap, use `tool_symbols.exe` (SHA079d20d7...) in another scratch database and function 0xB96A10, independent state tests 0xB96C73..0xB96CB6 and rectangle copy 0xB96CE1..0xB96CEA. Compare operands, branches and calls with preserved raw records. No original source text is implied by GUI decompilation.

## Reproduction and packaging

Root's `scratch/astra_wave2_checkpoint.py` performs the full verification and records both baseline comparisons. It uses wave1 as the source-change base, so do not treat it as a generic later-wave generator. `astra_wave2_update_records.py` preserves Stage0/wave1 fields while adding this verified state; `astra_wave2_write_handoff.py` writes this checkpoint. For a later wave, create fresh snapshot/report names and update the explicit baseline instead of overwriting older evidence.

Workers own one C file, use `tools/campaign/gate.py` with distinct scratch objects, and never run Ninja, edit shared headers/config, commit or push. Root independently reviews, integrates and runs whole-tree checks. No inherited exact loss, new behavioral owner, fake dependency, manual byte, invalid donor or credit exception is allowed.

Campaign records, scripts, source patches and normalized fingerprints are committed locally. Raw whitespace-bearing text captures are preserved byte-for-byte in `scratch/astra_raw_evidence.zip` (wave1) and `scratch/astra_wave2_raw_evidence.zip` (wave2), and `scratch/astra_wave3_raw_evidence.zip` (wave3). Input executables/PDBs, toolchain, binary probes and caches remain local and outside delivery archives. The output package contains the current source/config patch, manifest, atlas, evidence and a commit receipt. No push or canonical modification is part of this campaign.

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
