# `tiff_file.obj` -- exhaustive fixed-point closeout

Date: 2026-08-30

Canonical base: `58387f05c8a6a168fafae289bce9f9b67257825a`

Owner: `source/bitmaps/tiff_file.obj`

## Outcome

The requested two-residual closeout found no newer admissible C source to
transfer. The checked-in `source/bitmaps/tiff_file.c` is already the union of
all machine-resident progress and remains the unique best readable-C artifact.
The two residuals are therefore closed at their documented compiler-allocation
fixed points, not credited as exact:

| Function | January | Current XDK 3911 candidate | Strict result |
| --- | ---: | ---: | --- |
| `_tiff_get_bounds` | 112 padded / 6 relocs / `8490c88a00c5d6cb26bf2991c5977406e00a739173a246d0ecd5ae1f9dad604a` | identical | exact, preserved |
| `_tiff_export` | 1,056 padded / 37 relocs / `bb3cfd23e19a770c7007ddea8af93a4f868d72eed00f65f24706df1a986b14aa` | 1,056 / 37 / `ab6499a38e42782360f9b6db387e823e9093167184f9137fe9759ee6d8c60a28` | nonexact fixed point |
| `_tiff_import` | 1,344 padded / 51 relocs / `58ebbc3be5af4535adc2efa460f05275fb970e8c1f9f30ff292f640765a08091` | 1,344 / 51 / `6f9acb1e23ddee52f80030f9220f12f6660728d6199abb00b12736ef2fb03cda` | nonexact fixed point |

The 512-byte `_bss_0031c320` owner also remains exact, with normalized SHA-256
`076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560`.
The unit remains honestly `NonMatching` at one of three
strict functions. Equal size and relocation count for the other two functions
is envelope evidence only and grants no matching credit.

## Evidence census

Before the production compile, the audit read the repository `AGENTS.md` and
`CLAUDE.md`, the exact-match acceleration playbook, both TIFF object ledgers,
all TIFF-specific worktree notes, the complete Git history of
`source/bitmaps/tiff_file.c`, and the available public/reference material.
That material included:

- the authenticated Pastudan decompilation and January instruction listing;
- the punpckhdq symbol/inventory skeleton;
- commits `afdf79e71`, `58fb4e05a`, `7a2d1227b`, and `df9314369`;
- the `tiff-file`, `tiff-file-closeout-atomic`,
  `tiff-import-bitmap-result-exp`, `tiff-ownership-closeout-2`, and current
  campaign worktrees;
- the exhaustive allocation/lifetime donor search, rejected lifetime
  experiments, and machine-wide TIFF salvage census.

The live source census found 883 worktree copies collapsing to eight content
hashes. The prior object census examined 660 physical candidate objects, of
which 223 had the complete modern symbol set and collapsed to six machine-code
signatures. No source or object beat the current frontier. HCEA/Stian material
contains no independent TIFF body; punpckhdq contains only the symbol skeleton.
The only apparently newer dirty source is the documented output-holder
experiment, which regresses import from 745 to 834 differing normalized bytes
and is explicitly rejected.

The retained production source hashes to
`abd234365301c7ffcf776c88ec64ae6b9ae7477661487cbc17eb79bc4880a7ce`
and its Git blob remains `8d9e1ad1e8179b59d7d2089376d31efd9c16dc05`.
No production source, header, config, label, parked record, semantic exception,
compiler flag, tool, or target object was changed in this closeout.

## Fixed boundaries

### `_tiff_export`

The retained natural C already reproduces the complete 1,056-byte/37-reloc
envelope. The final measured boundary has 876 differing relocation-normalized
bytes, 18 of 37 ordered semantic relocation identities in sequence, and 11 of
37 relocation addresses aligned. January keeps the bitmap in EDI, TIFF/row
buffer in ESI, and TIFF handle in EBX with the two short tag values stack-homed;
VC7 instead keeps photometric in EDI, the bitmap in EBX, and spills TIFF,
producing a four-byte larger live frame before compensating elsewhere.

All authenticated natural source families have already been measured:
early-return versus nested failure topology, declaration and case order,
short/long tag and loop types, initialization spelling, row-size division,
channel expressions, scalar/aggregate holders, copied values, and legal
live-range changes. Each is inert or regresses the strict packet. There is no
new donor-backed lever in the current tree census, so this closeout does not
repeat a speculative variant.

### `_tiff_import`

The retained natural C reproduces the complete 1,344-byte/51-reloc envelope,
all 474 instructions structurally, and the first 145 instructions byte for
byte through the allocation null tests. It has 745 differing normalized bytes,
42 of 51 ordered semantic relocation identities, and 25 of 51 relocation
addresses aligned.

The first divergence is the allocation-stage register decision: January puts
the scanline in EBX, reuses ESI for the short `y` loop counter, keeps the bitmap
memory-homed, and uses EDI for the four-channel destination. The current VC7
allocation puts the scanline in EDI, retains the bitmap in ESI, uses EBX for
`y`, and spills the four-channel destination. A strict exact-donor scan found
no function with the complete required ESI-home/EBX-allocation/narrow-reuse
motif. The output-holder rewrite introduced alias uncertainty and regressed
the packet; standalone NULL assignments were byte-neutral. Reopening requires
new source/local-variable evidence or a genuine live-range donor, not another
spelling sweep.

## Reproof and gates

The frozen production edge was compiled once with XDK 3911 CL 13.00.9254.1
under `/O2 /Oy- /DDEBUG /Dxbox`. The resulting candidate object has raw SHA-256
`0ea9689ed545503d0e233e584b9ae6a2b67e6dc65645809b483c3b970a3e9672`;
the extracted January object has raw SHA-256
`af7ebe9529bf233229c81c99690ca3ddd381dfaaa1225a03930f578d1f3394a6`.
The hardened comparator reconfirmed all three packets above. A focused
whole-TU regression snapshot/check reports `_tiff_get_bounds` still exact,
`newly_exact: []`, `changed_nonexact: []`, and no ownership warnings.

The restored canonical state passes:

- full `halobetacache_build` and `libcmt_build`;
- ordinary report, semantic report, and strict progress;
- semantic audit: 470 units, 4,954 functions evaluated, 4,837 semantic exact,
  4,847 accepted exact, 131 hidden exact, and zero unit errors;
- object admission: zero candidates, contradictions, or revocations;
- parked validation: 12 active, zero stale, zero invalid;
- all 212 tooling tests;
- focused TIFF regression and complete hardened COFF comparison.

`build/report.json` hashes to
`fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`;
`build/semantic_report.json` hashes to
`3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.

Protected `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Only this additive fixed-point ledger is retained, and nothing is pushed.
