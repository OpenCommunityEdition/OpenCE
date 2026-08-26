# `hs_runtime.obj` mask-index and operand-order one-shot (2026-08-25)

## Frozen scope and provenance

This bounded wave starts from clean cumulative integration commit
`7212004a3157c21983891f598b4f494e3b21035b` in isolated branch and worktree
`jonas/hs-runtime-mask-index-20260825`. The only permitted production source is
`source/hs/hs_runtime.c`; this new Jonas ledger is the only other permitted
tracked path. Every header, configuration, semantic exception, parked record,
object option, Units path, and other source file is frozen. The gated Units
shared-header package and all protected Units neighbors are outside this wave.

Exactly one fixed `_code_000ba390` candidate may receive one ordinary
code-producing compile. There is no post-emission spelling change, retry,
tuning pass, alternate candidate, compiler change, or flag change. The pinned
compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The frozen starting identities are:

- source Git blob `73261b3dec4b32e61c12166c8d89ffb2879aea38`;
- source raw SHA-256
  `8e0abfbe84f43a96d3e26713e4b38d952a640d504f990b7cf61811a7cf5af4cf`;
- configuration Git blob `8825927921a7abf9ad1a30521c3c80df265cdb3a`;
- configuration raw SHA-256
  `2829306ccd9c648d86972cb84e8d7fb8de793b779e6c560c89e415c5b4c79f6c`;
- compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- January split-object raw SHA-256
  `8ead2e539de118ecb284e82abe96fbb636b66800bbe623c2f8a306546a4b8ea6`;
- cumulative base-object raw SHA-256
  `af6b5ed5f533b82e03022bee8f71e62ad6e1e7599c7164aca0444051626d6ca4`;
  and
- ignored baseline manifest
  `build/audit/hs_runtime_mask_index_baseline_20260825.json`, 1,557,332
  bytes, SHA-256
  `47c2be5ae1ced930c12efe35425b8a7c6d4dce0375018b62c779280361bc9039`.

The immutable copied baseline object is
`build/audit/hs_runtime_mask_index_baseline.obj` with the same base-object
hash. After support-only configuration, the copied object's timestamp was
restated without changing its bytes, and a one-target Ninja dry run reported
no work before this ledger or source candidate existed.

## January target and protected translation unit

January and the cumulative base both emit `_code_000ba390` with 126 meaningful
bytes, a 128-byte padded COMDAT, and 10 relocations, but their normalized code
is nonexact:

| Owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| January `_code_000ba390` | 126 | 128 | 10 | `5b133e16432ec0ec7dab5026d0eb4375421d4d22de51370b0225e596a51b726f` |
| Base `_code_000ba390` | 126 | 128 | 10 | `a33b9d1abba18b087581aa6416db4e8a4ea9efefabf0d10ef98b2b875091160a` |

The first raw and normalized instruction divergence is at `+0x06`. January
tests `actual_type` in AX, sign-extends it immediately into ECX, sign-extends
`desired_type` from SI into EDX, loads the desired mask through EDX into BX,
and loads the actual mask through ECX into DI. The base instead sign-extends
and loads the desired mask first, then sign-extends the actual index into EDX.
Consequently January's first `_hs_object_type_masks` relocation is at `+14`,
while the base's is at `+11`; all other nine relocation records already agree
in address, type, and destination.

At the return, January destructively computes `and ebx,edi`, compares the
result against the actual mask with `sub bx,di`, and carries the boolean in EBX
before moving BL to AL. The base preserves both word masks by copying EDI into
EAX and computes `and eax,ebx`. This is the ordinary allocation distinction
expected when the source's commutative AND operands are written in the
opposite order.

The baseline manifest freezes all 66 target function records, all 31 emitted
base function sections, all 42 emitted base non-code sections, and all 168 base
symbols. It records 25 accepted functions: 24 strict-exact owners plus the
inherited ordinary accepted strict-mismatch `_code_000b9a10`. They contribute
1,717 meaningful and 1,920 padded accepted code bytes. Every other emitted and
absent owner is frozen as well.

The direct caller `_hs_can_cast` is already strict exact at 253 meaningful and
256 padded bytes, 11 relocations, and normalized SHA-256
`43653ecb2f3ac4d6c6d44e1077ce61c1361dfbce9dcb63942b93cea7c0620a18`.
It calls `_code_000ba390` twice and is an explicit regression sentinel. A
successful shot projects 26/66 accepted functions, 1,843 meaningful and 2,048
padded accepted code bytes. It cannot complete or admit the object, and the
object continues to receive no data credit out of 11,940 target bytes.

The January split artifact records `_code_000ba390` as an external owner symbol
while the current private definition emits a static symbol. This is not source
linkage provenance. The strict-exact private HS helpers `_code_000b9e20`,
`_code_000b9f20`, and `_code_000ba1a0` all have the same target-class-2 and
base-class-3 difference. Claude's methodology explains that csplit defaults a
manually split label to external when `config/symbols.json` omits an explicit
static marker. Both January and the exact rebuilt `_hs_can_cast` pass these
arguments privately in AX and SI with no stack pushes, which is the binding ABI
evidence. C1 therefore preserves the static definition and requires the
baseline's complete symbol shape to remain unchanged.

