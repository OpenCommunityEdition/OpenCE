# Scout: representation materialization controls, 2026-09-25

Research only at canonical `20d0563d508f7836670db62514ffc3d1f1e2217e`.
**Zero exact gains; no production source/header/config edits.** Every measured
copy retains all 38 exact vehicles functions. Scout remains the sole residual.
No count instrument, dead local, helper expansion, pragma, volatile, compiler
optimization change or undefined-input experiment was added.

## Outcome worth carrying forward

The remaining four bytes of the frame defect have a narrowly measured cause:
the **computed local left vector's representation copy**, not the corresponding
forward-vector copy, determines whether VC7 packs the three scope-disjoint
12-byte vectors into one stack slot. The representation copy also ends x87
forwarding: it emits `fstp` to the 3D local followed by integer `mov` into the
2D local. Scalar component assignments instead retain the values on x87 with
`fst`, then later `fstp` into the 2D local. This is an actual emitted difference,
not an inferred declaration-order effect.

The historical near-exact body remains near-exact under today's repaired
headers/providers: **one two-instruction leaf-order swap at scout +0x628**.
Thus current header/provider repair alone does not solve the residual.

## Inputs read and excluded work

- `docs/campaign_house_rules.md`, `docs/matching_methodology.md`.
- `astra_90pct_research7_vehicles_20260920.md` and Lane B vehicles report.
- Structural vehicles `REPORT.md`, including M1a/M1b/M7/M10/M15.
- Round-1 vehicles worker and independent review ledgers, especially R7/R11.
- Round-2 vehicles admission manifest and current R3 report.
- Later primary `/Od` body at `0x008f80b0..0x008f930a`.

The old leveling `point_from_line2d` was **not** carried forward. Its own-site
machine bytes do not discriminate it from the attested component statements;
its benefit is solely a distant temporary-count change. Nor were the exhausted
declaration-order, operand-order, fake-local or square-root count probes rerun.

Donor input is the independent reviewer's `rv_od_leveling.c`, not the rejected
all-exact worker candidate. `probe.py` copies only its scout body into today's
canonical TU and asserts exactly two `point_from_line2d` calls and the component
leveling spelling. Input hashes are recorded in `measurements.json`.

## First 2x2: direct input views versus genuine helper/lifetime topology

Factor A removes the canonical temporary 2D `up` and `angular_velocity` input
copies and passes the original field views to the same four `dot_product2d`
sites. The primary build passes those field addresses directly.

Factor B carries the `/Od` helper/aggregate/lifetime transcription but retains
the input copies, and deliberately uses scalar component copies for the two
horizontal vectors. It excludes both load-bearing aggregate view casts.
AB combines these factors. This is a grouped structural control, not a claim
that B identifies a single source statement.
The prior January-motivated forward-copy-before-cross ordering is retained;
the later `/Od` build performs the cross first. Thus the experiment's
`genuine_topology` label does not claim verbatim recovery of all original C.

| Cell | Padded bytes | Frame | Instructions | REAL regions | Exact rows |
|---|---:|---:|---:|---:|---:|
| Baseline | 2464 | 0xa0 | 777 | 101 | 38/39 |
| A: direct views only | 2448 | 0x98 | 769 | 119 | 38/39 |
| B: genuine topology, input copies | 2464 | 0xa0 | 777 | 46 | 38/39 |
| AB: genuine topology, direct views | 2464 | 0x98 | 774 | 29 | 38/39 |
| January | 2464 | 0x94 | 775 | 0 | 39/39 |

A removes eight frame bytes in both contexts. B improves operation structure
but does not independently repair frame size. The four-byte residual remains
after AB. A's lower byte/instruction count is not a gain; it is further from
January in operation structure.

## Second 2x2: isolate the representation-copy site

Starting from AB, C restores only the local-left aggregate view copy, while D
restores only the object-forward aggregate view copy. Both are policy-held
diagnostic controls. Neither is proposed for promotion.

| Left copy | Forward copy | Frame | Instructions | REAL regions |
|---|---|---:|---:|---:|
| Scalar | Scalar | 0x98 | 774 | 29 |
| Aggregate (C) | Scalar | 0x94 | 775 | 10 |
| Scalar | Aggregate (D) | 0x98 | 774 | 22 |
| Aggregate (C+D) | Aggregate | 0x94 | 775 | 1 |

These differ only at the indicated representation-copy statements. No local
reordering or helper change is hidden in the second matrix.

VC7 `/FAsc` slot maps make the four bytes concrete:

| Local | Scalar left copy | Aggregate left copy |
|---|---:|---:|
| `vehicle_matrix` | -152 | -148 |
| `velocity` | -100 | -96 |
| hover<1 `vehicle_left` | -88 | -76 |
| hover>0 `desired_velocity` | -88 | -76 |
| flag-3 `vehicle_horizontal_forward` | -76 | -76 |
| `applied_alpha` | -72 | -84 |
| `magic_torque` | -64 | -64 |

Scalar projection uses two 12-byte slots, with `applied_alpha` sharing eight
bytes of the later horizontal-forward slot (24 bytes total). Aggregate left
projection lets all three scope-disjoint 12-byte vectors share one slot, plus
an independent eight-byte `applied_alpha` slot (20 bytes total).

Machine witness, same source site around candidate line 2204:

