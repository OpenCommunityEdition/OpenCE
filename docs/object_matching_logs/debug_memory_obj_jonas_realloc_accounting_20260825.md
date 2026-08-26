# `debug_memory.obj` Jonas realloc-accounting one-shot (2026-08-25)

## Frozen scope and provenance

This bounded source-only wave starts from clean cumulative integration commit
`689f0a24c2cfab8fe35261d12cb3dd2a1e58d74b` in isolated branch/worktree
`jonas/debug-realloc-accounting-20260825`. The only permitted production
source is `source/cseries/debug_memory.c`; this Jonas ledger is the only other
permitted tracked path. Every header, configuration, semantic exception,
parked record, object option, Units path, and other source file is frozen. The
gated Units shared-header package and all protected Units neighbors are
outside this wave.

Exactly one fixed `_debug_realloc` candidate may receive one ordinary
code-producing compile. There is no post-emission spelling change, retry,
tuning pass, alternate candidate, compiler change, or flag change. The pinned
compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The frozen starting identities are:

- source Git blob `61f564e6b90fc9f8ceb5d3e0f7e0a12d5c57f07e`;
- source raw SHA-256
  `27ec2d08e3b81879290b1e669ae6d8545cc6615c68570be26e260ac5c69aeaea`;
- frozen candidate source Git blob / raw SHA-256
  `534dac4d3137c828d96ff446e26c7890bbaead0b` /
  `260575f5729c47cb2d78bcdbe25d4b5949751436fde3096df56339d8ecb68d4e`;
- configuration raw SHA-256
  `2829306ccd9c648d86972cb84e8d7fb8de793b779e6c560c89e415c5b4c79f6c`;
- compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- January executable SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January split-object raw SHA-256
  `bcf575ff9e4dc3749ba66beb41991f62985262fccc3c643f06251d14ba942244`;
- cumulative source-identical object raw SHA-256
  `51360b620030da36e1434a02df34b1990731f990ddc7cb15caecd489602da3e2`;
- isolated source-identical baseline-object raw SHA-256
  `338379b8df74bc2e5d47a65fc49b54d3f83ad9f33c9e6b021839d0e871854bbb`;
  and
- frozen regression manifest
  `build/audit/debug_realloc_accounting_baseline_20260825.json`, 637,986
  bytes, SHA-256
  `60caf3a93ec7a27be7cce4d020d01ff12bc4ba91c6a94e7e3eac24ca268081ed`.

The isolated support setup compiled the unchanged baseline source once before
the candidate existed, then reached a one-target no-work state. Its immutable
copy is `build/audit/debug_realloc_accounting_baseline_20260825.obj`. This is
baseline establishment, not a candidate compile. Whole-object raw digests are
artifact sentinels only; strict authority is normalized function bytes plus
ordered relocation address, type, resolved destination and addend, symbol
shape, and complete runtime non-code ownership.

## January target and protected translation unit

January emits `_debug_realloc` with 403 meaningful / 416 padded bytes, 25
relocations, normalized SHA-256
`d812408198d3b0ffe7e64a41f32db28bceb17093d3b341f8c2c7d75e0f47f1d1`,
symbol type `0x20`, external storage class 2, and section flags `0x60501020`.
The source-identical baseline emits 393 meaningful / 400 padded bytes, the
same relocation count and symbol/section properties, and normalized SHA-256
`9597eb98281b48703e2ee47f5fb336e3b1fc3334c444dfc881549ac6dbfe3e47`.

The target tail directly proves the missing source predicate. At `+0x15F` it
tests the returned user pointer and branches into accounting at `+0x16C` when
the result is non-null. On the null-result path it tests `size` at `+0x168`,
skips accounting at `+0x189` when size is nonzero, and otherwise falls through
to `+0x16C`. The exact condition, in the target's evaluation order, is
therefore `result != NULL || size == 0`.

That condition is also the required realloc bookkeeping. A successful
allocation changes the tracked total, a zero-size realloc/free subtracts the
old allocation, and a failed nonzero realloc leaves the total unchanged. The
baseline `if (size != 0)` does the opposite in both exceptional cases: it
updates totals after a failed nonzero allocation and omits the zero-size
deallocation adjustment. The sixteen-byte extent gap and displaced accounting
relocations are consequences of this real semantic/CFG defect, not a cosmetic
allocator tie.

The frozen manifest protects all 18 target function records, all 18 emitted
base function sections, all 27 target-owned runtime data owners, and all 112
base symbols. The following 13 functions are inherited strict exact and must
remain exact:

