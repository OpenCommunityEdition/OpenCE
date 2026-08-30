# `ai_script.obj` count-family integration (2026-08-29)

This additive ledger records the bounded integration of the previously proven
AI Script count family.  The six-function cluster is strict exact on the
current canonical tree and preserves all nine inherited exact functions.
`ai_script.obj` remains `NonMatching`; no whole-object or data completion is
claimed.

## Scope and provenance

- Integration base:
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

Only the committed donor blob was consulted for transfer.  The donor
worktree's later dirty `ai_script.c` and `encounters.h` changes were explicitly
excluded.  The current three object-specific Jonas ledgers, the donor ledger,
the object-ledger schema, the current tree checkpoint, and the campaign tooling
notes were reviewed before mutation.

January COFF remains the byte, padding, relocation, linkage, and ownership
authority.  The only tracked implementation change is
`source/ai/ai_script.c`; this additive ledger is the only other tracked file.
No header, configuration, semantic allowlist, parked manifest, comparator,
compiler flag, or admission rule changed.

## Validated baseline and strict delta

At the integration base, the object had 9/116 strict functions and 160/20,112
padded code bytes.  The inherited exact set was the four lifecycle returns,
`ai_scripting_reconnect`, `ai_scripting_conversation_line`,
`ai_scripting_conversation_status`, `ai_scripting_retreat`, and
`ai_scripting_deselect`.

The transferred source compiled once through the natural production edge and
immediately advanced the object to 15/116 strict functions and 1,088/20,112
padded code bytes.  The gain is six functions, 897 meaningful bytes, 928
padded bytes, and 34 relocations.  Direct hardened comparison reports all 15
current functions `equal: true`; no inherited exact function was lost.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_000439c0` | 714 | 720 | 29 | `55292ff230977e5d59fd4db094fa56fc01cdf6d372c54c4c4bedb9581792b860` |
| `_ai_scripting_swarm_count` | 28 | 32 | 1 | `eb9506fd9e389aa9ceef7bc81e7adad7d53bfea4f8307b056b4e7140296e09eb` |
| `_ai_scripting_nonswarm_count` | 28 | 32 | 1 | `c93336ffbe9294d411357125175bb814de6604da6abbed0b718b4749dcc2ebf1` |
| `_ai_scripting_living_count` | 25 | 32 | 1 | `b9b8507d5098e24fa9bef9ba0d9777b3265d86639db8fe30158935dac26602b9` |
| `_ai_scripting_living_fraction` | 62 | 64 | 1 | `dd095f3976087d8610accd8e1c8e85c4bdba8307a00335027626cf05c414d5df` |
| `_ai_scripting_strength` | 40 | 48 | 1 | `007401fc57a48a33c68af296bce6e67452eca1b0af6041dd7acc5cfc0edf7060` |

The target-owned one-byte `_ai_script_squad_separator` datum remains absent,
as it was at baseline.  The object therefore remains ineligible for atomic
`Matching` admission even apart from its 101 residual functions.

## Accepted source controls

The file-local helper decodes the packed reference scope as unsigned, narrows
the encounter and sub-index values only at their typed semantic uses, and
reads the named encounter, platoon, and squad count/strength fields through
`encounters.h`.  Non-swarm counts retain the January-proven
`FLOOR(current_count - current_swarm_count, 0)` spelling.  The five public
functions are ordinary typed wrappers around that helper.

The source is readable C89: declarations begin their block scopes, parameters
are vertical, the private helper remains `static`, and no-argument inherited
functions continue to spell `void` and return explicitly.  The transfer adds
no assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw offset/address access, pointer/integer reconstruction, aliasing
pun, undefined behavior, byte forcing, object patch, or tool exception.

## Experiment record and do-not-repeat list

| ID | Source evidence | Result | Decision |
| --- | --- | --- | --- |
| D-A02 | The donor's first broad helper used signed scope extraction and a hand-written non-negative subtraction. | Equal size and relocation count, different normalized bytes. | Historical rejected form; do not repeat. |
| D-A02b | Unsigned scope extraction plus `FLOOR(delta, 0)`, with the five natural wrappers. | Six strict-exact functions, 928 padded bytes. | Authenticated committed donor form. |
| I01 | Transfer only D-A02b and its minimum typed includes/declarations onto the current base. | Exact on the first production compile; inherited exact set 9/9 retained. | Final integrated form. |

No spelling sweep, scheduling experiment, compiler-control attempt, or retry
was performed in this integration.  Do not replace the unsigned scope decode,
move narrowing to the script ABI boundary, replace `FLOOR` with a hand-written
conditional, remove `static`, or revisit the donor's rejected A02 form without
new authenticated source evidence.

## Residual classification and reopen criteria

The remaining 101 functions are outside this bounded integration.  Their
current classification remains source/topology reconstruction work, plus the
known `encounter_get_platoon` header-inline provenance residual described by
the donor ledger.  The absent one-byte separator remains a data-provenance
task.  Reopen this cluster only if a future header or topology change loses one
of the six strict packets, or if January-era source supplies contradictory
typed semantics.  Any resumed AI Script wave must recheck all 15 accepted
functions as a set.

## Reproducibility and gates

- Integrated `ai_script.c` SHA-256:
  `dc82e6b14d4d4785ef211c0d3587d28264f2ff0a9e41d8e8fee549ba76ef36d0`.
- Integrated production object SHA-256:
  `ce2d68e3a3540a704373e444f98664d053845aea4a819cd74183b3348a8948a4`.
- `build/report.json` SHA-256:
  `128f9b2afda1c3d13e9155b186d9488a0b07caa160b8181045612a4df52fd590`.
- `build/semantic_report.json` SHA-256:
  `be21306612815be9d600cd25e1cf1a426b7f0a1570d8d6b69fb722d56f7ae684`.
- Full Halo and libcmt production graphs, report, semantic report, progress,
  and semantic progress passed.
- Semantic audit: 470 units, 4,893 functions evaluated, 4,776 semantic exact,
  4,786 accepted exact, and zero unit errors.
- Campaign progress: 384/833 linked objects, 4,747/11,060 strict functions,
  and 603,852/2,198,102 strict padded code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- Tool suite: 212 passed.
- Protected Units sentinel remains exact at 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Direct 15-function comparison reports `all_equal: true`; forbidden-source
  scan and `git diff --check` are clean.

## Disposition

The six-function count family is integrated and eligible for partial strict
function credit: 897 meaningful and 928 padded bytes.  The translation unit
correctly remains `NonMatching`; no data or whole-object credit is granted.
