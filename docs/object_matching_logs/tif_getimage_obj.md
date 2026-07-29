# `tif_getimage.obj` exact-match ledger

## Scope and invariant

- Translation unit: `source/bitmaps/libtiff/tif_getimage.c`
- Target: January 2002 `tif_getimage.obj`
- Starting commit: `64f068c27cc56bcc55d58ceb5382863087082f3f`
- Toolchain: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Acceptance: every function and every target-owned data section must pass the
  hardened COFF comparator. No inline assembly, volatile codegen coercion,
  undefined behavior, binary patches, or compiler-flag changes.

The starting state was 24/27 strict-exact functions. All 745 target-owned data
bytes were already exact. The three residual functions were `makecmap`
(`code_00058c40`), `initYCbCrConversion` (`code_00059a60`), and
`putRGBContigYCbCrClump` (`code_00059ab0`).

## `initYCbCrConversion`

Target: 80 bytes, 10 relocations.

| Experiment | Result |
| --- | --- |
| Initialize `reciprocal = 1.0f/LumaGreen` before `D1` | 80 bytes, 9 relocations; rejected |
| Compute `D1`, then reciprocal, with direct three-factor products | 96 bytes, 10 relocations; rejected |
| Compute `D1`, then reciprocal, and stage the reused multiply through `product` | **STRICT EXACT** |

Accepted source shape keeps `1/LumaGreen` live on the x87 stack and reuses it
for both coefficient paths:

```c
D1 = 2 - 2*LumaRed;
one_over_luma_green = 1.0f / LumaGreen;
product = one_over_luma_green*LumaRed;
D2 = product*D1;
D3 = 2 - 2*LumaBlue;
product = one_over_luma_green*LumaBlue;
D4 = product*D2;
```

Final strict evidence:

- size: 80/80
- relocations: 10/10
- normalized SHA-256:
  `4c8558ad61a2a1d2cf69dc295bd14364eb926b8349e85e66ba7bc09faca8d50a`

## `putRGBContigYCbCrClump`

Target: 464 bytes, 27 relocations.

| Experiment | Result |
| --- | --- |
| Explicit invariants before an ordinary `for` loop | 464/27; zero-trip scheduling wrong |
| Guarded `if (cw > 0)` around invariant setup and `for` | 480/27; redundant precheck |
| Direct authentic expressions in ordinary `for` | 464/27; frame 0x1c vs target 0x20, broad drift |
| Guard + invariant setup + `k=0; do/while` | 464/27; `k` initialization scheduled after test |
| Move `k=0` before guard | best: 464/27, all relocations exact, four x87 operand bytes differ |
| Reverse commutative spelling (`Y + D1Cr`, etc.) | no change; rejected as a non-control |
| Split `R`/`B` accumulation into assignment plus `+=` | 480/27; rejected |
| Change invariant temporaries from `double` to `float` | no change; rejected as a non-control |

Current best residual is confined to two x87 additions:

- target R: `fld st(3); fadd st(1)`
- candidate R: `fld st(0); fadd st(4)`
- target B: `fld st(2); fadd st(1)`
- candidate B: `fld st(0); fadd st(3)`

Everything else, including all relocation addresses, the zero-trip guard, and
the three consecutive x87 pops, is exact. Do not repeat the commutative spelling,
split-accumulation, or float/double local-type experiments.

## `makecmap`

Target: 832 bytes, 18 relocations.

| Experiment | Result |
| --- | --- |
| Baseline local `palmap`, global `PALmap[i]` loop store | 832/18; non-exact register schedule |
| Exact-sibling-style local `palmap[i]` store | 832/17; loses required global relocation; rejected |
| Global-only/no local `palmap` | 816/18; rejected |

The target retains the allocation result in EDX for the first iteration, keeps
the output pointer in ECX and the loop index in EAX, then reloads global
`PALmap` on later backedges. The baseline instead uses EAX for the output pointer,
ECX for the loop index, and reloads the global before the first store.

High-evidence untried controls are:

1. declaration order matching exact sibling `makebwmap`: `i`, `nsamples`, `p`,
   then `palmap`, while retaining `PALmap[i] = p`;
2. explicitly peeled first-iteration topology using local `palmap[i] = p`,
   then `palmap = PALmap` on the loop backedge.

## Provenance breakthrough and final closeout

The remaining source topology was recovered from the authentic libtiff RCS
revision 1.8 source. The original translation unit used separate file-scope
statics, not the reconstructed aggregate that had coupled alias analysis and
register allocation across the three residual functions. Restoring the separate
objects made the authentic `makecmap` global accesses and the direct nested
`putRGBContigYCbCrClump` loop strict-exact.

Uninitialized file-scope statics were allocated in an unstable order by this
compiler. Explicit zero initializers made their source order deterministic and
recovered the January `.bss` layout. The placeholder `_bss_0031be54` symbol was
then replaced in `config/symbols.json` by the recovered local symbols `_D4`,
`_D3`, `_D2`, `_D1`, `_PALmap`, `_BWmap`, `_refBlackWhite`, `_YCbCrCoeffs`,
`_YCbCrVertSampling`, `_YCbCrHorizSampling`, `_filename`, `_stoponerr`,
`_bluecmap`, `_greencmap`, `_redcmap`, `_orientation`, `_photometric`,
`_samplesperpixel`, and `_bitspersample`, at the exact January offsets.
Each recovered entry is marked static in the csplit manifest; `llvm-nm` reports
matching lowercase `b` ownership in both target and candidate.

This file retains its authentic 1992 libtiff K&R declaration style rather than
mixing modern prototypes into a legacy third-party translation unit. The two
modified void functions nevertheless end in explicit `return;` statements per
the project's house rule; VC7 emits no additional instructions for them.

Rejected closeout experiments:

- one externally linked BSS anchor plus a `D4` macro: exact `makecmap` and
  `initYCbCrConversion`, but changed clump scheduling through the linkage/alias
  model;
- raw uninitialized separate statics: correct code, nondeterministic/wrong BSS
  allocation order;
- preserving the aggregate while copying authentic expressions: retained the
  aggregate-induced scheduling mismatches.

Final strict evidence for the former residuals:

- `makecmap`: 832/832 bytes, 18/18 relocations, normalized SHA-256
  `bf6832724e2051eaa90147ed6b5db734189bea61696903dfbc39cb03e37ff7af`;
- `initYCbCrConversion`: 80/80 bytes, 10/10 relocations, normalized SHA-256
  `4c8558ad61a2a1d2cf69dc295bd14364eb926b8349e85e66ba7bc09faca8d50a`;
- `putRGBContigYCbCrClump`: 464/464 bytes, 27/27 relocations, normalized
  SHA-256 `61f4a8ab80dc48d34197c99801ed6cfcdf0e55acbfe86d21c7d0ebdd6df55828`.

## Final admission state

- Hardened function sweep: **27/27 strict exact**
- Target-owned data: **745/745 bytes strict exact**
- `.bss`: **74/74 bytes**
- `.data`: **91/91 bytes**
- `.rdata`: **580/580 bytes**
- Consolidated `ninja progress` build: successful
- Progress admission: complete unit retained; no premature-completion revocation
- Object status: **Matching**
