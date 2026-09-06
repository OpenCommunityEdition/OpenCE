# Encounters initializer cluster — independent first-candidate review

## Verdict

**PASS for function-level integration of the frozen natural packet; not an
admission of whole-object Matching.**  The public initializer and private
starting-location reset are strict January matches.  The private constructor
is a complete, behaviorally equivalent 98.40206% residual and should be
retained honestly as a park.  The candidate introduces no helper, COMMON,
BSS, or other unexpected owner and changes no inherited runtime owner.

This review made no production source/header/configuration change and did not
compile a source variant.  Root owns production integration and all full-build
gates.

## Frozen inputs

| role | path | SHA256 |
| --- | --- | --- |
| January target | `build/split/source/ai/encounters.obj` | `7EDC1863EB2C08F7F2AD612E3CFA7B246C0EB9A3B5907FD08C2C17ECD9A056A3` |
| accepted-schema baseline | `scratch/encounters-schema-after-20260905/source/ai/encounters.obj` | `55A4CA43B592DEED7934687FCA14B43F6F87322616725CB60408ED2042D7EE59` |
| first natural candidate | `scratch/encounters-initialize-cluster-first-natural-20260905.obj` | `4D921A37F50E65FB8FDDE426D9A93B8B4D208D5DE35EE77E8D8C992578CD8649` |
| source fragments | `scratch/encounters_initialize_cluster_natural_20260905.inc` | `DC50E8CF84616D33FFEFBD80284956EE889992A389BF155DA757C14BBB9B780D` |
| production manifest | `scratch/encounters_initialize_cluster_production_20260905.manifest.json` | `8D4C1FB902801545A441FBF4C83EFFAC9B754BB3ACD88EFF880FA7896522F0C1` |

The manifest's four source anchors are unique in the frozen source context.
The candidate was evaluated unchanged; no declaration, lifetime, spelling,
barrier, pragma, force-inline, or flag alternative was tried.

## Function measurements

The gate moved from 43 exact / 4 residual / 14 unwritten to 45 exact / 5
residual / 11 unwritten.  All 43 inherited exact functions and all four
inherited residual bodies are byte/relocation/owner-identical to the accepted
schema baseline.

| owner | target | candidate | relocs | result |
| --- | ---: | ---: | ---: | --- |
| `_squad_reset_starting_locations` | 198 meaningful / 208 padded | 198 / 208 | 8 / 8 | strict, hash `3CD9E2AE...942C4` |
| `_encounters_initialize_for_new_map` | 146 / 160 | 146 / 160 | 11 / 11 | strict, hash `959E27E3...3090` |
| `_encounter_new` | 652 / 656 | 649 / 656 | 31 / 31 | residual, target hash `3984B20F...D288`, candidate `F95C0E4D...2719`, ordinary objdiff 98.40206% |

The ordered relocation destination list for `_encounter_new` is identical in
all 31 positions.  The only substantive instruction-selection difference is
the respawn-total result materialization:

- January loads 999 into EAX, branches on a zero `short`, otherwise sign
  extends the original `short` into EAX, then stores AX;
- the candidate branches on the same zero test, loads 999 into EDI only for
  zero, and otherwise stores the original DI.

Both paths store the same low 16 bits for every possible input value.  The
candidate sequence is three bytes shorter, shifting the remaining platoon
assert/loop code by three bytes and adding three terminal alignment NOPs.  No
source correction is justified; the complete January gameplay and assertion
behavior remains present.

The 30.0f multiplication relocation displays as target-undefined versus
candidate-defined in raw disassembly only because the accepted baseline
already emits `__real@41f00000`.  Symbolic relocation normalization yields the
same name and addend.  It is not a new helper or new data owner.

## Complete owner and linkage census

Candidate versus accepted-schema baseline:

- code owners: 51 -> 54, exactly the three intended functions;
- noncode owners: 49 -> 53, exactly four assertion/message literals;
- COMMON: the same four inherited 4-byte tentative owners
  (`encounter_data`, `squad_array`, `platoon_array`, `pursuit_data`);
- lost code/noncode owners: none;
- changed inherited runtime code/noncode owner payload, relocation, section
  flag, alignment, storage, type, value, or COMDAT selection: none;
- new BSS, helper, point helper, or candidate-only code owner: none;
- newly undefined external owners: only genuine existing APIs/data
  `_data_make_valid`, `_csprintf`, and `_temporary`.

The four new literal owners total 218 physical bytes and are each owned by the
January Encounters target itself with identical bytes, relocation count zero,
section flags, alignment, external storage, symbol type/value, and COMDAT
selection 2:

| literal role | bytes | normalized SHA256 |
| --- | ---: | --- |
| squad overflow message | 39 | `38108450BC5C6DED1CB990DEC752B0604D7B1D0E29C96C9CC900B0FADBEEDA92` |
| platoon overflow message | 41 | `246EDBA4EC9DB8D71F254A13BF04061E4676D0A7CD5B3A568F4222B686896DED` |
| squad-count assertion expression | 67 | `BDC868307B02F75A864AB07809F873D5EC1E8570A5751E6A51AB9F2281269EFD` |
| platoon-count assertion expression | 71 | `FE9B25682876C40DA1BDBC32BBF17C6D7E99A53D9F5F6C8BB28D754CCFBF4299` |

