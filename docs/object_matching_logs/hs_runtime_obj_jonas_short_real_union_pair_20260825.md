# `hs_runtime.obj` short-to-real union pair one-shot (2026-08-25)

## Frozen scope and provenance

This bounded wave starts from clean cumulative integration commit
`5bb854edd62a541882d8677504fb3b83afc21002` in isolated branch and worktree
`jonas/hs-runtime-short-real-20260825`. The only permitted production source is
`source/hs/hs_runtime.c`; this new Jonas ledger is the only other permitted
tracked path. Every header, configuration, semantic exception, parked record,
object option, and Units path is frozen. In particular, the gated Units
shared-header package and the protected Units neighbors are outside this wave.

Exactly one fixed two-function candidate may receive one ordinary
code-producing compile. There is no post-emission spelling change, retry,
tuning pass, alternate candidate, compiler change, or flag change. The pinned
compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The frozen starting identities are:

- source Git blob `73261b3dec4b32e61c12166c8d89ffb2879aea38`;
- configuration Git blob `8825927921a7abf9ad1a30521c3c80df265cdb3a`;
- compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- January split-object raw SHA-256
  `8ead2e539de118ecb284e82abe96fbb636b66800bbe623c2f8a306546a4b8ea6`;
- cumulative base-object raw SHA-256
  `af6b5ed5f533b82e03022bee8f71e62ad6e1e7599c7164aca0444051626d6ca4`;
  and
- ignored baseline manifest
  `build/audit/hs_runtime_short_real_baseline_20260825.json`, 1,557,332
  bytes, SHA-256
  `295cc0f4494124b940ea936dcd6da5e16a9a2e7702d6b020fdc086937b3b3328`.

The immutable copied baseline object is
`build/audit/hs_runtime_short_real_baseline.obj` with the same base-object
hash. A one-target Ninja dry run reported no work before this ledger or source
candidate existed.

## January targets and protected translation unit

Both requested functions already have January's padded extent and have no
relocations, but their normalized instruction bytes are nonexact:

| Function | Meaningful | Padded | Relocations | January SHA-256 | Base SHA-256 |
|---|---:|---:|---:|---|---|
| `_code_000ba290` | 21 | 32 | 0 | `edad63c921e96086fc19a9508f72f918ba0d80645bb595a3c38d0bbe793202ac` | `42154cb97a95437f3880603d2c432bd671c18f3b1c0ce9a640c79052fe519f15` |
| `_code_000ba2c0` | 22 | 32 | 0 | `d59469372c5e43c468a74db2b54fed0bf9f75c2ac0ba32505134ad3811aa26c0` | `cc674820b3dbbac354c3838a453ed129b3a06baf03033da6a474ceb0a750e54d` |

January uses the incoming four-byte argument slot as the conversion workspace.
It sign-extends the low short into EAX, stores that promoted integer back to
`[ebp+8]`, performs `fild` and `fstp` through that same slot, and returns its
four bytes in EAX. `_code_000ba2c0` adds the expected `inc eax`. The current
`long` plus pointer-pun reconstruction instead reserves `[ebp-4]` for the
promoted integer, so each function first diverges at `+0x03` and carries the
extra local-frame teardown.

The baseline manifest freezes all 66 target function records, all 31 emitted
base function sections, all 42 emitted non-code sections, and all 168 base
symbols. It records 25 accepted functions: 24 strict-exact owners plus the
inherited ordinary accepted strict-mismatch `_code_000b9a10`. C1 must preserve
every owner other than the requested pair. The object currently claims 1,717
accepted meaningful code bytes out of 14,604 and no data credit out of 11,940
target bytes. A successful shot projects 27/66 accepted functions and 1,760
accepted meaningful code bytes, but cannot complete or admit the object.

## Claude documentation and tooling reconciliation

