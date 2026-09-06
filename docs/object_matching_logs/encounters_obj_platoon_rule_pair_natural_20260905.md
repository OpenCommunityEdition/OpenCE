# Encounters: natural platoon-rule caller pair (2026-09-05)

## Packet and current disposition

Base publication: `4f90c05c54c78196ce188be3f4439cfdfb30d639`.
Canonical root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

This packet reconstructs the genuine private `encounter_update_platoons` /
`encounter_test_rule` pair, called by the already retained public
`encounters_update`. The first natural C candidate makes update-platoons
strict (315 meaningful / 320 padded bytes). Test-rule is complete natural
source at 91.02511% (656 padded bytes, 45 relocations versus January46).
No alternative source spelling, ordering, declaration, compiler flag, ABI,
inline request, anchor, alias, assembly, or forced helper emission was tried.

The full build measures 907,050 meaningful matched code bytes,
6,174 credited functions, 2,031,312 matched data bytes and 391/833 Matching
objects. The stable sweep is 6,220/8,245, with one gain and zero regressions.
These are the verified publication-batch totals after final admission below.
No whole-object completion or extra data credit is claimed.

Active Fable lane contents and reserved implementations were not inspected or
edited. The genuine shared schema affects canonical consumers including
ai_script; later Fable reconciliation must account for this schema rather than
assuming its independent build is unaffected. MatrixMath, Vehicles and
AI Debug implementations remain excluded.

## Primary semantics and owner schema

Full source-only triage and review:
`scratch/encounters_platoon_rule_pair_triage_20260905.md` and
`scratch/encounters_platoon_rule_pair_source_review_20260905.md`.
Root read both completely, independently dumped both full January functions,
and directly queried HCEX PDB for both structures and all ten rule constants.

January target:
`build/split/source/ai/encounters.obj`, SHA256
`7edc1863eb2c08f7f2ad612e3cfa7b246c0eb9a3b5907fd08c2c17ecd9a056a3`.
PDB/tool locations and hashes are in the preceding
`encounters_obj_platoon_schema_starting_location_bounds_20260905.md`.

The owning ai_scenario_definitions.h now declares:

- Original rule constants never through all_dead, values0..9.
- `platoon_rule`: signed-short rule_type and platoon_index, long pad; size8.
- Original unused1[3], attacking_defending_rule+0x30, unused2 and
  maneuvering_rule+0x3C in platoon_definition, size0xAC, alignment4.
- Remaining unrelated bytes stay opaque. No later unused tag blocks imported.
- Natural size/offset assertions document the authenticated layout.

The source includes the genuine main/console.h owner, correcting its previous
implicit console calls without changing that header. Both recovered functions
have ordinary static own-C prototypes and full long datum handles. The only
three target incoming edges are public update to platoon update, and the latter's
two rule calls. January's private EAX/EDI rule ABI emerges naturally; there is
no custom calling convention or forced export.

Typed accessors and TAG_BLOCK_GET_ELEMENT recover actual fields. Only scenario
lookup extracts the low-word index; datum access and both rule calls retain
salted full handles. Signed-short counts promote naturally for multiplication
and division, whose range stays within target32-bit int. Three real strength
tests use strict less-than; invalid platoon selectors use encounter fallback;
nonpositive original count, never and unknown rules return FALSE. All nine
diagnostic rule formats and the four transition strings are retained exactly,
including original potentially misleading dead-count percent wording.
Maneuver/defending gates preserve short-circuit and store order, using
!TEST_FLAG on the named initially-defending bit.

## One genuine header/include trial

Capture script:
`scratch/encounters_platoon_schema_capture_20260905.py`.
Before and after inventories:
`scratch/encounters-platoon-schema-before-20260905.json`,
`scratch/encounters-platoon-schema-after-20260905.json`.

Direct includes and actual Ninja dependencies independently identify13 TUs:
action_alert, action_fight, action_search, action_uncover,
actor_firing_position, actors, ai_communication, ai_debug,
ai_scenario_definitions, ai_script, encounters, players, hs.

