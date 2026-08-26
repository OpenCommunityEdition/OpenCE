# `tif_dirwrite.obj` rational-array declaration-order one-shot (2026-08-25)

## Frozen scope and provenance

This bounded wave starts from clean cumulative integration commit
`ed2774c60fb128d89f38207d701203aa165fae65` in isolated branch and worktree
`jonas/tif-dirwrite-declaration-order-20260825`. The only permitted production
source is `source/bitmaps/libtiff/tif_dirwrite.c`; this new Jonas ledger is the
only other permitted tracked path. Every header, configuration, semantic
exception, parked record, object option, and Units path is frozen. Exactly one
fixed candidate may receive one ordinary code-producing compile. There is no
post-emission source spelling change, retry, declaration permutation sweep,
tuning pass, alternate candidate, compiler change, or flag change.

The pinned compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`. Baseline source blob
`361094f23cc9a708180418d697478d5561d5d2d6`, configuration blob
`8825927921a7abf9ad1a30521c3c80df265cdb3a`, compiler SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
January split-object raw SHA-256
`c694573170454c8e86f939ff7a420c31f6d19727c28260977d067dba383c06a9`,
and cumulative base-object raw SHA-256
`b49639777c50b366136fa67632dec21e306e10163c78ec32f1371c5cf024a5e0`
are frozen before source emission.

The ignored one-unit baseline manifest is
`build/audit/tif_dirwrite_decl_order_baseline_20260825.json`, SHA-256
`409f1ce8e4c9e0145d2e71e2c61d0ba329100baa8359cba38c8ff87b15f88970`,
and pins the full starting commit. The immutable copied baseline object is
`build/audit/tif_dirwrite_decl_order_baseline.obj`, with the base-object hash
above. The ordinary object edge reported no work before this ledger or the
candidate source existed.

## January target and protected manifest

The single requested function is the file-local `TIFFWriteRationalArray`,
owned as `_code_000561f0`:

| State | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| January target | 143 | 144 | 8 | `e73a1566b46fd8abc4bbea20127bd74231e6bbc541e158d9ba469c51602bc40f` |
| Cumulative base | 143 | 144 | 8 | `fb123b0e7990778c1a1eb50b0ee82b69d9f4451dc2d735e3f044d67ee3fde1b6` |

All eight relocation addresses, types, destinations, and addends already
agree: `+0x15` to the source-path literal, `+0x2e` to `_debug_malloc`,
`+0x48` to `10000.0`, `+0x4e` to `0.5`, `+0x53` to `__ftol2`, `+0x6f` to
`_code_00055dd0`, `+0x79` to the same source-path literal, and `+0x81` to
`_debug_free`. The instruction graph and padded extent agree. The fifteen
differing bytes express one local-register role difference plus its epilogue
schedule:

- January uses EDI for `n`, then `dir`, then `status`, ESI for the allocation
  pointer `t`, and EBX for `i`, then `tif`;
- the cumulative base uses ESI for `n`, then `dir`, then `status`, EDI for
  `t`, and the same EBX roles; and
- January moves the return status to EAX before restoring EDI, while the base
  restores EDI before moving the ESI status to EAX.

The three inherited exact code owners are protected:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_TIFFSetupShortLong` | 71 | 80 | 0 | `67a8a0cb2c8819ade306c07797343ea4084f6268556d8548ae65500736374055` |
| `_code_00055e60` | 361 | 368 | 18 | `aead5f63c387f18caeba013e0c2076969c8a99c883a0ed035e33f77e5618d760` |
| `_TIFFSetupShortPair` | 101 | 112 | 1 | `3372e4c118e5f1d1cee8d48f53c89881962fc4cede988d4f80ee5673cf72c701` |

All 606 owned data bytes are exact and protected: fourteen `.rdata` owners
totaling 492 bytes, `_data_002b8688` at 110 bytes, and `_bss_0031be50` at
4 bytes. The current ordinary census is 3/14 exact functions and 533/4,083
accepted meaningful code bytes, with 606/606 data bytes. A successful shot
projects 4/14 and 676/4,083 code bytes, leaves data unchanged, and does not
claim whole-object completion.

Only one emitted same-TU caller exists, `_code_000562c0`; its call relocation
to `_code_000561f0` is already exact. A second source call is excluded with
disabled `COLORIMETRY_SUPPORT`. C1 changes neither signature nor caller.

## Claude documentation and tooling reconciliation

All durable Claude material that names this object or function was screened
before this shot: the corrected register-rename census, residual frontier,
csplit-artifact accounting, flag-provenance census, code-generation
dictionary, and the relevant classifier implementation.

Claude's historical residual frontier labeled `_code_000561f0` as proven R3
`alias-global-ownership`. That label is a detector false positive, not an
ownership fact. The old R3 implementation fires when any target instruction
contains a scale-eight SIB and any candidate instruction contains a
scale-eight LEA or SHL; it does not correlate the two sites. Here both objects
have the same allocation-size `lea` and the same scale-eight heap-buffer
stores. `t` is a local allocation, all relocation identities agree, and no
aggregate global is involved. The source blob and function fingerprints are
unchanged from the Claude measurement, so no later source change explains the
classification away.

The corrected register census omitted this function from its strict global
bijection list because the unchanged save order and differently scheduled
return restoration defeat a whole-function text bijection. Its broader TIFF
finding remains relevant: twelve flag variants plus the baseline did not move
this register family. That experiment varied `/Ox`, `/Ob1`, `/Os`, `/Gy-`,
`/GF-`, `/Oi-`, `/G5`, `/GB`, `/Gs-`, `/Op`, and packing/float options; it did
not test source declaration order. The flag-provenance record agrees with the
current `__ftol2` relocation and supplies no alternate-flag explanation.