- `_debug_memory_manager_initialize`;
- `_code_0007ccf0`;
- `_code_0007cd40`;
- `_code_0007cd90`;
- `_code_0007cdc0`;
- `_check_memory_status`;
- `_local_random`;
- `_code_0007ce40`;
- `_debug_check_memory`;
- `_debug_dump_memory_for_file`;
- `_debug_dump_memory_by_file`;
- `_debug_free`; and
- `_debug_dump_memory`.

All 27 January-owned runtime data sections are already strict exact: 1,073
logical / 1,120 padded bytes. Candidate-only discardable owners are the
ordinary `.drectve`, three SELECT_ANY XDK table COMDATs
(`_D3DTEXTUREDIRECTENCODE`, `_D3DSIMPLERENDERSTATEENCODE`, and
`_D3DPRIMITIVETOVERTEXCOUNT`), and one SELECT_ANY `"file"` literal; none is
target-owned or eligible for credit.

The object begins this wave at 13/18 accepted functions, 1,726/2,856 accepted
meaningful code bytes, and 1,792 accepted padded code bytes. A successful shot
projects 14/18 functions, 2,129 meaningful and 2,208 padded accepted code
bytes. It cannot complete or admit the object; `_code_0007cf50`,
`_code_0007cfc0`, `_code_0007d060`, and `_debug_malloc` remain outside scope.

## Claude documentation, history, and tooling reconciliation

All durable Claude material naming this object, function, or relevant tooling
was screened before freezing C1. Claude commit `fae4cc9a` records
`_debug_realloc` in `research/residual_frontier.json` as a 416/400-byte
`UNKNOWN`, `unclassified` residual with `recipe: null` and confidence `NONE`.
It supplies no source recipe, prior candidate, rejection, or closure.

Readable Claude heads either lack the body or retain the baseline
`if (size != 0)`. A Claude documentation/research search found no result-or-zero
candidate or production attempt. Reconstruction commit
`2fc59b8c1b9f31861e60aa27c528ff0a5e32d56f` introduced the same baseline
condition; the later durable debug-memory commits address only dump and free
work. A workspace census found 867 copies of the old predicate and zero copies
of C1. C1 is therefore genuinely novel in all locally available provenance.

The rejected `_debug_malloc` clear-branch candidate documented in
`debug_memory_obj_jonas_malloc_clear_branch_20260825.md` is not combined with
this wave. Claude's tail-merge work says dropping `static` from
`_code_0007cf50` is allocator-inert, so helper linkage stays frozen. The
retracted `0x10715873` register-write interpretation is not used: that field is
an operand-width class and the physical allocator site remains unknown.

Claude's hardened `tools/coff_compare.py`, complete-owner regression manifest,
semantic audit, object-admission audit, parked-function audit, and ordinary
progress tooling remain the authorities. Similarity, raw byte views, semantic
equivalence alone, or comparator exceptions grant no credit.

## Frozen candidate C1

C1 changes only the accounting guard:

```c
if (result != NULL || size == 0)
{
	data_002dcd0c.total_pointer_size += size - old_size;
	if (data_002dcd0c.total_pointer_size > data_002dcd0c.maximum_pointer_size)
	{
		data_002dcd0c.maximum_pointer_size = data_002dcd0c.total_pointer_size;
	}
}
```

This is the direct typed expression of the January CFG and ordinary realloc
semantics. It adds no local, cast, signature, caller, helper, literal, storage,
header, or runtime-data owner. It is readable, defined, typed C and uses no
assembly, `volatile`, barrier, pragma, intrinsic, attribute, forced/no inline,
`register`, raw offset/address, pointer or union pun, inactive-union read,
undefined behavior, synthetic anchor, object-byte patch, comparator exception,
or flag/tool change.

## Acceptance, rejection, and closeout gates

The frozen C1 source and this ledger must pass one XDK `/Zs` syntax-only
invocation without changing the immutable baseline object. A Ninja dry run
must then expose exactly one `CL build/base/source/cseries/debug_memory.obj`
edge. Exactly that edge may receive the wave's sole ordinary candidate compile,
after which the first-shot object is copied immediately under ignored
`build/audit/` before inspection.

Acceptance requires `_debug_realloc` to reproduce January's 403 meaningful /
416 padded bytes, normalized hash, symbol properties, and all 25 ordered
relocation address/type/destination/addend records. The complete baseline
manifest must retain all 13 inherited exact functions and every other code,
runtime non-code, and symbol owner. No unexpected constant, storage, or symbol
owner may appear.

