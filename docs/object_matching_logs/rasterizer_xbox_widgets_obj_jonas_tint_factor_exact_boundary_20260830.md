# `rasterizer_xbox_widgets.obj` tint-factor exact retention boundary

## Result

The natural January-order implementation of
`__rasterizer_widget_set_tint_factor` emitted strict exact on its sole
code-producing production compile. All 64 padded bytes, all six ordered
relocations, and the normalized SHA-256 match January. The source is not
retained: adding the external COMDAT immediately before the already accepted
z-buffer sibling changes that sibling's full COFF ownership fingerprint. The
candidate was inverse-reverted and the baseline object restored, so this lane
claims no matching credit.

| Measurement | January | Frozen candidate |
| --- | ---: | ---: |
| Meaningful bytes | `59` | `59` |
| Padded bytes | `64` | `64` |
| Relocations | `6` | `6` |
| Normalized SHA-256 | `a02aea237967d57d3bb8e05c2cfd8da465321d0df5a9cc8c446526db59ea0b60` | `a02aea237967d57d3bb8e05c2cfd8da465321d0df5a9cc8c446526db59ea0b60` |

The January split-object SHA-256 is
`c3055aafc2db35a6ae2565112df0dbae054f6ff79bd26f1ee3f7d2878f840112`.
The frozen candidate object's SHA-256 is
`bd2d19ff0e57f8aec10eea5ccc4cd4c54adf98928eb716c1e541da35d04d5862`;
it is preserved locally as
`build/audit/rasterizer_xbox_widgets_tint_first_candidate.obj`. The untouched
baseline object SHA-256 is
`da88c6fe0df35f79f285b6d886726a7ba7dfa0c8891b6da43a2e562fb991bfd3`.
Raw object hashes include debug metadata; the complete normalized packet and
resolved relocations are the equality authority.

## Provenance and one-shot source

The lane began from canonical commit
`e1adae772227b3a20048e368d3bc976b2a144423`. The campaign and tooling
documentation, current Xbox Widgets ledger, repository history, current
source/header topology, January disassembly, and HCEA exact-name donor were
read before mutation. No current ledger or ref-reachable history records a
prior production candidate for this symbol. The HCEA donor proves the public
`float` parameter ABI but describes a later platform implementation that
packs a global color; January is authoritative and instead proves the XDK
vertex-data call below.

The complete January packet asserts `global_d3d_device` at source line
`0x13E`, then calls the typed XDK wrapper with vertex register 10, the incoming
float, and zero. The frozen readable C89 body was:

```c
void _rasterizer_widget_set_tint_factor(
	float tint_factor)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_widgets.c",
		0x13E,
		global_d3d_device);
	D3DDevice_SetVertexData2f(10, tint_factor, 0.0f);

	return;
}
```

The first production invocation stopped in parsing because the initial
declaration used the repository alias `real`, which the translation unit's
existing include closure does not expose. It emitted no object or candidate
bytes and left the authenticated baseline object intact. The sole
declaration-only correction used the definition's proven plain `float` type;
the body and placement remained byte-for-byte frozen. The next invocation was
the only code-producing candidate compile and matched immediately. There was
no expression, topology, placement, include, compiler-option, or body retry.
The candidate source Git blob was
`5fb13a4acccc6fea4659c0cd058a621579d18c28`; inverse restoration returned the
source exactly to canonical blob
`6a38cf49a78e60f5cf37d906fed886d2e2a64fa3`.

The ordered relocation packet is:

| Offset | Type | Resolved destination |
| ---: | --- | --- |
| `+4` | DIR32 | `_global_d3d_device` |
| `+20` | DIR32 | target source-path string |
| `+25` | DIR32 | target `global_d3d_device` string |
| `+30` | REL32 | `_display_assert` |
| `+37` | REL32 | `_system_exit` |
| `+53` | REL32 | `_D3DDevice_SetVertexData2f@12` |

## Frozen whole-TU boundary

Direct hardened comparison of all three reconstructed functions in the
candidate object is green:

- `__rasterizer_widget_end`: 16 padded bytes, zero relocations, normalized
  SHA-256 `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`;
- `__rasterizer_widget_set_tint_factor`: the exact 64/6 packet above; and
- `__rasterizer_widget_set_zbuffer_enable`: 64 padded bytes, six relocations,
  normalized SHA-256
  `ae8498cc929333a837b1803c4e0ee8cf53e3119dc96e905aaf12e1924aca052e`.

The fail-closed pre-edit regression manifest nevertheless reports the
z-buffer sibling's accepted full fingerprint as changed and reports the
expected symbol-set addition. `changed_nonexact` remains empty; the runtime
bytes and resolved relocation packet of the inherited sibling remain strict
exact. This is a COFF numeric-section ownership change caused by inserting the
new external COMDAT in natural January order before the existing z-buffer
definition. The campaign has no general reviewed transition recipe for this
insertion, and no one-off adjudication or comparator exception is allowed.

The candidate was therefore removed wholesale. After restoring the source and
baseline object, the same regression manifest is fully green: widget end and
z-buffer are both `still_exact`, with no failure, warning, newly exact owner,
or changed nonexact owner. Do not retry the exact body, its type, or physical
placement. Reopen retention only after a repository-wide fail-closed ownership
model lawfully represents external-COMDAT insertion, or after enough of the
original translation unit is reconstructed to restore January section
topology without weakening inherited evidence.

## Validation and policy

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,949 functions evaluated, 4,832 semantic exact,
  4,842 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: 0 candidates, 0 contradictions, 0 revocations.
- Park validation: 12 active, 0 stale, 0 invalid.
- Tooling suite: 212/212 tests pass.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Regenerated ordinary report SHA-256:
  `05caa910cb8061bd3017ec2a439e6a32bfedae44bca09b67a635fb5386d72490`.
- Regenerated semantic report SHA-256:
  `654284f732a8504ea6f310037d2db0d877c0a907ad35d27819dd5a60f90936a3`.

The rejected body used readable typed C89, one parameter per line, and an
explicit return. It used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, raw address or offset, representation
pun, undefined behavior, synthetic anchor, byte forcing, object patch,
compiler/tool exception, alternate compiler, or alternate flags. Only this
additive boundary ledger is retained; nothing is pushed.
