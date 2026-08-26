# `render_objects.obj` RGB-clamp transfer disposition — 2026-08-26

## Outcome

This bounded shape-transfer wave did not add matching credit.  January
`_code_0017ae60` is byte-identical to the already exact
`source/scenario/scenario:_code_0017f370`: 160 padded bytes, zero
relocations, and normalized SHA-256
`be99b78c69d2f2b41d791415f13baa7dbf5d74f2d061695fc1d7cd0f810de4b6`.
The natural file-private source transplant was nevertheless eliminated by
VC7 because the recipient's caller is still absent from `render_objects.c`.
The first-shot object therefore contains no `_code_0017ae60` symbol.  The
candidate was rejected and the source was restored byte-for-byte to blob
`4ad6de9cbfaf22df9c03c6c32f8ad37e3459d95b`.

No external-linkage spelling, synthetic call, anchor, volatile use, barrier,
pragma, compiler flag, raw address, pointer/integer trick, or second compile
was attempted.  Recovering a real caller or obtaining independent source
linkage provenance is required before this helper may be reopened.

## Frozen authority and provenance

The wave starts from clean cumulative commit
`8a9724da5e1f14142227df55468ff1edee3dcbcb`.  Its complete evidence is under
`outputs/render_objects_0017ae60_evidence_20260826/`.

- January target object SHA-256:
  `21B42603C310403A91C4BCD986084F819AE46B042D9959B9C8F8CEDAFCAF3997`.
- frozen cumulative base object SHA-256:
  `992D9C6C2BE4256C68796E41BBA04BD68A86BA709ABCD553FC126BEC28F968B8`.
- baseline source SHA-256:
  `23B52CA1A6552857E87CBE65855A7DF73342DD186B394ECCC66816D4C0CA41E2`.
- pre-wave regression manifest SHA-256:
  `1F92C168BD2F20F1DE89B984D10D7C6302E07049566E3BB82E6C570E3F993D8E`.
- frozen first-shot source SHA-256:
  `A718B8FA3E444A41E8424AF1174EFD7FF1949BA8CA678C19D3BAD12379C63554`.
- unchanged first-shot object SHA-256:
  `ECA4F87DD153E51F92A2A9AF56FA9A24C2ADF473AB345AE11257E4A4793B8242`.

The current shape atlas proves the January donor and recipient have the same
160-byte, zero-relocation normalized body.  The Xbox HCEA reconstruction adds
semantic provenance: its function
`src/blam/rasterizer/interpolate_real_rgb_color.c` has the same three-channel
clamped-step contract, and
`src/blam/render/object_render_state_refresh.c` calls it for ambient, two
distant-light, and shadow colors.  The immediately following Xbox helpers are
the four-channel ARGB clamp and normal interpolation, matching the PC target's
neighboring helper sequence.  Those source snapshots are preserved under the
evidence directory's `provenance/` subdirectory.

This cross-build evidence proves semantics and parameter types, not January
source linkage.  The split target exposes `_code_0017ae60` with external COFF
storage class 2, but csplit also promotes the known file-private scenario
donor to storage class 2; its ordinary compiled donor symbol is storage class
3.  Consequently target-split storage cannot justify globalizing the
recipient.

## First and only production candidate

The frozen candidate copied the exact accepted scenario body and house style:

```c
static void code_0017ae60(
	real_rgb_color *current,
	real_rgb_color const *desired,
	real maximum_step)
{
	current->red += PIN(desired->red - current->red, -maximum_step, maximum_step);
	current->green += PIN(desired->green - current->green, -maximum_step, maximum_step);
	current->blue += PIN(desired->blue - current->blue, -maximum_step, maximum_step);

	return;
}
```

The pinned XDK 3911 compiler passed a no-output `/Zs` syntax check.  Exactly
one ordinary Ninja edge then compiled `render_objects.obj`.  VC7 removed the
unreferenced file-private function, so strict comparison failed closed at
symbol lookup rather than producing a near match.  This establishes the
missing-caller boundary directly.

The four inherited exact functions remained strict with all padded bytes and
ordered relocations unchanged:

- `_render_objects_initialize` — 64 bytes, 7 relocations;
- `_render_objects_initialize_for_new_map` — 16 bytes, 2 relocations;
- `_render_objects_dispose_from_old_map` — 32 bytes, 2 relocations; and
- `_render_objects_dispose` — 16 bytes, 1 relocation.

The no-build whole-TU regression check reports those four as `still_exact`,
with no newly exact functions, changed nonexact functions, failures, or
warnings.  Configuration, semantic ledgers, parked metadata, data ownership,
and completion status remain unchanged.

## Reopen condition

Do not repeat the standalone static transplant.  Reopen only when at least
one of the following is available:

1. an evidence-backed recovery of a genuine `render_objects.c` caller (the
   strongest current candidate is the 0x480-byte state-refresh function at
   `_code_0017a9e0`); or
2. independent January-era source/PDB/map evidence that proves the helper had
   external linkage.
