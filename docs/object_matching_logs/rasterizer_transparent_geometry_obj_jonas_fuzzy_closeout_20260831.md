# `rasterizer_transparent_geometry.obj` fuzzy closeout

## Result

The last unwritten function is now reconstructed as coherent typed C. The unit
moves from 17 exact / 1 residual / 1 unwritten to 17 exact / 2 parked residuals /
0 unwritten functions. All 17 inherited exact functions remain strict exact.

`rasterizer_transparent_geometry_initialize` has January's 160-byte envelope,
51 instructions, and 16 logical relocation destinations. It remains at the
already-documented 76.00% block-placement fixed point: January places the
allocation-error block before the auxiliary-buffer call, while this VC7 source
lays out the call first. The retained source is corroborated by HCEA, Stian's
lift, and historical commit `b6adf6705`; no exact-only artifact is retained.

The existing 176-byte sort driver is parked at 80.08772%. HCEA supplies the
cross-build semantic identities `rasterizer_sort_internal`,
`group_sorted_indices_cmpfn`, and `rasterizer_sort_external`, replacing all
three address-derived private names in source and `config/symbols.json`. The
artificial packed aggregate formerly used only to pin BSS layout is removed.
The state is now expressed as eight honest, individually named globals with
HCEA-authenticated names where available and restrained semantic names for the
two January-only cursor values. The two helper bodies remain strict exact after
the rename; the external sorter retains its documented VC7 register-allocation
residual.

The target splitter represents this state as one 74-byte `.bss` section with
the eight names anchored at offsets 0, 4, 52, 56, 60, 64, 68, and 72. The
rebuilt object honestly emits eight external COMMON owners sized 2, 48, 4, 4,
4, 4, 4, and 2 bytes; the missing two payload bytes are the linker's natural
alignment gap before the 32-bit bucket vector. Consequently this checkpoint
claims no whole-section BSS equality. It deliberately accepts that ownership
representation residual instead of recreating a source-only packed aggregate.

Because this C file has no dedicated header, its initializer and Xbox auxiliary
buffer prototypes now live in the associated `rasterizer.h` owner header.

## Evidence and policy

- January split object remains the byte, relocation, and size authority.
- Later HCEA files corroborate allocation semantics, global identities, and the
  complete external-sort topology.
- The prior closure-freshness audit exhausted 1,571 registered worktrees and all
  known Stian, Pastudan, Claude, HCEA, and public-donor source families.
- No inline assembly, volatile/register forcing, pragma, barrier, raw address,
  representation pun, fake dependency, dead branch, or byte-forcing source is
  introduced.
- The initializer and sorter are parked under the current fuzzy-match house
  rule, with explicit reopen criteria in `config/parked.json`.

## Targeted validation

After regenerating the split object with the semantic symbol names,
`tools/campaign/gate.py` reports 17 exact, 2 residual, and 0 unwritten functions.
The renamed `rasterizer_sort_internal` (48 bytes) and
`group_sorted_indices_cmpfn` (384 bytes) remain strict exact. The orchestrator
owns the subsequent full configure, Ninja, semantic-report, parked-ledger, and
test-suite validation before canonical integration.