Any difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning, and only this ledger records the rejection. No predicate-order,
De-Morgan, nested-branch, temporary, declaration, linkage, flag, or scheduling
variant follows without genuinely new source or compiler provenance. If exact,
the source and initial ledger are committed; full Halo and libcmt builds,
ordinary/semantic/progress regeneration, semantic/admission/parked audits, the
complete tooling suite, policy/scope and JSON checks, and a clean committed
regression snapshot follow. A literal one-object delete/rebuild must reproduce
the first-shot runtime result, followed by a ledger-only replay commit and a
second literal replay from corrected cumulative HEAD. No amend, push, rebase,
history rewrite, or worktree removal is authorized.

## Production result

The real XDK `/Zs` invocation passed and left both the live and immutable
baseline objects at raw SHA-256
`338379b8df74bc2e5d47a65fc49b54d3f83ad9f33c9e6b021839d0e871854bbb`.
The candidate source then remained frozen at Git blob
`534dac4d3137c828d96ff446e26c7890bbaead0b`. A Ninja dry run exposed exactly
one CL action, and exactly one ordinary production edge compiled
`build/base/source/cseries/debug_memory.obj`:

```text
[1/1] CL build\base\source\cseries\debug_memory.obj
debug_memory.c
```

There was no direct code-producing CL command and no second candidate compile.
The unchanged first-shot object is preserved at ignored path
`build/audit/debug_realloc_accounting_first_shot_20260825.obj`, raw SHA-256
`77795d4137edc7ec0156f6b6de6240e4786ffee9702371d601ee471be0cfd6dd`.

C1 failed closed. It emitted the intended Boolean condition, but VC7 retained
the baseline's broader value topology and produced only two additional
meaningful bytes:

| Owner | Meaningful / padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| January `_debug_realloc` | `403 / 416` | `25` | `d812408198d3b0ffe7e64a41f32db28bceb17093d3b341f8c2c7d75e0f47f1d1` |
| Source-identical baseline | `393 / 400` | `25` | `9597eb98281b48703e2ee47f5fb336e3b1fc3334c444dfc881549ac6dbfe3e47` |
| Frozen first shot | `395 / 400` | `25` | `34f3978b3d21eb6a283f69652224ec8a899a9a0e185a230760b56a6200cb395d` |

At its tail, the first shot tests the result in EDI at `+0x159`, enters
accounting at `+0x161` when non-null, otherwise tests size in EBX at `+0x15D`
and skips accounting at `+0x181` when nonzero. The semantics and short-circuit
order therefore match C1. January instead carries the result in ESI, reaches
its result test at `+0x15F`, reloads the prior size on the null path, tests the
new size in EDI at `+0x168`, and joins accounting at `+0x16C`. The required
sixteen-byte target extent and its earlier ESI/EDI/EBX allocation do not follow
from the predicate alone.

The first shot's first relocation remains at baseline `+55` rather than
January `+48`; its `_code_0007cf50` relocation moves from baseline `+340` to
`+338`, not January `+341`. Its final four `_data_002dcd0c` relocations are at
`+357`, `+363`, `+374`, and `+381`, while January requires `+366`, `+373`,
`+382`, and `+389`. Relocation count and destination multiset alone do not
grant credit.

The complete frozen-manifest regression check is clean: no failures or
warnings, all 13 inherited exact functions in `still_exact`, empty
`newly_exact`, and only `_debug_realloc` in `changed_nonexact`. Thus every
other function, all 27 target-owned runtime data owners, and the protected
symbol/ownership shape remain intact.

C1 was inverse-reverted wholesale. The working source again hashes to baseline
Git blob `61f564e6b90fc9f8ceb5d3e0f7e0a12d5c57f07e`. There was no predicate-order,
De-Morgan, nested-branch, temporary, declaration, linkage, flag, or scheduling
retry. The direct predicate is semantically correct and target-supported, but
this shot proves it is insufficient by itself; another production wave
requires genuinely new evidence for the earlier lifetime/allocation topology.

Because no implementation or metadata change survives, there is no exact
function gain, progress or admission change, full-build closeout, or
committed-state implementation replay. `debug_memory.obj` remains 13/18 exact
functions, 1,726/2,856 accepted meaningful code bytes, 1,792 padded accepted
code bytes, and 1,120/1,120 exact padded data bytes. This documentation-only
record is the complete outcome of the one-shot.
