# `ai_script.obj` vehicle-settings/status integration (2026-08-29)

This additive ledger records the bounded integration of the previously proven
AI Script vehicle-enterable setting and actor/platoon status cluster.  All six
functions are strict exact on the current canonical tree and preserve all 26
inherited exact functions.  `ai_script.obj` remains `NonMatching`; no
whole-object or data completion is claimed.

## Scope and provenance

- Integration base: local committed checkpoint
  `d83b4ba9a6a07dc8306202896eaef64d7204d635`, itself based on canonical
  `69724391f3a9a33020eb01d0e69c98eb1d02e38b` through the three preceding
  additive AI Script integrations.
- Committed donor checkpoint:
  `5a7285136b4fadb71c01019ede8e2a3858d4e0ef` in
  `work/ai-script-full-20260813`.
- Donor authority ledger:
  `docs/object_matching_logs/ai_script_obj.md` at that checkpoint.
- January split object SHA-256:
  `1a04c31fe9b02c1bdea6d15b2210d4f75493f51ea610cce5742c053bae3eca2e`.
- Pinned XDK 3911 VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Production flags remain `/O2 /Oy- /DDEBUG /Dxbox` through the ordinary
  generated Ninja edge.

Only the source blob committed at the donor checkpoint was transferred.  The
donor worktree's later dirty files were excluded.  The current and donor object
ledgers, tree checkpoint, Claude guidance, policy, and campaign tooling
documentation were reviewed before mutation.

January COFF remains the byte, padding, relocation, linkage, and ownership
authority.  The implementation change is confined to `source/ai/ai_script.c`.
AI Script-owned vehicle/platoon iterator types and APIs are declared in
`source/ai/ai_script.h`; focused owner headers declare the actor iterator and
actor-looking APIs, and their implementation files include those headers.
This additive ledger is the only other tracked file.  No configuration,
semantic allowlist, parked manifest, comparator, compiler flag, or admission
rule changed.

## Validated baseline and strict delta

At the integration base, the object had 26/116 strict functions, 2,043/19,281
meaningful code bytes, and 2,240/20,112 padded code bytes.  The complete
26-function exact set was retained.

