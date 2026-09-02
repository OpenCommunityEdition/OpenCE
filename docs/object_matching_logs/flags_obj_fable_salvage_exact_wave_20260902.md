# `flags.obj` Fable salvage exact wave (2026-09-02)

## Decision

The preserved Fable wave is accepted after house-rule reconciliation.  Against
canonical `293d9d0c4`, `source/objects/widgets/flags.obj` advances from 10
exact, zero residual and six unwritten functions to 14 exact, zero residual
and two unwritten functions.  The strict gain is four functions and 1,552
padded code bytes, with no loss among the ten inherited exact functions.

| Newly exact function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_flag_new` | 384 | 12 | `90715ff5d28478273c1f1ed96124b151c41c6ec6c3f7f483ba34c506a1bfacee` |
| `_flag_set_attachment_shape` | 208 | 3 | `bb7f31e5ab2cfcf69e162f33b056f8723ff9c07e74fe39ca19e8a4326d5972f0` |
| `_flag_tesselate_region` | 288 | 1 | `cb2c11dc56ad0a99b64cdbc051e618aa978d9e5ee451794dcbb87ee03b9bb32e` |
| `_flag_update_attachment` | 672 | 12 | `35985362dc30b3cf3fe23bee73e7fb1111de5d78e0dd127061fa809a228c0e1d` |

The object is not complete.  `_flag_update` (1,184 padded bytes) and
`_flag_render_proper` (2,112) remain explicitly unwritten; neither receives
fuzzy or exact credit here.

## Evidence and superseded boundaries

- January's split object is the byte, relocation, ABI and version authority.
- The preserved source input is commit `11831ea54` on
  `root/fable-small-families-salvage-20260902`.
- The independent HaloCEA reconstructions of `flag_new`,
  `flag_update_attachment`, `flag_tesselate_region`,
  `flag_set_attachment_shape`, their definition records and attachment-point
  records corroborate names, field offsets and the cloth algorithms.  They are
  later-build semantic evidence only; where their topology differs, January
  wins.
- All earlier Flags ledgers were reviewed.  In particular, the two rejected
  `_flag_new` candidates covered an invalid early-return interpretation and a
  different nested source shape.  This wave supplies the missing coherent
  topology: a valid tag may still allocate a datum whose invalid dimensions or
  missing blue shader make it a `noop`; it does not reject that allocation.
  The function retains the target's shared result return.  That is new evidence
  and supersedes the earlier instruction not to respell either exhausted
  candidate.

Strict equality is not inferred from size or an objdiff score.  Hardened COFF
comparison proves equal padded bytes, normalized bytes, relocation address,
type, destination and addend for all four additions.  The object exact-set
comparison reports precisely those four gains and `lost_exact: []`.

## Source-authentic behavior review

`flag_new` always performs January's scenario access, resolves the typed flag
tag, allocates from `flag_data`, marks unusable definitions as no-op datums,
and otherwise initializes all vertices and interior cells before applying the
two edge shapes.  The direct origin/zero-vector copies and short loop indices
match the target schedule.

`flag_tesselate_region` clips a rectangular cell region and uses named
tesselation constants to select its diagonal orientation.  Within the proven
loop bounds the compact `horizontal <= vertical` result is equivalent to the
later reconstruction's expanded sign/comparison expression.  January's exact
machine code confirms that this is not a coincidental simplification.

`flag_set_attachment_shape` makes two typed region calls per attachment span.
HaloCEA's later build instead expands the first half in place and calls the
helper for the second half.  January's exact 208-byte body and its two helper
relocations establish that the retained two-call topology is the correct Xbox
version.

`flag_update_attachment` resolves every named object marker, builds the
attachment row maps, interpolates force points, rigidly translates vertices
after a large anchor jump, and stores the new first attachment.  Its surprising
float-to-long `abs` checks are authentic: January contains the three `__ftol2`
calls, and the later HaloCEA analysis independently identifies the same
integer-truncation behavior.  The retained source therefore does not replace
it with `fabs`.  A zero-height span can also reach January's unguarded
zero-denominator interpolation; the later build adds a guard, but inserting it
would contradict the exact January control flow.  These oddities are recorded
rather than disguised as cleanup.

## House-rule reconciliation

- Raw typed `tag_get` and `datum_get` sites were replaced with TU-owned
  `flag_definition_get` and `flag_get` wrappers.  `FLAG_TAG`, flag limits,
  attached-edge and tesselation values are semantic constants.
- Structures use `real`, cseries integer aliases, named tag records and
  `TAG_BLOCK_GET_ELEMENT`; object marker access uses the owning object API.
- Public `flag_new` remains declared in `flags.h`.  Helper declarations stay
  in their owning translation unit because no other source consumes them.
- Function parameters follow the multiline convention and every retained body
  has an explicit terminal return.  Existing declaration/assignment separation
  is retained only where changing the exact C2 lifetime would lose target code.
- No address-derived private name or BSS name, raw byte offset, inline assembly,
  volatile/register forcing, optimization pragma, forced inline/noinline,
  fake dependency, representation pun or nonsensical exactness carrier is
  present.
- The January inline schedule is unchanged.  The candidate emits no
  `point_from_line3d` symbol or COMDAT.

The accessor/formatting cleanup recompiles to the same 14/0/2 gate, proving it
is byte-neutral rather than trading readability for credit.

## Verification

- `python tools/campaign/gate.py source/objects/widgets/flags --all`:
  14 exact, zero residual, two unwritten.
- Hardened target/baseline/candidate exact-set comparison: zero lost, exactly
  four gained.
- Candidate/source scan for `point_from_line3d`, `__asm`, `_code_` and `_bss_`:
  no findings.
- `tools/fake_match_scan.py`: zero review leads.
- Canonical `ninja all_source progress semantic_progress`: pass, 5,221
  accepted semantic-exact functions, zero unit errors, and 688,799 / 2,198,102
  exact code bytes overall.  The wave contributes four functions and 1,521
  report-accounted code bytes relative to canonical `9083d83ce`.
- `python -m pytest tools -q -p no:cacheprovider`: 258 passed.
- Parked validation: 108 active, zero stale, zero invalid.
- Object-admission audit: zero candidates and zero contradictions; the single
  inherited `player_rumble` rejection is unchanged.
- Canonical `flags.obj` contains no `point_from_line3d` symbol or COMDAT.
- `git diff --check`: clean.