All durable Claude material that names this object, these functions, or the
relevant comparison/code-generation tooling was screened before freezing C1.
The Claude-tip source through `fae4cc9a` and every unique reachable committed
`hs_runtime.c` blob preserve the original `long(long)` pointer-pun bodies.
Neither a union parameter/return form nor a union-typed table declaration has
a recorded compile, rejection, or ledger. The only exact-name documentation
is the later HS checkpoint, which lists both functions as remaining
`hs_runtime.obj` residuals without describing an experiment on them.

Claude's `research/residual_frontier.json` classifies both as 32/32-byte,
zero-relocation, pure-codegen residuals with `recipe: null` and confidence
`NONE`; it supplies no conflicting causal claim. The checkpoint's statement
that struct and union variants were exhausted belongs to the separate
`hs.obj` FPU-wrapper section. It does not cover these `hs_runtime.obj`
converters.

Commit `55a32325e82710aafe78ad6368fe2e2a4b369516`, co-authored by Claude,
is the closest positive precedent. It replaced `_code_000ba310`'s long/pun
reconstruction with a union-by-value and typed-result topology, recovered
January's incoming-slot reuse exactly, and preserved siblings. Its visible
diff leaves `_code_000ba290` and `_code_000ba2c0` untouched. The neighboring
strict-exact converters `_code_000ba220`, `_code_000ba240`,
`_code_000ba2b0`, and `_code_000ba2e0` independently confirm the same
four-byte union ABI in this compiler and translation unit.

The target-owned `_typecasting_procedures` matrix is not emitted by the
candidate source. Its January `.data` relocations point to `_code_000ba290`
once and `_code_000ba2c0` four times, alongside union-spelled conversion
siblings. In current C, the extern matrix is only null-tested by `hs_can_cast`;
there is no direct C call or initializer to type-check. A four-byte union and a
`long` both use one cdecl stack slot and EAX return under the pinned compiler,
and the undecorated C symbol names do not change. The existing convenience
typedef is already not C-type-compatible with several exact union converters,
so it is frozen rather than widened into a table redesign.

The corrected Claude register census, residual frontier, csplit-artifact
accounting, flag-provenance census, and code-generation dictionary provide no
whole-object or alternate-flag claim for this pair. Width-class IR labels are
not treated as physical-register evidence. Hardened `tools/coff_compare.py`,
the full-owner regression gate, and relocation identity remain the acceptance
authorities; fuzzy similarity, semantic-only equivalence, raw byte views,
debugger branches, and comparator exceptions grant no credit.

## Frozen candidate C1

C1 changes only the two prototypes and definitions from raw `long` transport
to the already defined four-byte conversion union. Each body explicitly
promotes the short member into the long member before converting that active
member to the real member:

```c
union hs_conversion_result code_000ba290(
	union hs_conversion_result value)
{
	value.long_integer = value.short_integer;
	value.real = (real)value.long_integer;

	return value;
}

union hs_conversion_result code_000ba2c0(
	union hs_conversion_result value)
{
	value.long_integer = value.short_integer+1;
	value.real = (real)value.long_integer;

	return value;
}
```

This is natural typed C and maps directly to January's promoted-integer store,
in-place x87 conversion, and union-bit return. Every union read follows the
member most recently supplied or written by the source topology. C1 uses no
assembly, `volatile`, barrier, pragma, intrinsic, attribute, forced/no inline,
`register`, raw offset/address, pointer pun, inactive-union read, undefined
behavior, synthetic anchor, object-byte patch, or flag/tool change.

## Acceptance, rejection, and closeout gates

After this ledger and the exact C1 source exist, the real compiler receives
one `/Zs` parse-only invocation, proven not to modify the baseline object.
Exactly one ordinary Ninja edge may then compile
`build/base/source/hs/hs_runtime.obj`. The unchanged first-shot object is
copied under ignored `build/audit/` before inspection.

Acceptance requires both requested functions to reproduce January's
meaningful/padded sizes, normalized hashes, symbol properties, and empty
relocation sets. The complete baseline manifest must retain every accepted
owner, and a baseline-to-shot comparison must show every other code and
non-code owner unchanged in normalized bytes, extent, flags, ownership, and
ordered relocation semantics. No unexpected constant, storage, or symbol
owner may appear.

