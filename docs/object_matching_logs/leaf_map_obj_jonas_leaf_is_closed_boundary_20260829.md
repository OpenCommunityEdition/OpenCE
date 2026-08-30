# `leaf_map.obj` leaf-closure predicate boundary (2026-08-29)

## Result

`_leaf_map_leaf_is_closed` received one frozen, readable, typed C89 candidate
compile.  It is **not** strict and production source is fully restored.  The
candidate reproduces the target's meaningful and padded extents, both ordered
relocations, and 95 of 96 padded section bytes.  The sole difference is one
conditional-jump opcode at section offset `0x40`.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful bytes | 88 | 88 |
| Padded bytes | 96 | 96 |
| Ordered relocations | 2 | 2 |
| Normalized SHA-256 | `76b984bd544d6a9fdd7c047f5d18d983bf4fc58a1f43518cf8e7a0f623857b12` | `5c66852ed123b808ac9c35e9feba2ff47dffa34379b8174cfd89ca9ec0ce2218` |

The complete candidate object SHA-256 was
`d6402bbc747ce592432f96a9ebe9791a1c52b0eaa8100446a6e68f1e88fcf7a3`.
The owner remains unwritten.  Do not retry it through a sign-bit macro,
comparison spelling, Boolean inversion, branch reversal, or declaration
change.  Reopen only if new authenticated source fixes the original
sign-condition expression before compilation.

## Evidence and ownership

January `source/structures/leaf_map.obj` is authoritative.  Its complete
object SHA-256 is
`03642b1a1367d576979c8c0fdae4208bf2ecfaa0b58f785c508817f8a989ba02`.
Its owner disassembly fixes this natural typed sequence:

- mask the requested leaf designator with `LONG_MAX`;
- obtain a 24-byte leaf from `leaf_map::leaves`;
- walk the leaf's `portal_designators` block with a signed 16-bit index;
- return false when any 32-bit designator is nonnegative; and
- otherwise return the low byte of the block count.

The clean semantic donor is HCEA
`src/blam/structures/leaf_map_leaf_is_closed.c` at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, Git blob
`f6b75a2aaf9749b9284569fe95599fb8164a8db9`, and file SHA-256
`84a58c5481eeeea40b6c8cd826b8491779b41c12a2aa0e85e7dc3e7bdc72df97`.
Its independently typed `map_leaf.h` is blob
`473e2ef443ce31aa1bcb14c55de944dca255df29`, file SHA-256
`580f3958ca3ccaa7dc148eafce3f3ede5395648cf81ad13449d31958a4997dca`;
its `leaf_map.h` is blob
`f2ca0cdaeb2ed6264cb6d752eb6c29dfd9d8ee0d`.

The complete prior Leaf Map ledger, current source/header, HCEA donor and
typed headers, repository history, January disassembly, and hardened
relocations were read before editing.  No prior source or ledger attempt was
found for this owner.

The candidate was link-closed.  Both relocations are public REL32 calls to
`_tag_block_get_element_with_size`, at exact section addresses 24 and 53.
The first call uses the existing `leaf_map::leaves` block with a 24-byte
element; the second uses the named `portal_designators` block with a 4-byte
element.  No private helper, literal, storage owner, or register ABI is
involved.

## Frozen candidate

The one emitted candidate added a typed 24-byte `struct map_leaf` containing
the named `faces` and `portal_designators` tag blocks, plus a compile-time size
check.  Its C89 body used `TAG_BLOCK_GET_ELEMENT`, a signed `short` loop index,
and a named 32-bit portal-designator pointer.  It matched January's complete
instruction sequence except for the one sign-condition opcode.

The source contained no assembly, `volatile`, `register`, pragma, intrinsic,
compiler attribute, optimizer barrier, raw address, raw offset,
pointer/integer reconstruction, representation pun, undefined behavior,
synthetic anchor, or byte-forcing device.  A non-emitting production-argument
`/Zs` check passed before the sole object-producing compile.

## One-byte fixed boundary

At offset `0x3E`, both target and candidate emit `test ecx, ecx` for the
loaded portal designator.  At offset `0x40`:

- January emits opcode `0x79`, `jns +0x10`;
- the natural HCEA comparison `*portal_designator >= 0` emits opcode `0x7D`,
  `jge +0x10`.

The destination, fallthrough, and every remaining byte are identical.
`jns` tests only the sign flag, whereas `jge` tests the signed-order relation
between sign and overflow flags.  The preceding `test` clears overflow, so
the two branches are behaviorally equivalent here, but strict byte identity
still fails.  The evidence does not authenticate whether January used a
sign-bit test macro or another source form that naturally selected `jns`.
Changing the comparison after seeing the opcode would be byte tuning, so no
retry occurred.

## Restoration

The inverse-revert is byte exact:

- `source/structures/leaf_map.c` restored Git blob:
  `182a6c5a6e8518aaf675b2e6bb134346bc6112a6`;
- `source/structures/leaf_map.h` restored Git blob:
  `30b1373e1461236fb35ec534f2d36631f65aa9ea`.

The focused restored-object gate reports one exact, zero residual, and 24
unwritten functions.  The inherited exact `_leaf_map_delete` remains strict.
This additive ledger is the only retained change from the shot.

The full validation packet passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` complete successfully;
- semantic report: 470 units, 4,918 functions evaluated, 4,801 semantic
  exact, 4,811 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,772/11,060 functions, 606,426/2,198,102 code
  bytes, and 1,856,050/4,176,062 data bytes;
- admission audit: zero candidates, contradictions, and revocations;
- parked audit: 12 active, zero stale, and zero invalid;
- test suite: 212 passed;
- protected Units sentinel: 1,920 padded bytes, 87 ordered relocations, and
  strict normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
- `build/semantic_report.json` SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`.
