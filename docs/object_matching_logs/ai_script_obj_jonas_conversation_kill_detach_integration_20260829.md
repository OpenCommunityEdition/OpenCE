# `ai_script.obj` conversation/kill/detach integration (2026-08-29)

This additive ledger records the bounded integration of the previously proven
AI Script conversation, detach, and kill cluster.  The seven-function cluster
is strict exact on the current canonical tree and preserves all 15 inherited
exact functions.  `ai_script.obj` remains `NonMatching`; no whole-object or
data completion is claimed.

## Scope and provenance

- Integration base: local committed checkpoint
  `d967ac71` (`Integrate AI script count family`), itself based on canonical
  `69724391f3a9a33020eb01d0e69c98eb1d02e38b`.
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
donor worktree's later dirty files were not consulted as implementation input.
The current and donor object ledgers, tree checkpoint, Claude guidance, policy,
and campaign tooling documentation were reviewed before mutation.

January COFF remains the byte, padding, relocation, linkage, and ownership
authority.  The only tracked implementation change is
`source/ai/ai_script.c`; this additive ledger is the only other tracked file.
No header, configuration, semantic allowlist, parked manifest, comparator,
compiler flag, or admission rule changed.

## Validated baseline and strict delta

At the integration base, the object had 15/116 strict functions, 954/19,281
meaningful code bytes, and 1,088/20,112 padded code bytes.  The entire inherited
15-function exact set was retained.

The transferred cluster advances the object to 22/116 strict functions,
1,599/19,281 meaningful code bytes, and 1,776/20,112 padded code bytes.  The
gain is seven functions, 645 meaningful bytes, 688 padded bytes, and 48
relocations.  Direct hardened comparison reports all 22 current exact
functions `equal: true`.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ai_scripting_conversation` | 98 | 112 | 8 | `2951969824fc2181f04afcdce7dbc47cce700c233474e3e770975f10bb8a2e12` |
| `_ai_scripting_conversation_stop` | 96 | 96 | 8 | `9aeb97d8aabdb7c31c5a81712d05ce4f4f775cedf653693ba020f5fba0c96a68` |
| `_ai_scripting_conversation_advance` | 96 | 96 | 8 | `9aeb97d8aabdb7c31c5a81712d05ce4f4f775cedf653693ba020f5fba0c96a68` |
| `_ai_scripting_detach_unit` | 86 | 96 | 6 | `c4383ff219af02ebfd6d8a01f1915b800f8d4a166d57b5bbe1ec78862e37f850` |
| `_code_000432b0` | 81 | 96 | 4 | `ae9b6281278164855f06f4ecacddff390335fb0368c5200645eb77e03c170e64` |
| `_ai_scripting_kill` | 94 | 96 | 7 | `f95bd2da52e7e2e34f2d8a97c797b92d80a02f7335610587b3f362517f51dde5` |
| `_ai_scripting_kill_silent` | 94 | 96 | 7 | `7632366a59c479c1c81a127087380ddcd22fdc78bdde28346854122332d4664a` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 94 nonexact functions.

## Accepted source controls

The three conversation wrappers narrow the script ABI value at its semantic
use, validate the index against the scenario conversation block, obtain the
typed conversation name, print the authenticated diagnostic when scripting
debugging is enabled, and call the typed conversation API.  Detach resolves a
unit through `unit_get` and deletes its actor when present.  The private static
kill helper uses the typed AI actor iterator and the two public kill wrappers
preserve the authenticated silent flag.

The old checkpoint's anonymous debug-byte expression is reconciled to the
current named `ai_debug.print_scripting` field from `ai/ai_debug.h`.  This
preserves the January `_ai_debug+153` relocation while using the canonical
typed declaration.  The current `ai/ai.h` declaration remains authoritative
for `ai_index_to_string`; no conflicting local declaration is retained.

The source is readable C89: declarations begin their block scopes, parameters
are vertical, the private helper remains `static`, and void functions return
explicitly.  The transfer adds no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, raw offset/address access,
pointer/integer reconstruction, aliasing pun, undefined behavior, byte forcing,
object patch, or tool exception.

## Integration record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| I02 | Transfer the seven-function committed donor cluster and its minimum typed dependencies onto `d967ac71`. | All seven functions were strict exact on the natural production compile; all 15 inherited functions remained exact.  The compiler also identified the donor's redundant local `ai_index_to_string` prototype as conflicting with the current typed header. | Authenticated topology accepted; stale redundant declaration removed. |
| I02a | Rebuild after only removing that redundant prototype. | Warning-free compile; the seven packets and complete 22-function exact set were unchanged. | Final integrated form. |

The second compile was a narrowed typed-header cleanup, not an emission retry
or speculative variant.  No spelling sweep, scheduling experiment,
compiler-control attempt, or optimizer workaround was performed.  Do not
restore the donor's anonymous debug array/index spelling, shadow the canonical
`ai_index_to_string` prototype, remove `static`, or alter iterator topology
without new authenticated source evidence.

## Residual classification and reopen criteria

The remaining 94 functions are outside this bounded integration: 93 are still
unwritten and `encounter_get_platoon` remains the known header-inline provenance
residual.  The absent one-byte separator remains a data-provenance task.  Reopen
this cluster only if a future header or topology change loses one of the seven
strict packets, or if January-era source supplies contradictory typed semantics.
Any resumed AI Script wave must recheck all 22 accepted functions as a set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `d2caf47bac3c857954a8261e5f289c317a9ff9391da0a9d143ff1269829f81a9`.
- Integrated production object SHA-256:
  `77f56ee318a866c8895d5fc0c245de18c29f4caf79c7f1c09d0effaef6306234`.
- `build/report.json` SHA-256:
  `12f6f3d3b7e40f48affd7990da648cb7a8ff013b788202d7b7df9a3aa0f5c709`.
- `build/semantic_report.json` SHA-256:
  `4b8df946cd92ce0235faa945ab8b90c3285b58d464b8e4abf0d6f840a41e44e1`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,900 functions evaluated, 4,783 semantic exact,
  4,793 accepted exact, and zero unit errors.
- Campaign progress: 384/833 linked objects, 4,754/11,060 strict functions,
  and 604,497/2,198,102 strict padded code bytes.
- Halobetacache progress: 282/468 linked objects, 4,587/7,574 strict
  functions, and 591,583 strict padded code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 22-function comparison reports `all_equal: true`; forbidden-source
  scan and `git diff --check` are clean.

## Disposition

The conversation/kill/detach cluster is integrated and eligible for partial
strict function credit: 645 meaningful and 688 padded bytes.  The translation
unit correctly remains `NonMatching`; no data or whole-object credit is
granted.