A complete 833-target scan found each of those four selected only in the
Encounters target with the properties above.  `__real@41f00000` is an
inherited baseline SELECT_ANY 4-byte owner with hash
`409303C5...1D96`; the selected January copy is in ActionAlert and has the
same bytes, flags, alignment, storage and selection.  There is no selection
mismatch and no new genuine helper, so no ordinary duplicate-link diagnostic
was required.  This is not a claim of a successful whole-image link or
whole-object ownership cleanup.

The candidate still contains four target-absent code owners and ten
target-absent noncode owners that were already present in the schema baseline;
they are inherited whole-object debt, not introduced by this packet.  In
particular, the existing `encounter_get_squad`/`encounter_get_platoon` owner
copies are unchanged even though the new constructor genuinely calls them.

The complete unsuppressed metadata comparison also records two inherited
compiler-metadata differences: the anonymous `.debug$S` payload changes from
197 to 184 bytes, and `_encounter_create`'s `$L...` compiler-local labels are
renumbered. Neither changes runtime bytes, real relocation destinations, ABI,
storage class, or linkage. All other inherited section records are exact.

## Inherited residual classification

`_encounter_create` is 736 padded bytes / 38 relocations with normalized hash
`37F1F1D0...831A` in both the schema baseline and candidate.  The January owner
is 752 padded bytes / 38 relocations with hash `2218EEE1...A7F9`.
Consequently this is not a stale-park change caused by the initializer packet.
The only baseline/candidate symbol-table difference in its section is a
mechanical renumbering of `$L...` compiler-local labels; every label value,
storage and type, the public owner record, section bytes and relocations remain
the same.

## Semantic and bounds review

The frozen source uses only existing typed owners.  It preserves the full
encounter datum handle for `datum_get`, masks only the scenario lookup to the
absolute low word, uses signed-short encounter/squad/platoon iteration, makes
the encounter and pursuit arrays valid before clearing the typed runtime
arrays, keeps allocation failure as a silent skip, and preserves all four
January fatal bounds assertions and their messages/line metadata.

Squad delay, automatic migration, respawn-budget, starting-location reset and
platoon defending behavior agree with the complete January instruction
streams.  Required-location flags are ORed into the bank established by the
whole-array clear; reset intentionally fills only the unused bank.  The
accepted schema supplies platoon bit 2 and starting-location bit 0 through the
real owner header.

The one-word required/unused banks are valid for the corroborated tag domain
of 0..32 starting locations.  The code intentionally has no local clamp or
assert; count 33 would reproduce the original overwrite into the following
field and is outside the valid-tag precondition.  No array widening or safety
facade is admitted.

The independent differential harness at
`scratch/encounters_initializer_cluster_behavior_20260905.py` passes all 49
target/candidate cases, including exact external API order, allocation failure,
all flag/default/respawn paths, valid counts 0..32, full memory images and all
four assertion paths.  Its durable report is
`docs/object_matching_logs/encounters_obj_initializer_cluster_runtime_review_20260905.md`
(SHA256
`2FE3430E353A22332B340209CE205D39AB03613379C82C9D873CA5EB5FDFFF52`).

## Reproducible evidence

- full owner audit:
  `scratch/encounters_initialize_cluster_owner_audit_20260905.py`
  (SHA256 `B66EE42A7425E858FD45D5855A51C03CC6171DB44A0787D569757E29EFF78EAB`)
  and `.json` (SHA256
  `7A4165E93C0953A964A890B4452D72B8FFC9E66E07D465E258EB0C183450EEE2`);
- pooled-owner census:
  `scratch/encounters_initialize_cluster_pooled_owner_census_20260905.py`
  (SHA256 `861172ED6A690E9D54BCF51A4383DD31B98F7214865497BECC8D0BD3506C5BCB`)
  and `.json` (SHA256
  `DC1138B4F562EDBDB160DAF0E293AC3FE949B3AE967CEFA601FBC7847688A6E3`);
- ordinary isolated report:
  `scratch/encounters-initialize-cluster-isolated-report-20260905/report.json`
  (SHA256
  `39BDFD530EA9B8A2B404566D87AD8F6C3771029BEDAC776C39BF87D90C869844`).
- independent full-section metadata cross-check:
  `scratch/encounters_initialize_cluster_first_candidate_metadata_audit_support_20260905.py`
  (SHA256
  `607CE3BF9CC690E0344F3F7971482D760BDE10025A7B9706945BE3FFA4FC2AFA`)
  and
  `scratch/encounters-initialize-cluster-first-metadata-audit-support-20260905.json`
  (SHA256
  `C11EE808DA9BEBEB6E1A8D484C4E4D8446FAF984607E626DD98E30DE757021B3`).

This ledger approves the frozen three-function packet as authentic functional
progress only.  The remaining 11 unwritten owners, inherited target-absent
owners/COMMON topology, and constructor residual keep the full Encounters
object outside Matching admission.
