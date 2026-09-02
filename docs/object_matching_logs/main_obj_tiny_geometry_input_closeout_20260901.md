# Main tiny geometry/input closeout (2026-09-01)

## Scope and provenance

This packet starts from canonical `484e63715345024485f1c0eb5e36feeaba71210d`
and restores four small Main-family owners from January machine evidence plus
the later HCEA source donors in the shared research cache:

- `halocea/src/blam/main/gamepad_button_is_down.c`
- `halocea/src/blam/main/screenshot_record.c`
- `halocea/src/blam/main/compute_subframe_counts.c`
- `halocea/src/blam/main/compute_window_bounds.c`

The HCEA routines establish names, types, and gameplay geometry. January's
disassembly, assertion strings, relocations, linkage, and call topology remain
authoritative wherever the later implementation differs.

## Exact recoveries

### `gamepad_button_is_down`

The retained ordinary C validates the button index, locates the first attached
gamepad, obtains a typed `gamepad_state const *`, and tests the named button
array. The semantic local pointer naturally makes VC7 choose January's
`[ecx+eax+0x10]` address form; no reversed index, carrier, or artificial alias
is present.

Strict COFF result: **128/128 padded bytes**, **6/6 relocations**, normalized
SHA-256
`13d2b18690b1d29b96ee00d8d68569df2ff0330687a678420b0257419bf08abf`.

### `screenshot_record`

The detached January body has no direct image xrefs and consumes its bitmap in
EAX and file reference in ECX, so a public cdecl reconstruction would be false.
The authentic caller experiment instead factors the existing TIFF/error block
in `main_present_frame` through a source-private `screenshot_record`. VC7
naturally inlines that call back into the caller and retains the private clone.
Both results match without pragmas or forced ABI controls:

- `screenshot_record`: **32/32 padded bytes**, **2/2 relocations**, normalized
  SHA-256
  `a46d5940ec112da49d813580fd4617755418774795d2a0f6047d21e85711634e`;
- `main_present_frame`: remains strict exact at **144/144 padded bytes** and
  **12/12 relocations**.

The target symbol is marked static, and no public prototype is emitted. This
preserves January's inline schedule rather than manufacturing a stack ABI.

## Credible fuzzy parks

### `compute_subframe_counts`

The helper is restored only together with its real caller. That context makes
VC7 emit it naturally with January's private convention: `num_players` arrives
in EBX and only the two result pointers use the stack. The target and candidate
both occupy **112 padded bytes** with **4/4 identical relocations**. Objdiff is
**94.347824%**. The only residual is January's `setl`/`test` loop decision
versus the compiler's direct `cmp`/`jge` for the same predicate. Ordinary
boolean-temporary, polarity, loop-form, and product-local variants did not
close it, so the direct HCEA topology is parked as `unclassified` without a
fake control.

### `compute_window_bounds`

The full HCEA geometry algorithm is restored with January's assertions at
source lines 1359, 1390, and 1391, semantic `rectangle2d` parameters, and named
rasterizer `screen_bounds`/`frame_bounds` fields. Target and candidate both
occupy **496 padded bytes**, carry **25/25 relocations at the same addresses**,
and decode to **176 instructions**. Objdiff is **99.87578%**. The only two
differences are commutative `imul` operand choices. Declaration-order,
expression-order, grouping, and product-local variants did not improve that
fixed point without collateral drift, so the natural routine is parked as an
instruction-scheduling tie.

## Prototype and definition-position audit

Adding the new public declarations to the broad `main.h` shifted VC7's type
allocation in `shell_xbox.c` and removed the expected `$T18267` owner for its
aligned-rdata semantic group; the regression remained when only the gamepad
prototype was left there. The declarations therefore live in the narrow
owner header `source/main/main_runtime.h`, included by `main.c`, following the
campaign's established remedy for C2 definition-position sensitivity. This is
not a local use-site prototype or a fake dependency.

Every ordinary `main.h` consumer was recompiled in the full build. Focused
gates remained unchanged for the supported C units: Main 73 exact / 2 residual
/ 20 unwritten; Game Engine 166/8/6; Players 52/17/1; Network Game Globals
26/0/0; Network Game Manager 13/0/6; Network Server Manager 24/0/46; Scenario
46/0/0; Shell Xbox 8/0/0; Units 189/0/0. The campaign gate does not accept the
space-containing `saved games/game_state` key or C++ `d3d_intimacy` source, but
both compile successfully in the full Ninja blast and the semantic/data-owner
audit passes.

## Admission checks

- Full `ninja`: pass, including report, semantic report, progress, aligned
  rdata, and non-code ownership validation.
- Strict verdict diff against the canonical snapshot: **2 gains, 0
  regressions** (`gamepad_button_is_down` and `screenshot_record`).
- Park manifest: **67 active, 0 stale, 0 invalid**.
- Focused fake-match scan across all changed C/header files: **0 findings**.
- `pytest tools`: **255 passed**.
- `git diff --check`: pass.
- Candidate Main object contains no `point_from_line3d` symbol/COMDAT.
- No volatile qualifier, inline assembly, pragma, fake dependency,
  address-derived private name, raw-offset field, or nonsensical branch was
  added by this packet.