The committed donor cluster reproduced on its first and only natural
implementation compile.  The object advances to 32/116 strict functions,
2,499/19,281 meaningful code bytes, and 2,736/20,112 padded code bytes.  The
gain is six functions, 456 meaningful bytes, 496 padded bytes, and 27
relocations.  Direct hardened comparison reports all 32 current exact
functions `equal: true`.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ai_scripting_vehicle_enterable_distance` | 62 | 64 | 5 | `5a3592801a18441f038566ba4ef4d02882012365835df673eb342d5d65a7067a` |
| `_ai_scripting_vehicle_enterable_team` | 78 | 80 | 5 | `80f33a2645d9f955bd83efb4a44092be1a83a51652474fffd0f17a94bee5aaf4` |
| `_ai_scripting_vehicle_enterable_actor_type` | 78 | 80 | 5 | `9cee5b4eaa735b47bd0c6626456dc47a8c8e24ee92ddc1a6e35a8e0928fd4f53` |
| `_ai_scripting_stop_looking` | 74 | 80 | 6 | `213c88cf3035ca08e4534507db9afe5fa02a7de58adcefb43ad05f78be415989` |
| `_ai_scripting_is_attacking` | 83 | 96 | 3 | `f59f3fecb24f71dc77f750aa832494717c14e9d4e4a8d91576805d72e7b88041` |
| `_ai_scripting_going_to_vehicle` | 81 | 96 | 3 | `4e803b197abd37ed416c007c7a6c7f6f0ad155996c0e8c313e5de1f9ad83d286` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 84 nonexact functions.

## Accepted source and declaration controls

The team and actor-type setters preserve the script ABI's full-width `long`
formals and narrow once into semantic `short` locals.  The distance setter and
both bitmask setters use the typed `ai_script_vehicle_enterable` fields.  That
structure's 0xC size and radius/team/actor-type offsets are compile-time
checked.  No raw object or tag access is used.

`ai_scripting_is_attacking` keeps the donor-proven result local and `break`
topology; direct early returns are a known 80-byte rejection against the
96-byte January packet.  Its 0xC platoon iterator is fully named and checked.
`ai_scripting_going_to_vehicle` uses the fully typed 0x1C actor iterator and
canonical `actor_datum` action/vehicle fields.  The iterator is not an opaque
byte buffer.  Stop-looking resolves the typed unit actor field and calls the
owner-declared `long actor_index` API; it does not retain the donor's stale
local `word` prototype.

The two focused new headers use include guards without pragmas.  AI Script,
`encounters.c`, and `actor_looking.c` consume the same owner declarations, so
there are no local prototype shadows.  Header inclusion is acyclic and the
dependent rebuild is warning-free.

The source is readable C89: declarations begin their block scopes, parameters
are vertical, and void functions return explicitly.  The transfer adds no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw offset/address access, pointer/integer reconstruction, aliasing
pun, undefined behavior, byte forcing, object patch, or tool exception.

## Integration record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| D-A05 | Donor vehicle-enterable setter reconstruction. | Short team/type script formals shifted a relocation.  Full-width formals with one semantic local narrowing made all three setters exact. | Transfer only the proven ABI/local split. |
| D-A07 | Donor status reconstruction. | Direct returns made `is_attacking` 80 bytes; result-local plus `break` made its 96-byte packet exact.  Typed actor-state topology made `going_to_vehicle` exact. | Transfer only the proven result-local topology and typed state access. |
| I04 | Transfer the six authenticated bodies with canonical named debug/structure fields and typed iterators. | Warning-free first implementation compile; six new strict packets exact and all 26 inherited packets preserved. | Function implementation accepted without retry. |
| I04h | Initially place the actor iterator/looking declarations in broad `encounters.h` and `actors.h`. | The 45-TU header blast changed unrelated VC7 emission: semantic/accepted totals reached only 4,792/4,802, strict progress fell to 603,478 bytes, and the protected Units sentinel changed to `e7d266cb3735b0f1da7c23f1413c70e558660fa3bd45a4a1b6e23981ae36d799`. | Rejected before commit; broad headers restored byte-for-byte.  Do not repeat. |
| I04f | Move those exact declarations into focused owner `actor_iterators.h` and `actor_looking.h`, included only by the consumers and definitions. | Final dependent rebuild touches only `ai_script.obj`, `encounters.obj`, and `actor_looking.obj`; semantic/accepted totals rise by exactly six, all 32 AI Script packets stay exact, and Units returns to its protected hash. | Final declaration ownership. |

The body compile was not retried or tuned.  The later compilations only audited
and corrected declaration placement after the required header blast-radius
gate exposed an unrelated regression.  No function spelling sweep, scheduling
experiment, compiler-control attempt, or optimizer workaround was performed.
Do not shorten the setter script ABI, remove the team/type semantic locals,
replace the result-local `break` topology, restore opaque iterator storage,
restore the stale `word` prototype, or move the focused declarations into the
broad AI headers without new authenticated evidence.

## Residual classification and reopen criteria

The remaining 84 functions are outside this bounded integration: 83 are still
unwritten and `encounter_get_platoon` remains the known header-inline provenance
residual.  The absent one-byte separator remains a data-provenance task.  Reopen
this cluster only if a future header or topology change loses one of the six
strict packets, or if January-era source supplies contradictory typed semantics.
Any resumed AI Script wave must recheck all 32 accepted functions as a set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `6f2642272e9988d2172228b5ffbde356f9a34d7e52144e2946007c4354876054`.
- Integrated `ai_script.h` SHA-256:
  `b879667ba462b65ea2a3feefc5ec5088cec3627e464643161059a84861256934`.
- Focused `actor_iterators.h` SHA-256:
  `73677245b5441970b821b2d267a774bfe17caa556fae9c4a46953027393fdf8e`.
- Focused `actor_looking.h` SHA-256:
  `ba9177131d8190dcbb7d52c1608fbba7869f2fe69d07763ac38b3e3785d0ed1e`.
- Integrated production object SHA-256:
  `d564a0400d7ed727017b441a49a9bcafe139b820fd3c5c8a4255839f14313e71`.
- `build/report.json` SHA-256:
  `7dd3f98ae948e616363bb76c19b1cb18507a38ab01e0ccc4db6f6cc0ee8e6e4e`.
- `build/semantic_report.json` SHA-256:
  `0de9d9c1bb6eeeacccad51a376af409f9d741d041b7503bee1a4b1ffb52f2223`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,910 functions evaluated, 4,793 semantic exact,
  4,803 accepted exact, and zero unit errors.  Both exact totals are exactly six
  above the validated base.
- Campaign progress: 384/833 linked objects, 4,764/11,060 strict functions,
  and 605,397/2,198,102 strict code bytes.
- Halobetacache progress: 282/468 linked objects, 4,597/7,574 strict
  functions, and 592,483/1,770,166 strict code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 32-function comparison reports `all_equal: true`, 2,736 padded
  bytes, and 146 relocations; added-line policy scan and `git diff --check`
  are clean.

## Disposition

The vehicle-settings/status cluster is integrated and eligible for partial
strict function credit: 456 meaningful and 496 padded bytes.  The translation
unit correctly remains `NonMatching`; no data or whole-object credit is
granted.
