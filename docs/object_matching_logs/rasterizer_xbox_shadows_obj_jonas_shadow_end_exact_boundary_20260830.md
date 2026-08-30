# `rasterizer_xbox_shadows.obj` shadow-end exact packet / TU boundary (2026-08-30)

## Outcome

The single natural typed-C89 reconstruction of
`__rasterizer_environment_shadow_end` is strict-exact as an isolated function
packet, but is not retained.  The fail-closed whole-translation-unit gate
reported changed accepted evidence for all three inherited exact functions and
changed BSS, initialized-data, string-COMDAT, debug, and symbol ownership.
Production source and the rebuilt object were restored exactly; this commit is
ledger-only and grants no matching credit.

## Frozen input and provenance

- Integration base: `3a53f9ac5612d99470c3972776a22511e6cd7121`.
- January target object SHA-256:
  `b21253ca6dcc619abd165c3696ae838ceeb53a1c3d94cf639d08605328c29c71`.
- Untouched/final source blob:
  `3191e8735760f473856e5846c192931d63288115`.
- The existing lifecycle ledger was read in full.  It covers only
  `__rasterizer_environment_shadows_begin`,
  `__rasterizer_environment_shadow_model_end`, and
  `__rasterizer_environment_shadows_end`; neither repository history nor the
  registered local/research trees contained a prior body or measured shot for
  this symbol.
- The HCEA applied body and types independently authenticate the render-target,
  environment-shadow enable, shadow-used, and shadow-restored semantics.  The
  January COFF disassembly and resolved relocation destinations determine the
  Xbox-specific omission of HCEA's PC render-target-disable test, the warning,
  the five-argument Xbox `rasterizer_set_target` call, and the exact statement
  order.

## Sole code-producing candidate

The readable candidate asserted `global_d3d_device` at source line `0x233`,
required primary target zero plus the named environment-shadow debug flag,
warned at error priority 2 when the typed `shadow_used` flag was false, and,
when the initialized `shadow_restored` flag was false, restored the current
target with `(FALSE, FALSE, FALSE, TRUE)` before setting the flag true.
The global/window/debug prefixes and the shadow state were typed, with a
compile-time assertion proving `shadow_used` at `+0x49`.

One parse-only `/Zs` check initially rejected an aggregate-size assertion:
natural VC7 alignment makes the typed aggregate 76 bytes while January owns 74
bytes.  It emitted no object.  Removing only that invalid size claim was a
declaration-only correction; the frozen function body did not change.  Exactly
one normal `/O2 /Oy- /DDEBUG /Dxbox` code-producing compile followed.  There
was no body retry, source-shape variant, tuning sweep, compiler option change,
packing pragma, raw byte/offset access, or prohibited construct.

The sole emitted candidate measured:

- 126 meaningful / 128 padded bytes;
- 14 ordered relocations;
- normalized SHA-256
  `bb3c901ecb7a42e715117d680fd821d034652ca3690106ed6aab2d34b6459a1a`;
- raw candidate object SHA-256
  `1cd153d3f5ee9cf1db9a58d1e7c3b70b9f2976c891bbf95b92c096da3e93b1c4`.

The hardened comparator returned `all_equal: true`.  Relocation offsets and
destinations were exactly:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+1` | `DIR32` | `_global_d3d_device` |
| `+17` | `DIR32` | exact source-path COMDAT |
| `+22` | `DIR32` | exact `global_d3d_device` expression COMDAT |
| `+27` | `REL32` | `_display_assert` |
| `+34` | `REL32` | `_system_exit` |
| `+44` | `DIR32` | `_global_window_parameters + 0` |
| `+52` | `DIR32` | `_rasterizer_debug_options + 0x12` |
| `+61` | `DIR32` | `_bss_0046628c + 0x49` |
| `+70` | `DIR32` | `"### WARNING empty shadow has been cast"` COMDAT |
| `+77` | `REL32` | `_error` |
| `+85` | `DIR32` | `_data_0030cf84 + 0` |
| `+99` | `DIR32` | `_global_window_parameters + 0` |
| `+111` | `REL32` | `_rasterizer_set_target` |
| `+120` | `DIR32` | `_data_0030cf84 + 0` |

## Retention boundary

The frozen manifest correctly identified the function as the sole
`newly_exact` packet and reported no changed nonexact function.  Retention was
still rejected because the complete unit contract failed:

- accepted evidence changed for all three inherited exact lifecycle leaves;
- `.bss|owners=_bss_0046628c` changed because the natural typed aggregate is
  76 bytes while January owns 74 bytes;
- the newly authenticated one-byte true `.data` owner `_data_0030cf84`, three
  exact string COMDATs, four debug sections, and the symbol set changed.

Direct strict comparison separately proved that the actual code packets of all
three inherited lifecycle functions remained byte/relocation exact.  That does
not override the fail-closed full-owner gate.  Reopening this leaf therefore
requires a legal, typed, non-packing C representation that preserves the
74-byte BSS owner and the complete inherited TU fingerprint.  Repeating this
body without solving that ownership/alignment boundary is not useful.

## Restored-state verification

- The final source blob equals the integration-base blob exactly.
- Restored regression manifest: `ok: true`, three `still_exact`, no failures,
  no warnings, no newly exact functions, and no changed nonexact functions.
- Full Halo/libcmt builds, ordinary report, semantic report, and progress pass.
- Semantic audit: 470 units, 4,953 functions evaluated, 4,836 semantic exact,
  131 hidden exact / 78,940 bytes, 4,846 accepted exact, zero unit errors.
- Object admission: 0 candidates, 0 contradictions, 0 revocations.
- Parked validation: 12 active, 0 stale, 0 invalid.
- Tool suite: 212/212 tests pass.
- Protected Units sentinel `_unit_preprocess_node_orientations` remains strict
  exact at 1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

## Policy

The rejected candidate used ordinary readable typed C89, explicit `void` and
`return;`, and one parameter per line.  It introduced no assembly, `volatile`,
`register`, optimizer pragma, intrinsic, attribute, barrier, raw address or
offset access, pointer/integer reconstruction, representation pun, undefined
behavior, synthetic anchor, object patch, comparator exception, or byte forcing.

