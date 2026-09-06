# Encounters platoon/rule pair — independent runtime differential review

## Verdict

**PASS for the frozen function-level packet.**  The January target, the first
natural candidate, and the actual canonical first-build object agree with an
independent readable model and with each other in all 111 cases.  This review
does not grant byte-exact credit to the residual rule evaluator and does not
admit the whole Encounters object as Matching.

No source, header, configuration, park, build, or production file was changed.
No source variant was compiled or suggested from harness output.

## Frozen inputs and results

| role | path | SHA256 |
| --- | --- | --- |
| January target | `build/split/source/ai/encounters.obj` | `7EDC1863EB2C08F7F2AD612E3CFA7B246C0EB9A3B5907FD08C2C17ECD9A056A3` |
| first natural candidate | `scratch/encounters-platoon-pair-first-natural-20260905.obj` | `5748D27E063EEC544DCAD4CE834A088E14272458EEE5D02D42083D6EFDA67F9F` |
| actual canonical first build | `scratch/encounters-platoon-pair-canonical-first-20260905.obj` | `44B52909171E2ED014C4BFC68F9DD1283A8DB4B3F1CF17F92C5009E4F774A8F8` |
| harness | `scratch/encounters_platoon_rule_pair_behavior_20260905.py` | `497E978676457B296BEE9FAF7B44D65B5CF6C2B56CB5B920AB4143ECAEAD28FE` |
| target-only baseline | `scratch/encounters-platoon-rule-target-baseline-20260905.json` | `C215FEB14BDD4CA3ABCBFC29008A87EFEB1FF0BCD0D617E2241747C327FA5271` |
| first-natural differential | `scratch/encounters-platoon-rule-first-natural-differential-20260905.json` | `C20A1B710183E4AC075C643D6F640674BAB96A631DD85C4BF13010CE32F7EABD` |
| canonical differential | `scratch/encounters-platoon-rule-canonical-differential-20260905.json` | `3200CAE8CB70C583AD916BB9B18D784C699E6AB7B1E1552C57C622F0F29B56CD` |

Each report records 111 cases, zero model failures, zero target/candidate
event differences, and zero target/candidate memory differences.

## ABI and execution boundary

Every case enters only `_encounter_update_platoons` through its ordinary
stack argument.  Its real static call to `_encounter_test_rule` executes from
the tested object.  The harness therefore neither calls that private helper
directly nor assumes its January EAX/EDI register schedule is a source ABI.

The genuine `_encounter_get_platoon` machine-code accessor also executes.  The
January target uses the selected copy from
`build/split/source/ai/ai_script.obj` (SHA256
`418DEE372C43EC72C398032311F1DD94A3ABCB90806860249B3B59BFC29C14F1`);
the candidates use their naturally available owner.  Only true outside
boundaries are modeled: `datum_get`, `global_scenario_get`,
`tag_block_get_element_with_size`, `console_printf`, and the unreachable
assert/exit endpoints.

All datum lookups are checked to receive the complete 32-bit salted encounter
handle.  Four explicit salts straddle signed boundaries (`0001:0000`,
`7FFF:0001`, `8000:0003`, and `FFFF:0007`), while scenario lookup is verified
to use the low-word absolute index.  Neither candidate triggers an accessor
assertion or reads/writes outside the modeled runtime/tag regions.  Callee-
saved registers, return stack, encounter data, tag definitions, and guard
bytes are preserved.

## Rule semantics covered

The independent model is based on the complete January instruction streams,
not donor output.  The executed matrix covers:

- all rule values 0 through 9 and unknown signed values -32768, -1, 10, and
  32767;
- valid self/other platoon selection and invalid indices -32768, -1,
  `platoon_count`, and 32767, all falling back to encounter-wide state;
- signed original/current/platoon counts, including -32768, -1, zero, exact
  integer threshold boundaries, and signed-division truncation for the 25%
  rule;
- original count <= 0 for every real rule, which always returns false before
  rule evaluation or diagnostic output;
- each strength threshold immediately below, exactly equal, and immediately
  above 0.75f/0.5f/0.25f, plus both infinities, positive and negative quiet
  NaNs, and a signaling NaN;
- all nine true rule-value diagnostic formats with exact normal C promotions
  (float to double and signed short to int), and every combination of the two
  debug flags;
- maneuver false/true transitions, maneuver-disable behavior, active
  maneuver defense suppression, already-desired defense, failed and passed
  defense rules, attacking and defending transitions, nonliving platoon
  skips, signed-empty lists, and ordered mixed-platoon updates.

The measured behavior confirms ordered floating-point comparison: only a
strictly smaller finite/infinite value triggers a strength rule; equality and
every tested NaN are false.  It also confirms the January integer predicates,
including `(current * 4) / 3 <= original` with signed truncation toward zero.
All rule-value diagnostics occur inside the rule helper before the caller's
maneuver/defense transition message.

The target and both candidates produce identical exact format strings and
promoted values, including the original `survivors %d = 0` wording.  The
runtime states agree byte-for-byte across the full platoon-array image rather
than only at the two changed Boolean fields.

## Bounded exclusions

- Invalid datum handles and malformed tag blocks remain outside the genuine
  external API/tag preconditions and were not fabricated.
- `console_printf` itself is stubbed.  The exact format bytes, call order, and
  promoted arguments are checked; terminal rendering is not.
- Huge positive platoon counts beyond configured valid tag/runtime memory are
  excluded.  Negative, zero, and representative multi-element signed counts
  are covered.
- The harness is a semantic/ABI differential only.  Code/data owner matching,
  the evaluator's measured 656-byte/45-relocation residual, jump-table
  ownership, parks, and whole-object admission remain root's separate audit.

Within those boundaries, there is no runtime objection to retaining the
natural `_encounter_test_rule` residual alongside the strict
`_encounter_update_platoons` caller.