## Claude documentation, history, and tooling reconciliation

All durable Claude material that names this object, this function, or the
relevant comparison and code-generation tooling was screened before freezing
C1. Reconstruction commit `a3e2662acd6ea7ccb934f6eff47edc8588f72ddf`
introduced the private prototype, definition, and two same-translation-unit
calls. Every readable durable Claude branch that contains the body retains the
same `static` definition, two word masks, desired-then-actual table accesses,
and actual-first commutative AND expression. Twelve reachable historical
source snapshots resolve to 10 unique blobs, eight of which contain the body;
none contains a widened actual index or reversed-AND candidate. Nine hundred
twenty-four accessible worktree bodies likewise retain the current form.

Claude's `research/residual_frontier.json` classifies `_code_000ba390` as a
128/128-byte pure-codegen residual with equal relocation counts and identities,
`UNKNOWN` category, `unclassified` subtype, `recipe: null`, and confidence
`NONE`. It supplies no causal claim, tested candidate, rejection, or closure.
A Claude-wide durable documentation and source search names the function only
in symbol configuration, source, and that residual inventory. The corrected
register census does not classify it as a pure register rename, and the
code-generation dictionary, flag-provenance census, register-tie material,
unwritten-recipe inventory, and HS ledgers contain no `_code_000ba390`
experiment or alternate flag claim. The new Jonas short-to-real ledger closes
only `_code_000ba290` and `_code_000ba2c0`; it does not cover this helper.

Claude's corrected allocation findings provide mechanism support without
claiming a guaranteed result. The cumulative register-tie record demonstrates
that a declaration initializer can change IR tuple creation and allocation even
when later load scheduling is unchanged, and its reachability note explicitly
identifies materializing an index before a competing value as a legitimate
ordinary-C lever. A separate house-rule example records a widened `long` local
producing an early sign extension where an inline narrow form did not. Those
findings support C1's initialized index as a typed value/lifetime, not as mere
declaration reordering.

The same Claude record cautions that commutative operand-order effects are
contextual: a swap was inert in a different hashtable assertion, while other
documented cases changed load or parameter choices. That unrelated miss does
not reject this word-mask AND, whose target directly selects the destructive
desired-mask form, but it prevents treating operand order as a universal rule.
The two interacting factors are therefore frozen as one combined candidate and
will not be split into post-shot variants.

Claude's private-ABI guidance says to work outward from an exact caller and
warns that external linkage can destroy a same-translation-unit register
convention. That guidance, the two exact AX/SI call sites at `_hs_can_cast`
relocations `+0x97` and `+0xD0`, and the same-TU-only xref census all prohibit
removing `static`. Its csplit findings independently caution that manually
split owner symbols can have synthetic external storage. Width-class IR labels
are not treated as physical-register evidence.

No Claude artifact grants credit: hardened `tools/coff_compare.py`, full-owner
regression comparison, and relocation identity remain the acceptance
authorities. Similarity, raw byte views, semantic equivalence, width-class IR
labels, or comparator exceptions grant no credit.

## Frozen candidate C1

C1 preserves the private signature and both assertions. It introduces one
natural widened index before the two word masks, uses that index for the actual
mask load, and writes the commutative intersection in January's destructive
operand order:

```c
static boolean code_000ba390(
	short actual_type,
	short desired_type)
{
	long actual_type_index = actual_type;
	word actual_type_mask;
	word desired_type_mask;

	desired_type_mask = hs_object_type_masks[desired_type];
	actual_type_mask = hs_object_type_masks[actual_type_index];

	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x599,
		actual_type>=0 && actual_type<NUMBER_OF_HS_OBJECT_TYPES);
	match_assert("c:\\halo\\SOURCE\\hs\\hs_runtime.c", 0x59a,
		desired_type>=0 && desired_type<NUMBER_OF_HS_OBJECT_TYPES);

	return (desired_type_mask & actual_type_mask)==actual_type_mask;
}
```

The widened initializer gives the compiler a source-backed reason to establish
the actual index before the existing desired-mask load, while the later table
access keeps the target's desired-then-actual memory order. Reversing only the
commutative AND operands gives the compiler a source-backed reason to overwrite
the desired mask in EBX and return the comparison through BL. The mathematical
result is unchanged because a mask contains all actual bits exactly when its
intersection with the desired mask equals the actual mask.

C1 is readable, defined, typed C. It uses no assembly, `volatile`, barrier,
pragma, intrinsic, attribute, forced/no inline, `register`, raw offset/address,
pointer pun, inactive-union read, undefined behavior, synthetic anchor,
object-byte patch, comparator exception, or flag/tool change. It adds no
signature, caller, literal, storage, or runtime-data owner.

## Acceptance, rejection, and closeout gates

