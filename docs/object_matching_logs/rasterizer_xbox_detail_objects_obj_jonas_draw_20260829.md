# `rasterizer_xbox_detail_objects.obj`: draw owner (2026-08-29)

## Result

`__rasterizer_detail_objects_draw` is strict target-COFF exact. The function
contributes 1,136 padded code bytes and 52 ordered relocations with normalized
SHA-256
`3e00e754672e61f743c26496fb81376ef7c6011a20994814483485db6e8cb2bd`.

The object advances from 1,824/4,240 to 2,960/4,240 exact padded bytes and
from 9/20 to 10/20 exact functions. It remains honestly partial with ten
unwritten owners. The strict repository board advances to 634,727/1,922,669
bytes and 4,624/8,246 functions; fully exact objects remain 274/619.

Direct `tools/coff_compare.py` comparison proves equal padded bytes, normalized
hash, and relocation address/type/destination/addend records. The January split
object raw SHA-256 remains
`0830A98EC28D64C17A59B271DEEE4D2A051D552F71CFC15E9B0AE5A514C50FD5`;
the integrated rebuilt object raw SHA-256 is
`003B4ABA2DF547B9F8852CFA9DB3923DE9EBE3A5D39DE95A9AA5738CFD0AD2DE`.
Raw whole-object hashes are provenance only.

## Recovery evidence

The implementation reconciles four independent evidence classes:

- the January Xbox COFF and its assert/error strings;
- `work/halocea-reference/src/_rasterizer_detail_objects_draw.c`;
- the recovered Xbox topology in
  `work/stianeklund-halo-reference/src/halo/rasterizer/xbox/rasterizer_xbox_decals.c`;
- the typed detail-object layouts in
  `work/research-cache-demon/source/structures/detail_object_definitions.h`,
  plus the repository bitmap-group and XDK rasterizer conventions.

The recovered path resolves each layer's collection and bitmap group, binds
and configures the texture, selects the collection shader permutation, builds
the sixteen type constants and 128 sprite-frame constants, uploads both
constant arrays, then supplies cell origin/Z-reference constants and submits
the quad-list vertex ranges. Every tag and tag-block access remains typed.

## Compiler-sensitive source topology

The first typed-C reconstruction already had the exact 1,136-byte extent,
relocation identity, calls, control flow, and rendering arithmetic. Two
source-authentic lifetime details closed the remaining compiler differences:

- keeping the sequence definition as the scoped typed owner of its `sprites`
  block selects January's `lea` member-address form;
- pairing the per-layer loop counter and bitmap pointer in the per-layer state
  gives each genuine value its January stack lifetime without dummy storage,
  aliasing, or raw layout access.

The last residual was a dependency-free transposition of `fstp [esp+8]` and
the independent X-coordinate shift. The XBE's own error string preserves the
written expression surface as
`(cell->cell_z*(real)DETAIL_OBJECT_CELL_SIZE)`. Restoring those outer
parentheses around the ordinary floating expression changes only VC7's inlined
XDK scheduling and produces the exact January instruction order. This is the
measured inline-expression-surface control described by the campaign
playbook, not an optimizer barrier or machine-code forcing device.

The D3D success paths use the same normal `>= 0 && success` topology already
validated in `rasterizer_xbox_text.c`; the stock XDK inline wrappers emit the
target out-of-line D3D calls naturally.

## Policy boundary

The retained implementation is C89-compatible typed C with declarations at
the top of each block, one parameter per line, and an explicit terminal
return. It adds no assembly, `volatile`, `register`, pragma, attribute,
intrinsic, optimizer barrier, raw address/offset access, representation pun,
undefined behavior, dummy emission anchor, object patch, compiler/flag change,
semantic flag, comparator exception, admission exception, or park. Units,
Matrix Math, Vehicles, and all protected AI/game-engine sources are untouched.

## Validation

- Selected campaign gate: one exact, zero residual, zero unwritten.
- Whole-object campaign gate: ten exact, zero residual, ten unwritten.
- Direct COFF comparator: exact 1,136 bytes, 52 relocations, and normalized
  hash.
- Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` graph: pass.
- Semantic audit: 470 units, 4,765 functions evaluated, 4,643 semantic exact,
  4,653 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid.
- Complete tooling suite: 205 passed.
- Clean-commit regression snapshot/check: all ten accepted owners remain exact,
  with no changed nonexact sections, failures, or warnings.
- `git diff --check` and the scoped prohibited-token/raw-address audit: pass.

## Next bounded wave

The object has ten anonymous owners left. The 544-byte and 432-byte owners are
stock-XDK render-state families and should be approached through exact wrapper
identity. The remaining non-wrapper bodies are 96 and 80 bytes; the six tiny
owners total 128 bytes.