Header SHA256:
`5424e4e887430e9def8e23dde5b1f7c02da0662a11f31a5bd80a40e1fd460618`
before,
`52e4b251e63cb58d9c8dcf5bc86ebeba1852f473cecebea06354fda1864783c5`
after.

Full Ninja preserves 6,219 strict owners and all282 parks before body
integration. The independent all-section audit, rerun by root through
fingerprint/compare_unit for every frozen pair, passes all13:
1,890 sections, 1,883 unchanged and seven compiler-local-label-only;
5,082 symbol records each, normalized symbol delta0/0;
911 defined functions and15 COMMON records each; no state transitions,
new/lost runtime/debug owners, or new point helper.
The inherited AI Debug point helper remains unchanged, not newly permitted.
No earlier speech-timer or initializer exception is applied here.

Durable independent ledger:
`docs/object_matching_logs/encounters_obj_platoon_rule_schema_header_audit_20260905.md`.
Report:
`scratch/encounters-platoon-rule-schema-runtime-audit-support-20260905/summary.json`,
SHA256 `afe707f4ca30e8cfb9167c63ee6947dddf47b49951c2ce419864219b40cb16a94`.

## Frozen first candidate and actual canonical build

Source fragment:
`scratch/encounters_platoon_rule_pair_natural_20260905.inc`,
SHA256 `9b12742c28394a288b566013e1d0849d46aba1de3fd47d2f0994eb4929463a9a`.
Manifest:
`scratch/encounters_platoon_rule_pair_production_20260905.manifest.json`,
SHA256 `b0f93089239b53adc97cdb4949c29a643d5d2b12838245ca17322784921aed91`.
Both unique anchors were checked by root before the sole body trial.

PID-scratch candidate:
`scratch/encounters-platoon-pair-first-natural-20260905.obj`,
SHA256 `5748d27e063eec544dcad4ce834a088e14272458eee5d02d42083d6efda67f9f`.
Actual canonical object:
`scratch/encounters-platoon-pair-canonical-first-20260905.obj`,
SHA256 `44b52909171e2ed014c4bfc68f9dd1283a8db4b3f1cf17f92c5009e4f774a8f8`.
Actual source SHA256:
`73fc683ec106d2d1aebf3c58481ac7ff5d8baf97c7848b390d4848a3aed1f9d9`.

Candidate-to-actual comparison has132 unchanged sections; only .debug$S
changes185 to197 bytes because the recorded output object path changes from
scratch/_gate_13576.obj to build/base/source/ai/encounters.obj. Root read the
actual debug bytes; this is not a source-path or runtime discrepancy.
Report `scratch/encounters-platoon-pair-candidate-canonical-audit-20260905.json`,
SHA256 `dfaff914436ad8b04f0f8c3b2b6e7ef762639924d9e1fe7a86ba9968e28dddde`.

The all-section baseline-to-actual report is
`scratch/encounters-platoon-pair-canonical-all-section-20260905.json`,
SHA256 `ee6568f30d7ed003010d24c6f4fc748b84b96e306cb8b98508e6cce0eed3a518`.
There are113 unchanged old sections, one encounter_create label-only change,
and one real metadata transition in encounters_update's call target:
undefined external declaration to the genuine static definition.
The public caller remains strictly byte/relocation-identical against January.
No old runtime owner or COMMON is lost. The only added sections are the two
intended code owners,13 genuine strings and three real constants; no new
point_from_line3d or unrelated code helper appears.
New owner/pooled-constant checks are separate below, not waived by this list.

## Complete residual accounting, no source tuning

Target test-rule hash:
`ba62edfcd33544627a6b126232380c7f9da859156bbd098cc6425a6e86825c57`.
Candidate:
`98d8b09149975b27ae9c9fe9f4c0bc9f839a98a76de5b000907b095e0b3b9813`.

Root independently proves:

- Relocation-masked prefix0..0x42 is identical.
- Fallback0x43..0x50 reads the same three fields and spills the same real;
  natural load order/register choice differs.
- Target dispatch tests unsigned rule0..9 with ten entries. Candidate
  decrements and checks unsigned0..8 with nine entries for rules1..9.
  Never/unknown still reach the common false path.
