# `ai_script.obj` allegiance/setter integration (2026-08-29)

This additive ledger records the bounded integration of the previously proven
AI Script allegiance-removal and encounter-flag setter cluster.  All four
functions are strict exact on the current canonical tree and preserve all 22
inherited exact functions.  `ai_script.obj` remains `NonMatching`; no
whole-object or data completion is claimed.

## Scope and provenance

- Integration base: local committed checkpoint
  `2373e84d17fe4905c1e02ce9c22d0e42a087a6e6`, itself based on canonical
  `69724391f3a9a33020eb01d0e69c98eb1d02e38b` through the two preceding
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
Declaration-only corrections were made in the owning `source/ai/encounters.h`
and `source/game/game_allegiance.h` headers so the integration adds no local
prototype shadows.  This additive ledger is the only other tracked file.  No
configuration, semantic allowlist, parked manifest, comparator, compiler flag,
or admission rule changed.

## Validated baseline and strict delta

At the integration base, the object had 22/116 strict functions, 1,599/19,281
meaningful code bytes, and 1,776/20,112 padded code bytes.  The complete
22-function exact set was retained.

The committed donor cluster reproduced on its first and only natural
production compile.  The object advances to 26/116 strict functions,
2,043/19,281 meaningful code bytes, and 2,240/20,112 padded code bytes.  The
gain is four functions, 444 meaningful bytes, 464 padded bytes, and 32
relocations.  Direct hardened comparison reports all 26 current exact
functions `equal: true`.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ai_scripting_allegiance_remove` | 75 | 80 | 5 | `e0f3b464ae690fde9a084704bcdd05400728a95cb72df7eaae4810c17e215523` |
| `_ai_scripting_set_respawn` | 123 | 128 | 9 | `fb4c383478f9b0b8773768ee8086682e65ce9cc09501db74adc686d4591e60fc` |
| `_ai_scripting_set_deaf` | 123 | 128 | 9 | `fb4c383478f9b0b8773768ee8086682e65ce9cc09501db74adc686d4591e60fc` |
| `_ai_scripting_set_blind` | 123 | 128 | 9 | `fb4c383478f9b0b8773768ee8086682e65ce9cc09501db74adc686d4591e60fc` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 90 nonexact functions.

## Accepted source and declaration controls

The three encounter setters preserve the script ABI's full-width `long`
packed AI reference.  They narrow only at the typed encounter setter call with
`ai_reference & UNSIGNED_SHORT_MAX`; the earlier donor experiment using a
`short` script formal is not retained.  Each wrapper uses the authenticated
debug string and the canonical named `ai_debug.print_scripting` field.
Allegiance removal retains its proven two-short script ABI and calls the typed
game allegiance API only when both values are valid.

The donor declared `game_allegiance_remove` locally as returning `void`, but
the current definition returns `boolean`.  The owning
`game/game_allegiance.h` now declares the actual `boolean (short, short)` type;
AI Script includes that header and intentionally ignores the result.  This
removes a conflicting prototype without changing the exact emitted packet.
The three encounter setter declarations likewise live in `ai/encounters.h`.
No file-local shadows remain.

The source is readable C89: declarations begin their block scopes, parameters
are vertical, and void functions return explicitly.  The transfer adds no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw offset/address access, pointer/integer reconstruction, aliasing
pun, undefined behavior, byte forcing, object patch, or tool exception.

## Integration record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| D-A03 | Committed donor allegiance reconstruction. | `ai_scripting_allegiance_remove` was strict exact; the separate `allegiance_broken` variants were not. | Transfer removal only. |
| D-A04 | Committed donor encounter setter reconstruction. | `short` AI-reference form misplaced the final relocation; full-width `long` form narrowed at the encounter call and made all three setters exact. | Transfer only the full-width form. |
| I03 | Transfer the four authenticated bodies, use `ai_debug.print_scripting`, and reconcile callees into owning headers with their actual types. | Warning-free first compile; four new strict packets exact and all 22 inherited packets preserved. | Final integrated form. |

No retry, spelling sweep, scheduling experiment, compiler-control attempt, or
optimizer workaround was performed.  Do not shorten the encounter wrappers'
script ABI, move narrowing to function entry, restore the donor's incorrect
void declaration, shadow an owning header, or restore the anonymous debug
array/index spelling without new authenticated source evidence.

## Full rebuild blast radius

The declaration-only owner-header changes caused the ordinary full graph to
recompile `game_allegiance.obj`, `ai_profile.obj`, `encounters.obj`,
`game_statistics.obj`, `actor_perception.obj`, `actions.obj`, and
`ai_debug.obj`.  The build was warning-free.  Global semantic exact and
accepted-exact totals each increased by exactly four relative to the validated
base, with zero unit errors; therefore the header blast radius lost no accepted
packet.  Direct post-rebuild comparison also reconfirmed the complete
26-function AI Script set.

## Residual classification and reopen criteria

The remaining 90 functions are outside this bounded integration: 89 are still
unwritten and `encounter_get_platoon` remains the known header-inline provenance
residual.  The absent one-byte separator remains a data-provenance task.  Reopen
this cluster only if a future header or topology change loses one of the four
strict packets, or if January-era source supplies contradictory typed semantics.
Any resumed AI Script wave must recheck all 26 accepted functions as a set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `1d808b5fa49d67f5accdf119d049999274ecd09e4462f06660c1a69e697ce3e4`.
- Integrated `encounters.h` SHA-256:
  `a1614c57d91b46571c98bc3eaf5c3708ce1a80c78ca510f3103d48f923142213`.
- Integrated `game_allegiance.h` SHA-256:
  `9633e5ea0b7cd5ff36d84902b0588080fa73ab162ade90ddcd4dded452a6131b`.
- Integrated production object SHA-256:
  `e488f725e798b8f7003576c1d06c50c06474d93884352e5b57f6a99ce5bf1349`.
- `build/report.json` SHA-256:
  `ea08de80684a43ead2a8248637e6a95e049c19faae0970e012280f00dc41b46d`.
- `build/semantic_report.json` SHA-256:
  `06bfc3334ceb7a7ecc2eea6a7b566996eecb4f5215322252bb35d0139c736387`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,904 functions evaluated, 4,787 semantic exact,
  4,797 accepted exact, and zero unit errors.
- Campaign progress: 384/833 linked objects, 4,758/11,060 strict functions,
  and 604,941/2,198,102 strict code bytes.
- Halobetacache progress: 282/468 linked objects, 4,591/7,574 strict
  functions, and 592,027/1,770,166 strict code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 26-function comparison reports `all_equal: true`, 2,240 padded
  bytes, and 119 relocations; added-line policy scan and `git diff --check`
  are clean.

## Disposition

The allegiance/setter cluster is integrated and eligible for partial strict
function credit: 444 meaningful and 464 padded bytes.  The translation unit
correctly remains `NonMatching`; no data or whole-object credit is granted.