The csplit-artifact accounting closed the object's earlier
`TIFFLinkDirectory` naming/CRT debt and reports no open comparator artifact.
The typed C2 IR width-class field is not treated as a physical-register claim.
No debugger branch, raw-byte view, semantic-only equivalence, fuzzy compare,
or comparator exception is used. Hardened `tools/coff_compare.py`, including
relocation destination and ownership semantics, remains authoritative.

Novelty was audited separately across all durable history and accessible
worktrees. Four unique historical blobs across eight commits, plus more than
1,400 accessible worktree copies, retain the vendored libtiff 1.16 order
`int i, status;` then `u_long *t;`. No durable candidate, ledger, or compile
record contains the reverse order. The one-shot is therefore genuinely
untried under campaign evidence standards, but it is deliberately classified
as low confidence: declaration order is often allocator-inert, and the
historical upstream spelling supports the current order rather than C1.

## Frozen candidate C1

C1 changes only the order of two uninitialized automatic declarations:

```c
	u_long *t;
	int i, status;
```

The hypothesis is bounded and direct. The current integer-first spelling
coalesces `n`/`dir`/`status` into ESI and assigns `t` to EDI; pointer-first may
reverse those priorities and thereby reach January's roles and tail schedule.
No value, type, lifetime, expression, statement, signature, global, literal,
or control-flow fact changes.

C1 is readable, semantically equivalent C89 in the established vendored-file
style. It uses no assembly, `volatile`, barrier, pragma, intrinsic, attribute,
forced/no inline, `register`, raw offset/address, pointer or union pun,
inactive-union access, undefined behavior, synthetic anchor, object-byte
patch, or flag/tool change.

## Acceptance, rejection, and closeout gates

After this ledger and exact C1 source exist, the real compiler receives one
`/Zs` parse-only invocation, proven not to modify the baseline object. Exactly
one ordinary Ninja edge may then compile
`build/base/source/bitmaps/libtiff/tif_dirwrite.obj`. The first-shot object is
copied unchanged under ignored `build/audit/` before inspection.

Acceptance requires `_code_000561f0` to match the January meaningful/padded
size, normalized hash, symbol properties, and all eight relocation records.
The three protected exact siblings must remain strict exact. A baseline-to-shot
comparison must keep every other code/data owner byte- and relocation-exact,
and the object may gain no unexpected owner. The one-unit regression gate must
report exactly `_code_000561f0` in `newly_exact`, the three siblings in
`still_exact`, no changed nonexact owner, zero failures, and zero warnings.

Any deviation rejects C1: the two-line source change is inverse-reverted
wholesale with no second compile or tuning, and only this ledger records the
miss. If C1 is exact, the source and initial ledger are committed, followed by
full Halo and libcmt builds, fresh ordinary/semantic/progress reporting,
semantic, admission and parked audits, the complete tooling test suite,
policy/scope and JSON checks, and a clean committed regression snapshot. A
literal one-object delete/rebuild must reproduce the first-shot result. Replay
facts are then appended in a ledger-only commit and replayed once more from
corrected HEAD. No amend, push, rebase, history rewrite, or worktree removal
is authorized.

## Production result

The real XDK `/Zs` invocation passed and left the cumulative base object at
raw SHA-256
`b49639777c50b366136fa67632dec21e306e10163c78ec32f1371c5cf024a5e0`.
The source then remained frozen at candidate Git blob
`3871b2e05a00bc8e19a36645165bdc7aed2e8ed4`. The Ninja dry run exposed
exactly one CL action, and exactly one ordinary production edge compiled
`build/base/source/bitmaps/libtiff/tif_dirwrite.obj`. There was no direct
code-producing CL command and no second candidate compile.

The unchanged first-shot object is preserved at ignored path
`build/audit/tif_dirwrite_decl_order_first_shot.obj`, raw SHA-256
`12c5aaa321d42f14909ebd980079242602062851f6960afc8dd268b0a4d15d47`.
The requested function did not move: it remained 143 meaningful / 144 padded
bytes, eight exact relocation records, and baseline normalized SHA-256
`fb123b0e7990778c1a1eb50b0ee82b69d9f4451dc2d735e3f044d67ee3fde1b6`.
It therefore failed January's required normalized SHA-256
`e73a1566b46fd8abc4bbea20127bd74231e6bbc541e158d9ba469c51602bc40f`.
The declaration swap is allocator-inert for this real translation unit.

The three protected siblings remained strict exact. An independent hardened
baseline-to-first-shot comparison covered all fourteen code owners and all
sixteen non-code owners: 30/30 were equal in normalized bytes, padded extent,
flags, ownership, and ordered relocation semantics, with zero unequal owners.
The different raw object digest is compiler debug/timestamp metadata only;
there is no runtime change, regression, unexpected owner, or partial result to
retain.

C1 was inverse-reverted wholesale with no second compile, alternate spelling,
permutation sweep, or tuning. The working source again hashes to baseline blob
`361094f23cc9a708180418d697478d5561d5d2d6`. Because no implementation or
metadata change survives, there is no exact-function gain, no progress or
admission change, no whole-object claim, and no committed-state implementation
replay. `tif_dirwrite.obj` remains 3/14 exact functions, 533/4,083 accepted
meaningful code bytes, and 606/606 exact data bytes. This documentation-only
closeout is the complete durable record of the one-shot and retires bare local
declaration order for `_code_000561f0` absent genuinely new provenance or a
new ordinary-C lifetime/definition topology.
