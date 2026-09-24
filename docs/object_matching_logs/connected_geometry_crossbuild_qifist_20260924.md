# `connected_geometry.obj`: independent build and compiler-flag check (2026-09-24)

This is zero-credit research. Production `source/tool/connected_geometry.c`, its header, symbols and both parks remain unchanged. January still gates at **7/10 exact**, with `_connected_geometry_find_or_add_vertex`, `_connected_geometry_find_or_add_edge` and `_triangle_coplanar` residual. The previous closeout and park entries remain the authority for candidate source and owner holds.

## Controlled `/QIfist` test

I compiled the unchanged TU with the `build.ninja` cflags and again with only `/QIfist` added, using `tools/campaign/gate.py --all`. All ten gate rows were identical. For the three residuals, normalized function hashes, padded sizes, relocation counts/destinations/addresses and `compare_function_distance.py` byte distances were identical:

| Function | Padded size | Differing normalized bytes | Hash prefix |
| --- | ---: | ---: | --- |
| `_connected_geometry_find_or_add_edge` | 256 (January 240) | 239 | `6c26dc4c` |
| `_connected_geometry_find_or_add_vertex` | 192 | 25 | `e42aac4d` |
| `_triangle_coplanar` | 384 | 10 | `4a01cf6d` |

The large edge distance includes its 16-byte size/layout gap; it is not an instruction count. `/QIfist` is decisively inert for this TU and should not be re-probed as the x87 remedy.

## Separate-build evidence

Supplied `C:/Users/isabe/Downloads/haloleak2024/halo1/h1_tags/tool_symbols.exe` has SHA-256 `079d20d7792e9565b24ab2cff8115f76a6e48749a8e8ffd190f57954495f0a97`. This 2020 Win32 Tool build is **not** the January Xbox target. Its debug directory refers to an external `tool_symbols.pdb` that is absent from the supplied archive; the 2011 HCEX PDB has no `connected_geometry` compiland. Thus no matching Tool local/type/line records were available. The PE was inspected as data, never executed. The January split object has SHA-256 `9cb22e0eca7f318d9b3122f60a4f2f661539bf8b4bf8a23593955e9f42ede684`.

The exact duplicate-triangle warning string at PE VA `0x010EABC8` has a code xref at `0x00C099EE`, locating the separate-build triangle builder at `0x00C09770`. Its three vertex lookups call the helper at `0x00C09EA0`. That helper performs three ordered component comparisons, each obtaining the array element and then computing an `fabs` result; its debug instructions are consistent with direct `point.{x,y,z} - existing.{x,y,z}` expressions rather than the canonical named `pending_coordinate` temporary. This does **not** establish January's declaration shape.

The separate-build coplanarity predicate at `0x00C0A560` makes three distance calls at `0x00C0A6DD`, `0x00C0A713`, and `0x00C0A745`; then the plane-from-points call at `0x00C0A77F`; then the facing-dot call at `0x00C0A793`. Callee identities are inferred from argument/return behavior and the neighboring graph, not PDB names. This supports a natural high-level spelling using `plane3d_distance_to_point` three times, `plane3d_from_points`, and `dot_product3d`. It is a useful check against a fabricated scalar graph but not proof of January's inline schedule.

## January measurement of that helper spelling

In a source-only scratch trial, I replaced the three hand-expanded distance expressions with `fabs(plane3d_distance_to_point(plane, pointN)) < 0.01f`, and the hand-expanded facing dot with `dot_product3d(&triangle_plane.n, &plane->n) > 0.0f`. No new point/vector cast was introduced; the existing owner-header helper contains its own internal view. The `_point_from_line3d` emitted-symbol guard passed and all seven exact siblings stayed exact. `_triangle_coplanar` remained residual at 384/384 padded bytes, 151/151 decoded instructions, and 11/11 identical relocations. Its normalized-byte distance **worsened from 10 to 24**; the candidate hash is `9713d17b6bdb520f13ad004ef2619543c7df75551c4bcd12aa33a1e9f7d24186`. The compiler scheduled the first three dots `z,y,x` rather than January's `y,z,x`, and still scheduled the facing dot differently. The source was restored, without landing or rebaselining a park.

Reopen on a matching first-party PDB/source artifact, or a genuine same-compiler source construct with an independently evidenced reason to change the x87 IL schedule. The 2020 Tool build confirms semantics but does not justify a decoration-only rewrite or overriding the uninitialized-edge hold.
