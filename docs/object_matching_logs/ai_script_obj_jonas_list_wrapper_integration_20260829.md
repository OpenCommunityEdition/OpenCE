# `ai_script.obj` list-wrapper integration (2026-08-29)

This additive ledger records the bounded integration of the two previously
proven AI Script object-list wrappers.  Both functions are strict exact on the
current canonical tree and preserve all 32 inherited exact functions.
`ai_script.obj` remains `NonMatching`; no whole-object or data completion is
claimed.

## Scope and provenance

- Integration base: local committed checkpoint
  `dbb20ef6d3536fe2f8ea824e1572daed68d4bebe`, itself based on canonical
  `69724391f3a9a33020eb01d0e69c98eb1d02e38b` through the four preceding
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

Only the source blob committed at the donor checkpoint was transferred; the
donor worktree's later dirty files were excluded.  January COFF remains the
byte, padding, relocation, linkage, and ownership authority.

The implementation change is confined to `source/ai/ai_script.c`.
`source/ai/ai_script.h` now owns the two single-unit callee declarations, and
AI Script consumes the current typed `hs/object_lists.h` API.  This additive
ledger is the only other tracked file.  No configuration, semantic allowlist,
parked manifest, comparator, compiler flag, or admission rule changed.

## Validated baseline and strict delta

At the integration base, the object had 32/116 strict functions, 2,499/19,281
meaningful code bytes, and 2,736/20,112 padded code bytes.  The complete
32-function exact set was retained.

The committed donor cluster reproduced on its first and only natural compile.
The object advances to 34/116 strict functions, 2,625/19,281 meaningful code
bytes, and 2,864/20,112 padded code bytes.  The gain is two functions, 126
meaningful bytes, 128 padded bytes, and six relocations.  Direct hardened
comparison reports all 34 current exact functions `equal: true`.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ai_scripting_attach_units` | 63 | 64 | 3 | `3f6dc337cf204f38056dc6ac8bd98a89cd77c54c681f740c4eb3e64f1084b253` |
| `_ai_scripting_magically_see_units` | 63 | 64 | 3 | `f13c755da0cd5eead408d08e628ec1fed6f05d60e2227d3b2f8acff858946404` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 82 nonexact functions.

## Accepted source controls

Each wrapper obtains the first typed object-list element and reference cursor,
calls its authenticated single-unit AI Script operation, then advances through
`object_list_get_next` until `NONE`.  The argument order differs intentionally:
attach passes `(unit_index, ai_reference)`, while magically-see passes
`(ai_reference, unit_index)`, exactly as proven by the committed donor and the
January relocation/code packets.

The list API comes from its owner `hs/object_lists.h`.  The two single-unit
callee types are declared once in `ai_script.h`; no local prototype shadow,
raw list access, object/tag offset, or untyped reconstruction is present.  The
source is readable C89 with declarations at block entry, vertical parameters,
and explicit void returns.  The transfer adds no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, raw address
access, pointer/integer reconstruction, aliasing pun, undefined behavior, byte
forcing, object patch, or tool exception.

## Integration record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| D-A06 | Committed donor list-wrapper topology using the object-list cursor APIs. | Both wrappers reproduced January on their first strict donor build. | Authenticated donor form. |
| I05 | Transfer only those two bodies, include `hs/object_lists.h`, and move the single-unit declarations into the AI Script owner header. | Warning-free first compile; two new strict packets exact and all 32 inherited packets preserved. | Final integrated form. |

No retry, spelling sweep, scheduling experiment, compiler-control attempt, or
optimizer workaround was performed.  Do not reverse either single-unit call's
argument order, replace the owner list API with raw datum access, hoist or
reconstruct the reference cursor, or add local callee shadows without new
authenticated source evidence.

## Header blast radius and residuals

The owner-header declarations caused only `debug_keys.obj` to rebuild in the
full dependency graph after the already validated AI Script compile.  The
rebuild was warning-free; global semantic and accepted-exact totals each rose
by exactly two and no accepted packet was lost.

The remaining 82 functions are outside this bounded integration: 81 are still
unwritten and `encounter_get_platoon` remains the known header-inline provenance
residual.  The absent one-byte separator remains a data-provenance task.  Any
resumed AI Script wave must recheck all 34 accepted functions as a set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `c2e5ddc2cf52dbf98fa26914e9724fe0b657908358c79cd6e6b4eea992b8b129`.
- Integrated `ai_script.h` SHA-256:
  `aec72fe664f302fe7477446181f36588eae86234c5631aa6e0b8cc1b498cf5d2`.
- Integrated production object SHA-256:
  `cecbe9073b9b14f7411622938fc695297ed03ffa09ff1fd09e901f7ba56388bd`.
- `build/report.json` SHA-256:
  `2e0b26832cc2252efdf498be867d3e328f589c32463fa984a46834d33fd0e35c`.
- `build/semantic_report.json` SHA-256:
  `16d2efe9b06d69d9561ab801ec6762caa32af34956a281e71b7437775bbc9ef6`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,912 functions evaluated, 4,795 semantic exact,
  4,805 accepted exact, and zero unit errors.
- Campaign progress: 384/833 linked objects, 4,766/11,060 strict functions,
  and 605,523/2,198,102 strict code bytes.
- Halobetacache progress: 282/468 linked objects, 4,599/7,574 strict
  functions, and 592,609/1,770,166 strict code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 34-function comparison reports `all_equal: true`, 2,864 padded
  bytes, and 152 relocations; added-line policy scan and `git diff --check`
  are clean.

## Disposition

The two list wrappers are integrated and eligible for partial strict function
credit: 126 meaningful and 128 padded bytes.  The translation unit correctly
remains `NonMatching`; no data or whole-object credit is granted.