Any difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning, and only this ledger records the rejection. If exact, the source and
initial ledger are committed, followed by full Halo and libcmt builds, fresh
ordinary/semantic/progress reports, semantic/admission/parked audits, the
complete tooling suite, policy/scope and JSON checks, and a clean committed
regression snapshot. A verified literal one-object delete/rebuild must
reproduce the first-shot runtime result. Replay evidence is then appended in a
ledger-only commit and repeated from corrected HEAD. No amend, push, rebase,
history rewrite, or worktree removal is authorized.

## Production result

The real XDK `/Zs` invocation passed and left the baseline object raw SHA-256
`af6b5ed5f533b82e03022bee8f71e62ad6e1e7599c7164aca0444051626d6ca4`
unchanged. The candidate source then remained frozen at Git blob
`b15febcedf263484ad46226e82ef83ded3dad6eb`. A Ninja dry run exposed exactly
one CL action, and exactly one ordinary production edge compiled
`build/base/source/hs/hs_runtime.obj`:

```text
[1/1] CL build\base\source\hs\hs_runtime.obj
hs_runtime.c
```

There was no direct code-producing CL command and no second candidate compile.
The unchanged first-shot object is preserved at ignored path
`build/audit/hs_runtime_short_real_first_shot.obj`, raw SHA-256
`57bae5e3afbf6804c34396d30cf3598e58bc6435e4a26158ecfef1e5fb3c75b2`.

C1 failed closed. Both requested functions reproduced their prior normalized
code byte-for-byte:

| Function | First-shot padded | First-shot relocations | First-shot SHA-256 | Required SHA-256 |
|---|---:|---:|---|---|
| `_code_000ba290` | 32 | 0 | `42154cb97a95437f3880603d2c432bd671c18f3b1c0ce9a640c79052fe519f15` | `edad63c921e96086fc19a9508f72f918ba0d80645bb595a3c38d0bbe793202ac` |
| `_code_000ba2c0` | 32 | 0 | `cc674820b3dbbac354c3838a453ed129b3a06baf03033da6a474ceb0a750e54d` | `d59469372c5e43c468a74db2b54fed0bf9f75c2ac0ba32505134ad3811aa26c0` |

VC7 therefore treats the explicit typed union member sequence as
allocator-inert for these functions. It still reserves the four-byte local,
stores the promoted integer at `[ebp-4]`, converts from that local, and writes
the real result into the incoming slot. Union-by-value ABI alone does not
recover January's workspace coalescing.

The one-unit regression check found all 25 inherited accepted functions in
`still_exact`, with empty `newly_exact` and `changed_nonexact` lists. Its sole
failure was `.debug$S|anonymous=0`, the expected compiler debug/timestamp
metadata change between the copied cumulative object and a fresh isolated
compile. An independent baseline-to-shot COFF census found the same 73
sections and 168 symbols, identical symbol tuples, and identical bytes for
every section except `.debug$S`, whose metadata payload changed from 179 to
183 bytes. No code owner, runtime constant/storage owner, symbol contract, or
ordered relocation changed; the requested pair's unchanged normalized hashes
also prove that the raw-object difference grants no runtime result.

C1 was inverse-reverted wholesale with no alternate direct-assignment form,
retry, or tuning. The working source again hashes to baseline Git blob
`73261b3dec4b32e61c12166c8d89ffb2879aea38`. The shorter expression
`value.real = value.short_integer` remains mechanically uncompiled, but this
shot removes the prior slot-reuse inference and leaves it below the evidence
threshold for another production wave absent genuinely new compiler or source
provenance.

Because no implementation or metadata change survives, there is no exact
function gain, progress or admission change, full-build closeout, or
committed-state implementation replay. `hs_runtime.obj` remains 25/66 accepted
functions, 1,717/14,604 accepted meaningful code bytes, and 0/11,940 data
bytes. This documentation-only record retires the explicit two-stage union
candidate and is the complete outcome of the one-shot.
