# `rasterizer_xbox_decals.obj`: typed disposer (2026-08-29)

## Result

This bounded lifecycle batch starts from canonical commit
`1c7eda8c3f4576b4a0768dd8e84d0cddcdbcb908`. The ordinary typed-C
implementation of `_rasterizer_decals_dispose` matched on the first compiled
candidate. The object advances from 5/29 to 6/29 strict functions, from
79/4,333 to 240/4,333 meaningful code bytes, and from 128/4,592 to 304/4,592
padded code bytes. It remains `NonMatching`; no completion label or data credit
is claimed.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_decals_dispose` | 161 | 176 | 20 | `1326cad86d7849e2ef42d4f0e08fd073805be1d8dd57a42cd87c0a514efacdf6` |

The hardened comparator reports `equal: true`. It verifies all padded bytes
and all 20 relocation addresses, types, targets, and addends. The relocations
include the typed `_bss_0045e8e8` fields at +16 and +20, the external
`_global_d3d_device`, the natural assertion literals and calls, the stdcall
`_D3DResource_Release@4`, and `_lruv_delete`.

## Source and provenance

The target is the January `cachebeta.exe` build 2342 split object. The target
object hash is
`bcbbf5e61571861c22ed4503fae05f8760652ba2d53f87e7e5d112c4db23c892`.
Compilation uses the pinned XDK 3911 compiler (`13.00.9254.1`) and the unit's
unchanged repository flags.

The retained source expands the existing external decals-global prefix with a
typed opaque `local_d3d_vertex_buffer` pointer at +16 while preserving the
typed LRUV cache pointer at +20. Offset assertions prove the layout at compile
time. The disposer then follows January's authenticated topology directly:

1. assert the cache, vertex buffer, and D3D device globals;
2. release and clear the vertex buffer when present;
3. delete the LRUV cache;
4. return explicitly.

No writable storage is defined by this wave. The prefix only gives types to
the pre-existing external `_bss_0045e8e8` owner, and every access goes through
a named field rather than a raw address or pointer offset.

## Rejected sibling

The same first batch included an ordinary typed implementation of
`_rasterizer_decals_initialize`. It did not pass the strict gate and was
removed immediately:

| Candidate | Target | Candidate | Relocations T/B | Target hash | Decision |
| --- | ---: | ---: | ---: | --- | --- |
| `__rasterizer_decals_initialize` | 272 padded | 288 padded | 31/33 | `87bdf054c7cc834eb2ed4baa6be508efc6476dca660b1c3d50a89e8cb5940b1a` | reverted |

The initializer's source, declarations used only by it, and its two excess
relocations were not retained. Its candidate hash was not preserved, so none
is invented here. Reopen it only from a measured first-divergence and
relocation comparison against this target snapshot.

## House-rule audit

The retained function uses readable typed C, puts `void` on its own argument
line, and has an explicit terminal `return;`. It contains no assembly,
intrinsic, barrier, pragma, `volatile`, `register`, attribute, raw address,
pointer/integer reconstruction, undefined aliasing, union punning, synthetic
anchor, compiler-flag override, object patch, comparator exception, semantic
allowlist, or parked entry. No protected source or pre-existing ledger was
edited.

## Validation

- `tools.campaign.gate source/rasterizer/xbox/rasterizer_xbox_decals --all`:
  6 exact, 0 residual, 23 unwritten.
- Direct `tools.coff_compare`: `all_equal: true` for the new disposer.
- The five inherited exact functions remain exact; the initializer is absent.
- Full Halo and libcmt builds: pass.
- Progress: 575,308/2,198,102 meaningful code bytes and 4,587/11,060
  functions overall; Halo is 562,394/1,770,166 bytes and 4,420/7,574
  functions. This wave contributes exactly one function and 161 meaningful
  bytes.
- Semantic audit: 470 units, 4,736 functions evaluated, 4,614 semantic exact,
  4,624 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, deleted-path scan, protected-source boundary, and
  retained-path review: pass.