- All470 instruction bytes target0x6D..0x242 equal candidate0x6E..0x243
  after relocation masking.
- The nine meaningful rule destinations and all nine diagnostic destinations
  agree after that one-byte offset shift. Target's tenth entry only sends
  never to the false path. Candidate omits that one internal DIR32 relocation.
- Table2 moves from0x26C to0x268; alignment padding accounts for equal656-byte
  COMDAT sizes. Neither table is silently treated as free-standing data.

Reproducer `python -B -m scratch.encounters_platoon_rule_aligned_residual_20260905`
writes `scratch/encounters-platoon-rule-aligned-residual-20260905.json`,
SHA256 `3be91167dbf242c427bb0a8b55744607ca2e7123fa5bc2565d53be15f809ef5a`.
This is a semantic diagnostic, not masked-byte exact credit. The rule receives
zero exact credit, and no equivalent spelling is tried to force a redundant
table entry or a register allocation.

## Runtime, new-owner and final admission

The January target, first candidate and actual canonical object each pass
111/111 independent runtime cases. Only the real update-platoons entry is
called; its real static rule callee and genuine platoon accessor execute.
There is no imposed private register ABI. The harness checks full handles,
tag/API call order, signed selectors/counts, all rule cases and unknown cases,
real threshold equality, infinities and quiet/signaling NaNs under masked x87
exceptions, all nine diagnostic formats and promoted arguments, maneuver and
defending changes, disabled/empty/nonliving/mixed platoons, full state images,
guards and callee-saved registers. Root read the complete harness and reran
the actual canonical differential: all111 pass with no event/memory mismatch.
The root result exactly reproduces the independent report hash
`3200cae8cb70c583ad916bb9b18d784c699e6ab7b1e1552c57c622f0f29b56cd`.
See `encounters_obj_platoon_rule_pair_runtime_review_20260905.md`.
Malformed external handles/tags, huge out-of-domain counts, console rendering,
unmasked FP exceptions and full-game integration are not claimed tested.

Root also read and reran the complete new-owner audit against the actual
canonical object. All833 target objects and572 built canonical objects were
censused for the16 new data names. The13 strings (298 physical bytes) are
owned by January Encounters; three real constants add12 physical bytes.
January selects0.25/0.5 from ActionAlert and0.75 from AI Debug. Every payload,
alignment/flags, symbol type/storage/value and SELECT_ANY choice is compatible.
All93 canonical physical data definitions are identical for each name,
with zero incompatible or NODUPLICATES owners. Counts including Encounters
are16 copies of0.25,60 of0.5,4 of0.75 and13 unique strings.
This genuine compatible duplicate data is not an extra exact-code helper;
no new ordinary-link duplicate failure is indicated. No full-game link is
claimed by the census.

Root report:
`scratch/encounters-platoon-pair-root-canonical-new-owner-audit-20260905.json`,
SHA256 `e0d1e0d0581f65044b64b7b54143f57883fbcf496a60675ce501eb5e2dbaefc5`.
The two tables stay in the private code owner, with21 target internal
relocations versus20 candidate and25 genuine external relocations each.
Every table destination is a valid instruction boundary; all rule/debug
destinations have separately been compared by root's aligned proof.

The packet is accepted at function level: Encounters advances45E/5R/11U
to46E/6R/9U. The strict update adds315 meaningful code bytes and one credited
function. The natural rule residual is parked at91.02511%, not byte-matched.
The310 physical literal bytes give zero additional aggregate data credit.
Whole-object Matching remains391/833; no target-only or inherited extra-owner
debt is hidden.

Final broader checks pass: Ninja, 288 tool tests plus26 subtests,
6,220/8,245 stable owners with zero losses,283 valid parks,
admission0 candidates/0 contradictions/5 recorded rejections/0 revocations,
two-file fake-match scan with zero review leads, and clean diff checking.
Bitmap source/header hashes remain exactly unchanged from the preceding
published checkpoint. No admission label, matcher exception or compiler flag
has changed.
