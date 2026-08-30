# `ai_script.obj` vehicle-dispatch integration (2026-08-29)

This additive ledger records the bounded integration of the final four
previously proven vehicle-dispatch functions.  All four are strict exact on
the current canonical tree and preserve all 34 inherited exact functions.
`ai_script.obj` remains `NonMatching`; no whole-object or data completion is
claimed.

## Scope and provenance

- Integration base: local committed checkpoint
  `811151c2`, itself based on canonical
  `69724391f3a9a33020eb01d0e69c98eb1d02e38b` through the five preceding
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

The implementation bodies are confined to `source/ai/ai_script.c`.
`source/ai/ai_script.h` now owns the fully named actor-reference iterator and
the public iterator/wrapper declarations.  The focused
`source/units/vehicle_scripting.h` owns the seat-helper declaration and is
consumed by both AI Script and the defining translation unit.  This additive
ledger is the only other tracked file.  No configuration, semantic allowlist,
parked manifest, comparator, compiler flag, or admission rule changed.

## Validated baseline and strict delta

At the integration base, the object had 34/116 strict functions, 2,625/19,281
meaningful code bytes, and 2,864/20,112 padded code bytes.  The complete
34-function exact set was retained.

The committed donor cluster reproduced on its first and only natural compile.
The object advances to 38/116 strict functions, 3,261/19,281 meaningful code
bytes, and 3,536/20,112 padded code bytes.  The gain is four functions, 636
meaningful bytes, 672 padded bytes, and 23 relocations.  Direct hardened
comparison reports all 38 current exact functions `equal: true`.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00044ea0` | 76 | 80 | 0 | `40de58d006e8689f6f2a33f30962295135b6bd0ca184cc2159492233b0a28a6e` |
| `_code_00047160` | 330 | 336 | 9 | `1961b64f3afcc8064b0526eff4968e7be9bcbd416343e47ac32a38bb72982bb7` |
| `_ai_scripting_go_to_vehicle` | 115 | 128 | 7 | `6f38bcd811c426a9c59b980511191470c991d3c8d00c0dbc293c31ad72b8a483` |
| `_ai_scripting_go_to_vehicle_override` | 115 | 128 | 7 | `60b8c4798c36c94daff7430e226f5323cf7ef544cb7ca41b70f0beefd3d2c64d` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 78 nonexact functions.

## Accepted source controls

The private comparator retains the authenticated explicit three-way result:
the already-going-to-vehicle flag sorts first, then squared distance, with a
literal zero only when both keys compare equal.  The dispatch helper preserves
the donor's delayed `candidate_count` lifetime after the invalid-reference and
typed-unit guard, direct candidate-array indexing, increment at the end of the
candidate fill, and the exact squared-distance grouping
`dy * dy + (dx * dx + dz * dz)`.

The actor-reference iterator is fully named and asserted at 0x18 bytes with
`actor_index` at offset 0x10.  The candidate is fully typed and asserted at
0x0C bytes, with `distance_squared` at offset 0x04 and the boolean state at
offset 0x08.  The vehicle is obtained through the canonical typed
`unit_try_and_get` result; actor state/action fields and the seat-helper API
are named and typed.  The public script ABI remains 32-bit, with the proven
narrowing only at semantic call sites.

Both wrappers use the canonical `ai_debug.print_scripting` field.  The source
is readable C89 with declarations at block entry, vertical parameters, and
explicit void returns.  The transfer adds no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, optimizer barrier, raw address or offset access,
pointer/integer reconstruction, aliasing pun, undefined behavior, byte
forcing, object patch, or tool exception.

## Integration record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| D-A07 | Committed donor comparator, candidate collection, dispatch loop, and two public wrappers. | All four packets reproduced January on their first strict donor build. | Authenticated donor form. |
| I06 | Transfer the natural four-function cluster with named iterator/candidate layouts, typed accessors, and focused seat-helper ownership. | Warning-free first compile; four new strict packets exact and all 34 inherited packets preserved. | Final integrated form. |
| I06-H1 | Include `vehicle_scripting.h` at the top of `units.c`. | The broad declaration-order perturbation changed the protected Units sentinel and lost one global strict packet. | Rejected; do not repeat. |
| I06-H2 | Include `vehicle_scripting.h` immediately before the seat-helper definition. | The legacy declaration-order perturbation produced the same Units regression. | Rejected; do not repeat. |
| I06-H3 | Include the focused header at the end of `units.c`, after all emitted functions. | The compiler still type-checks the definition/declaration pair while every Units packet and global exact total remain unchanged. | Final declaration-conformance placement. |

No body retry, spelling sweep, scheduling experiment, compiler-control attempt,
or optimizer workaround was performed.  Do not collapse the explicit
comparator, hoist `candidate_count`, replace direct indexing, move the fill
increment, reassociate the squared distance, reintroduce opaque iterator
storage, broaden the declaration into `units.h`, or move the focused include
earlier in `units.c` without new authenticated source evidence.

## Header blast radius and residuals

The focused owner declarations and named layouts rebuilt their ordinary
dependents warning-free.  The final full graph increased global semantic and
accepted-exact totals by exactly four and lost no accepted packet.  The two
rejected header placements were discarded before the final proof build.

The remaining 78 functions are outside this bounded integration: 77 are still
unwritten and `encounter_get_platoon` remains the known header-inline
provenance residual.  The absent one-byte separator remains a data-provenance
task.  Any resumed AI Script wave must recheck all 38 accepted functions as a
set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `d62c7407fa0956cea7abacbf50fda23935961e8202f365953b1b040e2408391a`.
- Integrated `ai_script.h` SHA-256:
  `8d3b6e8a004aecdc1c26b091c48dd8785301b1db763bf6870ea62387c3ecc9db`.
- Focused `vehicle_scripting.h` SHA-256:
  `edb0fc3e7b4d5eb7bf6c6d6d743b4b9d2b4f414970ec4920a52fce6184b046af`.
- Integrated `units.c` SHA-256:
  `3f9c20fc9b8de85baf707039acea64929a9d71cff9e635bbba920daaf62d0c7c`.
- Integrated production `ai_script.obj` SHA-256:
  `73c4d73b89d6432fce7cda90806b4db36b797c63977fe8a3298d6e298c13aa1a`.
- `build/report.json` SHA-256:
  `a3c0f858b56ac38b7f7d42cb0edf209f371691475c5dab8d9330fdf0aef8ad23`.
- `build/semantic_report.json` SHA-256:
  `8c1f102d258f0c3662a2cbb5822f3678fa5b0bd2d9bb639a676a01e85c37468b`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,916 functions evaluated, 4,799 semantic exact,
  4,809 accepted exact, and zero unit errors.
- Campaign progress: 384/833 linked objects, 4,770/11,060 strict functions,
  and 606,159/2,198,102 strict code bytes.
- Halobetacache progress: 282/468 linked objects, 4,603/7,574 strict
  functions, and 593,245/1,770,166 strict code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 38-function comparison reports `all_equal: true`, 3,536 padded
  bytes, and 175 relocations; candidate layout assertions, added-line policy
  scan, and `git diff --check` are clean.

## Disposition

The four vehicle-dispatch functions are integrated and eligible for partial
strict function credit: 636 meaningful and 672 padded bytes.  The translation
unit correctly remains `NonMatching`; no data or whole-object credit is
granted.
