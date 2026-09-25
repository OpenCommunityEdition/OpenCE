# Scout research handoff

**No exact gain or source promotion. Vehicles remains 38/39.** New controlled
evidence separates the 12-byte frame defect into an eight-byte input-copy
effect and a four-byte computed-left-vector representation/packing effect.
See [LEDGER.md](LEDGER.md) for the full causal matrices and protected holds.

The two useful handoff patches are:

- `AB_genuine_topology_direct_views.patch`: scalar horizontal copies; 29 REAL
  regions, 0x98 frame. No load-bearing aggregate view copies.
- `held_aggregate_control.patch`: both primary-attested aggregate view copies;
  1 REAL region, exact 0x94 frame. **Policy-held**, no count-shifting leveling
  helper. This is a compiler reproducer, not an admission candidate.

Both preserve all 38 exact controls. `measurements.json` records source/object
hashes, normalized scout hashes, complete exact-row lists and noncode/ownership
checks; each `.gate.txt` is actual fresh output. A/B/AB and the orthogonal
left-copy/forward-copy controls are preserved as canonical-relative patches.

## Exact codegen mapping

At source `vehicle_left_horizontal = *(real_vector2d const *)&vehicle_left`,
aggregate copying emits `fstp [left]` at +0x34b, integer load at +0x34e, integer
store at +0x35c; the second component follows at +0x364/+0x369/+0x36f.
The field-by-field control instead emits `fst [left]` at +0x349 and
`fst [left+4]` at +0x35e, keeps both values on x87, then `fxch` at +0x370 and
`fstp` into the two horizontal components at +0x372/+0x375.

Only the local-left aggregate copy reduces frame 0x98 -> 0x94. The forward
aggregate copy changes scheduling but not frame size. `/FAsc` proves the
three scope-disjoint 12-byte locals then share -76, instead of occupying two
12-byte groups with the eight-byte applied-alpha vector partly overlapping
one group. This is a measured storage/materialization distinction; the exact
internal C1/C2 allocator flag remains a hypothesis.

The final held residual is source `dot_product3d(&vehicle->object.
translational_velocity, &vehicle->object.forward)` at candidate lines
2271-2272: January +0x628 loads `[ebx]` then multiplies `[esi+0x18]`, while ours
loads `[esi+0x18]` then multiplies `[ebx]`. All later offsets reconverge.

## Reproduce

From canonical repository root at `20d0563d`:

```powershell
python research/astra_breakthroughs_20260925/scout/probe.py
python research/astra_breakthroughs_20260925/scout/audit_new_helper.py
python research/astra_breakthroughs_20260925/scout/run_ghidra.py
```

The first command uses unchanged canonical source plus the preserved historical
reviewer donor at the path recorded in `probe.py`; it writes only generated
scratch copies and research receipts. Full source/object/listing/diff artifacts
are in `scratch/astra_scout_20260925/`.

## Primary tools

The operational Ghidra path is `run_ghidra.py` + `ScoutLaunch.java` +
`ScoutPrimary.java`. It uses installed Ghidra 12.1.2 and JDK21, opens the primary
program through `getReadOnlyDomainObject`, bounds disassembly to
`0x008f80b0..0x008f930a`, verifies the function body stays inside that bound,
and rolls back the temporary function transaction in `finally`. No project
save occurs. Execution is capped at 60 seconds (compilation at 30).

Input `halo_cache_symbols.exe` SHA-256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

Actual exports, under `scratch/astra_scout_20260925/`:

- `ghidra_scout.asm`: SHA-256
  `56d8cd21549169ca292f0da384ea53709999652f0a4edcb33ca1d3a71e158b4a`.
- `ghidra_scout.c`: SHA-256
  `dcc4f3a50ee882eefe4c398a1aec904b25ddaf981665ee672bbf90983b1b8ec5`.

Ghidra reports the bounded function body `008f80b0..008f9309`. Its fresh export
corroborates the local-left integer copy at `008f8733..008f8745` and the two
component leveling expressions at `008f8b4a..008f8bba`. Decompiler names are
analysis names, not recovered symbols.

IDA Free 9.4 was attempted with the preserved `ida_probe.idc`. It could not
start under the current sandbox: `Fatal registry error: Access is denied.`
The owned hidden process was stopped after 20 seconds. No IDA result is claimed.

## Holds and next falsifiable step

The load-bearing aggregate view copies remain held. The third leveling
`point_from_line2d` is excluded because it is unsupported at its own site and
only moves a distant temporary-count tie. The newly emitted genuine two-call
`point_from_line2d` copy is byte-identical and link-coalesces, but its scout
caller is not strictly exact, so that alone is not admission authority.

Next use the compiler debugger on the fixed scalar-left and aggregate-left
sources: test whether `vehicle_left` enters stack packing with different
address-observed/aggregate storage flags. Same flags but different packing
would falsify the current mechanism and implicate interference construction.
Do not resume declaration-order or +1-temp sweeps for the final leaf; require
new first-party source or header-context evidence that predicts it.
