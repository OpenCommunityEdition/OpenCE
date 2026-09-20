# AI debug independent review: authenticated corrections, zero exact credit

The final scratch candidate `scratch/astra-ai-debug-five-point-vision.c` is
admissible as an incremental source correction at **zero exact credit**. Its
SHA-256 after newline normalization is
`6ad32ff1bb3303b7a0578769c525d1711e6ae170ac950700d0197491e24fa666`.
It restores an actual marker-point copy and the actual vision-cone accumulation
graph. It does not establish an exact function or whole-object admission.

I independently rebuilt the source from frozen commit
`905e5e3b41591af98c82accf3f57fc38118c2e7f` and the proposed candidate, each into
scratch through the unchanged gate. The durable comparison is
`scratch/astra-alias-wave2-ai-debug-review.json`; reproduction is
`python -B scratch/astra-alias-wave2-ai-debug-review.py`. This review performed
no production, header, configuration, Ninja, or commit operation.

## Fresh machine evidence and source meaning

`scratch/astra-alias-wave2-ai-debug-primary.py` freshly reads the original PE,
its RTC descriptor, its helper thunk and implementation, and the January COFF.
Its output is `scratch/astra-alias-wave2-ai-debug-primary.json`. This supplements
the worker's actual Ghidra output and source patch; it does not depend on a
decompiler's inferred local types. The PE SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`,
timestamp 1606092379. Its relation to January's source revision remains
unverified. January code and relocations are the final authority.

The marker's additional point is real storage with address uses. January
copies the three base-point components into `ebp-0x114/-0x110/-0x10c` at
`+0x1140..+0x115a`, then takes that aggregate's address at `+0x11b7` and
`+0x1264`. Fresh PE RTC descriptor `0x4a4148` has 94 protected records. Its
records 18 through 22 identify `mid_point`, `p0`, `p1`, `p2`, `p3`, each 12
bytes at offsets -756, -776, -796, -816, -836. Raw PE code
`0x49dbf9..0x49dd43` copies `base_point` into `p0`, computes the top and side
vertices, and draws `p0->p1->p2->p3->p0`. The candidate expresses that graph.
The copy is neither self-assignment nor declaration filler: it is initialized,
its address is used twice, and its storage exists in January. January's later
reuse of those three slots for strings does not negate the earlier point.

The selected marker patch preserves the already present arithmetic expressions
for the center and side points. It does not introduce new manual helper
expansion. The later PE spells these computations as calls; that difference is
recorded, not presented as proof that the retained expression spelling was the
original January source. The worker's all-calls marker variant was measured
and excluded. Neither variant earns credit. The new copy and vertex identities
are supported independently of the spelling choice.

The vision change restores an observable sequence of floating-point operations.
January relocation `+0x55d2` names `_global_zero_vector3d`. Three loads and
stores initialize the direction element, followed by three basis contributions
stored back into it at `+0x5608..+0x5689`. Fresh PE reads confirm pointer
`0xa31e20` addresses `0x996bb0`, whose 12 bytes are zero; raw instructions
`0x4a3648..0x4a365a` copy them. Calls at `0x4a36b0`, `0x4a370c`, and
`0x4a3768` use the same element as input and output, with the looking, left,
and up vectors in that order and headspace components i, j, k. RTC records 86
and 89 identify the 48-byte `direction_vector` and 12-byte `headspace_vector`.

All three calls target thunk `0x409ab1`, independently decoded as a jump to
`0x42e0d0`. That implementation computes each component as `v*t+p`, writes
the result, then proceeds to the next component, and returns the result pointer.
It is the point-from-line operation, not an assumed identity based on a name.
Initialization precedes every in-place read. The point/vector cast uses the
existing owner's three-real, 12-byte representations and the existing project
idiom also used in `real_math.c`. No helper, foreign prototype, raw-address name,
new type facade, uninitialized input, or out-of-range access is introduced.

This source change is not claimed to be numerically interchangeable with the
old single-expression formula for every floating-point input: initializing and
storing each accumulated contribution is precisely the authenticated operation
graph. No global arithmetic or aggregate-copy law follows from this case.

## Independent controls and ownership

The two fresh gates both report 57 exact target functions and three residuals
out of 60. All 57 exact names are saved in the independent JSON. Only
`_ai_debug_render_actor` changes among all 78 candidate code owners; the other
77 sections, including the other two residuals, remain identical. There are no
new or lost code owners. The complete named-definition inventory is 491 before
and after, with identical names, offsets, storage classes, types, section names,
and flags.

All 408 runtime noncode sections retain their bytes, relocations, symbol
definitions, flags, and section auxiliary records. This count excludes 16
debug sections; it is consistent with the worker's 424 total noncode sections.
All five COMMON definitions and their sizes remain unchanged: `_ai_debug`
547628, `_global_ai_debug_drawstack_height` 4, last/next positions 12 each,
and `_global_temporary_render_color` 16. Debug object-name records differ only
in the gate's temporary object path. Their raw decoded records and byte offsets
are retained in the JSON.

The inherited `_point_from_line3d` definition is still 48 bytes with zero
relocations and normalized SHA
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`.
Its flags, SELECT_ANY selection byte 2, and complete section auxiliary record
`30000000000000008e305989000002000000` are identical. The correct check for
this TU is preservation of that inherited definition; a blanket absence check
would falsely reject the baseline. All 18 inherited target-absent code owners
remain unchanged and are enumerated in the JSON. No new helper emission occurs.

`scratch/astra-alias-wave2-ai-debug-fake.json` reports one file scanned and zero
leads. That lexical scan supplements, rather than replaces, the source and
machine review above.

## Admission boundary

| Object state | Actor padded section | Relocations | Normalized SHA prefix |
|---|---:|---:|---|
| Frozen HEAD | 24848 | 1903 | e20fcee6e777 |
| Candidate | 24976 | 1905 | 62c77e81f1dd |
| January | 24976 | 1905 | c236780ab71e |

Equal section extent and relocation count do not establish equal normalized
code, relocation addresses, or destinations. The candidate still has an
`0x81c` frame where January has `0x810`; the first table begins at 24588 rather
than 24576. The unmodified alignment retains these differences. Neither
embedded tables nor their parent section are separate new-credit owners.
The actor remains residual, as do `_code_00039990` and `_code_0003a910`.
There is no new strict function, meaningful byte, padded byte, park retirement,
or whole-object admission from this candidate. The remaining allocator and
x87 lifetime differences require new source-context evidence; this review
does not authorize a speculative fifth shape.
