# collision_features.obj matching log

## Result

- Upstream provenance: `punpckhdq/halo` commit `1aa21fd12d56a6fe737cd0e9ab4516e31f3a3f47` (`collision_features (#48)`).
- Compiler: XDK 3911 CL 13.00.9254.1 with the repository flags `/O2 /Oy- /DDEBUG /Dxbox`.
- Strict result after adaptation to the current campaign trunk: 20/20 functions pass `tools/coff_compare.py` (`section_infos_equal`).
- Upstream whole-object report: 6,929/6,929 meaningful code bytes and 896/896 data bytes; `.text` and `.rdata` are both 100%.
- No assembly, volatile codegen coercion, `__forceinline`, byte patching, undefined-behavior trick, compiler-flag change, or comparator exception is used.

## Integration and type audit

The upstream implementation was compiled and measured in a pristine worktree before any transplant. All 20 functions passed the hardened comparator, including relocation destination identity. The full upstream report also proved the 896-byte `.rdata` ownership and content.

The current campaign already had partial collision-feature structures. The completed upstream layout was retained and guarded with compile-time size/offset checks:

- `collision_feature`: `0x0C`
- `collision_sphere`: `0x1C`
- `collision_cylinder`: `0x28`
- `collision_prism`: `0x68`
- `collision_feature_list.spheres`: `0x08`
- `collision_feature_list.cylinders`: `0x1C08`
- `collision_feature_list.prisms`: `0x4408`

The source follows the project formatting rules: one parameter per line, explicit returns, no raw `tag_get`/`object_get` casts, and no gratuitous alignment directives. Several boolean collision routines retain multiple returns because that is the January control-flow shape; forcing a single exit would change the object, so this is one of the house rule's documented exceptions.

## Current-trunk codegen interaction

The pristine upstream patch was exact, but its first current-trunk build left only `_collision_prism_test_vector` nonexact. Size and relocation identity were already exact (`0x290`, 11/11). Two independent instruction-order differences were isolated:

1. The `vn` dot product evaluated the `j` product before `k`; January evaluates `k`, then `j`, then `i`.
2. At the inlined `point_from_line3d` boundary, `fstp p3d.z` and `lea &p3d` were transposed.

The accepted readable controls are:

- keep the upstream no-parentheses `point_from_line3d` body (`v*t + p`), which restores the January inline-boundary scheduling;
- accumulate `vn` sequentially in January order (`k`, then `j`, then `i`), which preserves the observed x87 evaluation order.

Together these restore strict equality for all 20 functions.

## Rejected forms

- The unadapted donor body under the current shared header: exact size/relocations, but two instruction-order regions differed.
- Reversing the one-line dot-product terms alone: VC7 reassociated them back to `j`, `k`, `i`; no improvement.
- Sequential `k`/`j`/`i` accumulation while retaining the parenthesized inline helper: fixed the dot product but left the `fstp`/`lea` transposition.
- A named pointer holding the return of `point_from_line3d`: optimized to the same remaining transposition.
- Longhand first or second point-from-line expansion: either had no effect on the transposition or introduced repeated `fchs` operations and a six-byte regression.

## Admission gate

Before admission, regenerate the full report on the integration worktree, compare the strict accepted-function ledger against the pre-transplant baseline, and require zero lost entries. Any data or sibling-function regression rejects the object despite its own 20/20 result.