After this ledger and the exact C1 source exist, the real compiler receives one
`/Zs` parse-only invocation, proven not to modify the immutable baseline
object. Exactly one ordinary Ninja edge may then compile
`build/base/source/hs/hs_runtime.obj`. The unchanged first-shot object is copied
under ignored `build/audit/` before inspection.

Acceptance requires `_code_000ba390` to reproduce January's meaningful and
padded sizes, normalized hash, and all 10 ordered relocation semantics. It must
place the two mask relocations at `+14` and `+23`, preserve the baseline's
static symbol tuple, and retain `_hs_can_cast` plus all other 24 accepted
functions. The complete baseline manifest must show every other code and
non-code owner unchanged in normalized bytes, extent, flags, ownership, and
ordered relocations. No unexpected constant, storage, or symbol owner may
appear.

Any difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning, and only this ledger records the rejection. Without genuinely new
disassembly or provenance evidence, no declaration-order, temporary-width,
operand-order, cast, signature, linkage, or flag variant follows, and the two
C1 factors are not compiled separately. If exact,
the source and initial ledger are committed, followed by full Halo and libcmt
builds, fresh ordinary, semantic, and progress reports,
semantic/admission/parked audits, the complete tooling suite, policy/scope and
JSON checks, and a clean committed regression snapshot. A verified literal
one-object delete/rebuild must reproduce the first-shot runtime result. Replay
evidence is then appended in a ledger-only commit and repeated from corrected
HEAD. No amend, push, rebase, history rewrite, or worktree removal is
authorized.

## Production result

The real XDK `/Zs` invocation passed and left the live baseline object raw
SHA-256
`af6b5ed5f533b82e03022bee8f71e62ad6e1e7599c7164aca0444051626d6ca4`
unchanged. The candidate source then remained frozen at Git blob
`ceaeb23d71b060ee32b30db3fffd7025c31f280c`. A Ninja dry run exposed exactly
one CL action, and exactly one ordinary production edge compiled
`build/base/source/hs/hs_runtime.obj`:

```text
[1/1] CL build\base\source\hs\hs_runtime.obj
hs_runtime.c
```

There was no direct code-producing CL command and no second candidate compile.
The unchanged first-shot object is preserved at ignored path
`build/audit/hs_runtime_mask_index_first_shot.obj`, raw SHA-256
`c004605279fb5b95065499faf73c95ab1aeafe86b9173aee12d01df617cf6374`.

C1 failed closed. VC7 canonicalized both proposed source factors away and
reproduced the complete baseline `_code_000ba390`:

| Owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| January `_code_000ba390` | 126 | 128 | 10 | `5b133e16432ec0ec7dab5026d0eb4375421d4d22de51370b0225e596a51b726f` |
| First-shot `_code_000ba390` | 126 | 128 | 10 | `a33b9d1abba18b087581aa6416db4e8a4ea9efefabf0d10ef98b2b875091160a` |

The initialized `long actual_type_index` did not survive as an early value.
The compiler still sign-extends and loads `desired_type` first, then
sign-extends `actual_type`; the first mask relocation remains at baseline
`+11` instead of January's required `+14`. It also canonicalizes the reversed
source intersection back to `mov eax,edi; and eax,ebx` and returns the boolean
through EAX, rather than destructively updating EBX and moving BL to AL. The
entire first-shot disassembly, relocation sequence, padded extent, and
normalized hash are byte-for-byte the baseline. The target-supported source
topology is therefore allocator-inert in this context.

The exact `_hs_can_cast` caller remains 256 padded bytes with all 11 relocation
records and normalized SHA-256
`43653ecb2f3ac4d6c6d44e1077ce61c1361dfbce9dcb63942b93cea7c0620a18`.
The one-unit regression gate retained all 25 inherited accepted functions in
`still_exact`, with empty `newly_exact` and `changed_nonexact` lists. Its sole
failure was ordinary compiler metadata: anonymous `.debug$S` grew from 179 to
183 bytes between the copied cumulative baseline and the fresh isolated
compile. Independent COFF inspection retains the same 73-section and
168-symbol shape; every code owner, runtime constant/storage owner, symbol
tuple, and ordered relocation is otherwise unchanged. Debug metadata grants no
runtime credit.

C1 was inverse-reverted wholesale. The two factors were not compiled
separately, and there was no declaration-order, temporary-width, cast,
signature, linkage, flag, or operand variant. The working source again hashes
to baseline Git blob `73261b3dec4b32e61c12166c8d89ffb2879aea38`.
This result retires both the initialized-index and reversed-intersection forms
for this helper; another production wave requires genuinely new source or
compiler provenance.

Because no implementation or metadata change survives, there is no exact
function gain, progress or admission change, full-build closeout, or
committed-state implementation replay. `hs_runtime.obj` remains 25/66 accepted
functions, 1,717 meaningful and 1,920 padded accepted code bytes, and no data
credit out of 11,940 target bytes. This documentation-only record is the
complete outcome of the one-shot.
