# `ui_widget.obj`: bitmap reconstruction wave (2026-09-02)

## Result

- Base: canonical commit `5cd391421a40d29ced14122cd55e9ff5965d8546`.
- Baseline gate: **53 exact / 3 residual / 46 unwritten** of 102 functions.
- Final gate: **54 exact / 4 residual / 44 unwritten**.
- Exact padded-byte gain: **32 bytes** (`3,872` to `3,904`).
- Unwritten padded bytes: **19,200 to 18,000**. The 1,168-byte bitmap body is now an honest residual rather than a placeholder.

## Exact closure

`_compute_offset_coordinate` is exact at 32 padded bytes. Target and candidate
both have three relocations, normalized SHA-256
`631ea40d7e408ba3b72ca9671cf2110ee39784c5209c6b6ee43379c11499da02`,
and the same `delta * 0.001`, integer-time multiply, and `fmod(..., 1.0)`
operation order.

The helper remains `static __inline` because the January object both emits its
private 32-byte section and expands the same computation naturally inside the
bitmap routine. No forced-inline directive or call-site workaround was added.

## Reconstructed fuzzy body

`_draw_bitmap_in_rect` now contains the full source-plausible UI bitmap path:

- default source rectangle and destination clipping;
- normalized texture extents and four dynamic-screen vertices;
- ordinary single-map rendering;
- typed interface-bitmap access for the two plasma maps;
- the four target-authenticated wrapped-map rates and directions;
- map scales, tint/fade state, blend state, and the final rasterizer draw.

The January object, target disassembly, relocation identities, and existing
bitmap/interface/rasterizer owner types were authoritative. The open HCEA UI
implementation was used only to corroborate high-level semantics; its later
layout and control-flow differences were not copied.

The current fixed point is **1,120 bytes versus the 1,168-byte target**, with
the exact `0x13c` stack frame and **36/36 relocations in the same symbolic
order**. All target constants and callees agree. The remaining normalized
instruction SHA differs because C2 chooses a shorter local/value schedule,
including different register allocation around the initialized parameter
block. Further cosmetic expression permutations did not change that boundary.

This function is documented as a fuzzy frontier, but is not added to
`config/parked.json`: its unequal code size does not meet the campaign's strict
formal-park preconditions. It should be revisited only with stronger source or
compiler-shape evidence, not anchors, dummy operations, or nonsensical logic.

## House-rule audit

- All new functions and parameters follow the multiline declaration style and
  every function has an explicit return.
- Engine scalar/color/geometry types are used instead of raw substitutes.
- Typed bitmap access goes through `bitmap_group_get`; the body contains no raw
  `tag_get` or object-access cast.
- New dependencies come from their owning bitmap, interface, cseries-Windows,
  and rasterizer headers. No shared header or cross-module prototype changed.
- No address-named private function or BSS global was introduced.
- No volatile/register/pragma steering, fake branch/call, dummy arithmetic,
  inline assembly, or raw byte emission was introduced.
- The January math inline schedule is unchanged and no
  `point_from_line3d` reference or COMDAT was introduced.

## Verification

- `gate.py source/interface/ui_widget --all`: **54 / 4 / 44**.
- `_compute_offset_coordinate`: exact size, relocations, identities, and SHA.
- `_draw_bitmap_in_rect`: exact symbolic relocation sequence and stack frame;
  honest size/SHA residual recorded above.
- `fake_match_scan.py source/interface/ui_widget.c --fail-on-findings`: zero
  findings.
- Repository tool suite with a lane-local pytest temp root: **261 passed**.
- Protected `units.obj` gate: **189 exact / 0 residual / 0 unwritten**.
- `git diff --check`: clean apart from Git's local LF/CRLF advisory.