```text
held_aggregate_control:
  +034b fstp [vehicle_left]
  +034e mov edx,[vehicle_left]
  +035c mov [vehicle_left_horizontal],edx
  +0364 fstp [vehicle_left+4]
  +0369 mov eax,[vehicle_left+4]
  +036f mov [vehicle_left_horizontal+4],eax

held_D_forward_aggregate_only (scalar left copy):
  +0349 fst [vehicle_left]
  +035e fst [vehicle_left+4]
  +0370 fxch st(1)
  +0372 fstp [vehicle_left_horizontal]
  +0375 fstp [vehicle_left_horizontal+4]
```

**Inference, not yet an internal-compiler proof:** the aggregate read makes
the local an address-observed representation object rather than only a set of
forwardable floating-point components, changing its allocator class/packing.
The emitted materialization and slot-group changes are proved; the precise
C1/C2 flag responsible has not been traced.

## Best remaining unexplained signature

`held_aggregate_control.c:2271`:

```c
real speed = dot_product3d(&vehicle->object.translational_velocity,
    &vehicle->object.forward)/definition->unknown2f8;
```

Current object and January have 775 instructions, 2464 padded bytes and 91
relocations. The only REAL region is the i-component multiplication:

```text
January +0628 fld  dword ptr [ebx]          ; forward.i
        +062a fmul dword ptr [esi+0x18]    ; translational_velocity.i
ours    +0628 fld  dword ptr [esi+0x18]
        +062b fmul dword ptr [ebx]
```

All later offsets reconverge after the five-byte pair. This is not a remaining
frame issue. Current authentic helper/provider changes do not move it.
The prior count instruments are evidence that this leaf is sensitive to IL
ordinal context, not authority to add a count-shifting source construct.

## Fresh primary-tool evidence

Ghidra 12.1.2 actually exported and decompiled the primary function during this
run. `run_ghidra.py` compiles the standard `GhidraLaunchable` entry point to
avoid a local OSGi loader failure. It opens a **read-only** program, defines the
aligned entry only in a temporary transaction, rolls that transaction back,
and never saves the project. The export's executable SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

- `0x008f8733..0x008f8745`: first two words of the local 3D left vector are
  copied by integer MOVs to the horizontal 2D local, not by float arithmetic.
- `0x008f8b4a..0x008f8bba`: leveling is two component expressions; no third
  `point_from_line2d` call is present.
- `0x008f8cda..0x008f8ce8`: the dot inputs are original forward/velocity
  addresses, in the order transcribed above.

`scout_temporary_analysis` and Ghidra local names are analysis labels, not
recovered source names. This later build does not establish January source
identity. Full export/decompilation and logs remain in scratch.

IDA Free 9.4 was located and attempted with `-A`, an IDC export script, scratch
output DB/log paths and a hidden process. Startup reported **`Fatal registry
error: Access is denied.`** No IDA disassembly was produced; the owned process
was terminated after the bounded 20-second attempt. Do not claim IDA success.

## Containment and ownership

`measurements.json` and the seven `.gate.txt` files preserve actual control
outputs, source/object hashes, normalized scout hashes and exact row lists.
For every non-baseline cell:

- Scout is the only changed existing code owner; all 38 inherited exact rows
  remain exact; no code owner disappears.
- All nondebug noncode sections are unchanged under the strict comparator.
- Fake-match scan: zero leads (not an admissibility verdict).
- A has no new owner. B/AB/C/D/C+D emit only one new owner,
  `_point_from_line2d` (48 bytes); no new runtime relocation names it.
- That helper is identical to January's selected action_vehicle copy and the
  current provider, normalized SHA-256
  `d90e91bc9340636a11d9fe06c53bd87fad47428860899fd41c08c65be2cbe273`.
  Both selected-provider input orders have no duplicate-definition error.
  Unresolved externals remain expected; this is not a whole-program link.
- Canonical `vehicles.c` hash before/after is identical; no source or config
  patch was applied. Only scratch and this research packet were written.

The new helper's only reconstructed emitter is non-exact scout. Thus successful
identity/coalescing alone does not meet the strict-exact-caller admission rule.
The aggregate cast controls also remain held. No variant is a landing patch.

## Next falsifiable experiment, without a source sweep

Reuse the fixed scalar-left and aggregate-left artifacts with the compiler
debugger. Track `vehicle_left` from the two-source forms into the stack-slot
allocator. Test the specific prediction that its address-observed/aggregate
storage classification differs **before** packing, explaining the -88 versus
-76 slot group. A same classification with different packing would falsify
that mechanism and shift attention to lifetime/interference construction.

For exact scout, reopen only with a genuine first-party construct or header
context that predicts the +0x628 leaf order while keeping all preceding sites
fixed. Do not repeat a +1 temporary search or add an unattested leveling helper.
The scalar AB patch is the most useful no-aggregate-copy research starting
point (101 -> 29 regions); the held aggregate patch is the one-region compiler
reproducer. Neither earns exactness credit.

## Reproduce

From this repository root:

```powershell
python research/astra_breakthroughs_20260925/scout/probe.py
python research/astra_breakthroughs_20260925/scout/audit_new_helper.py
python research/astra_breakthroughs_20260925/scout/run_ghidra.py
```

Generated `.c`, `.obj`, full `/FAsc` listings, normalized disassemblies,
REAL-region diffs, fake-scan receipts and primary-tool outputs live under
`scratch/astra_scout_20260925/`. Compact canonical-relative source patches and
actual gate output live beside this ledger. The patch filenames are experiment
identities, not admission recommendations.
