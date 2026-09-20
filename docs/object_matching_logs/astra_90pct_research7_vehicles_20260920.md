> Research only: no admission or retained source/header/configuration change.

# Packet 7 vehicles: bounded primary reconstruction, no admission

Closed with **two scratch source shapes, zero exact gains, and no production change**. The target remains `_update_alien_scout_physics`, 2,456 meaningful / 2,464 padded bytes. Both shapes preserve all 35 exact canonical controls. Neither is a landing candidate. No shared header, configuration, Ninja build, commit, compiler option, comparison rule, or target object was changed.

The frozen baseline is commit `76dfc1fb779a082752550f795ca356d8c5f4b93f`. `baseline.c`, `baseline.obj`, and `baseline.json` preserve its source, actual production object, hashes, complete owner inventory, exact controls, and three relevant residual measurements. `review.py` independently confirms that the current production source and object still equal these frozen files. The projection-header test was performed by root separately and restored before either vehicles compiler invocation.

## New primary evidence and its limits

`raw-primary.json`, `cache-scout.asm`, and `cache-rtc-records.json` derive directly from the supplied `halo_cache_symbols.exe`, SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Scout begins at **0x8f80b0**, with a dynamic stack-alignment prologue that the initial ordinary-prologue heuristic missed. Its return ends at 0x8f930a and its RTC descriptor is 0x8f930c. The 17 RTC local records identify, among others:

| RTC local | Frame offset | Bytes |
|---|---:|---:|
| `applied_alpha` | -316 | 8 |
| `offset_from_vertical` | -332 | 8 |
| `angular_velocity` | -348 | 8 |
| `alpha_desire` | -364 | 8 |
| `alpha_bonus` | -384 | 8 |
| `turning_torque` | -408 | 12 |

The fresh HCEA DIA records in `hcea-update_alien_scout_physics-sym.txt` independently name 12 aggregate locals, including `applied_alpha`, `alpha_desire`, and `turning_torque`. They also corroborate the four-parameter signature. The PPC storage locations and flattened optimized local list do not prove January lexical scopes. The two builds are later evidence; equality of source revision to January is **unproven**.

Actual Ghidra output is `ghidra_008f80b0.asm` / `.c`, with receipts in `ghidra.log`. The original project lacked a defined function at this aligned entry. `AstraVehiclesLaunch.java` disassembled and defined it only in an in-memory transaction that was rolled back; the primary project was closed without saving. `scout_temporary_analysis` is an analysis label, not a recovered symbol. The earlier `ghidra_008f6a20` and `ghidra_008f7650` exports are reconnaissance and are not used as scout evidence.

The decisive raw sequence is:

- **0x8f8809..0x8f881a:** copy the two global-zero components to RTC `alpha_desire`.
- **0x8f8843 and 0x8f886e:** two calls through thunk 0x4075b3, passing the same `alpha_desire` as input and output, first with `offset_from_vertical`, then with `angular_velocity`.
- The constant bytes at **0x9401b4 / 0x9e2f80** are `000080bf` / `000070c1`, exactly -1.0 / -15.0.
- The thunk jumps to **0x444020**. `cache-point_from_line2d.asm` proves two component operations `result = p + t*v`, returning the result pointer. The existing genuine `math/real_math.h` helper has that operation graph. The semantic helper name is inferred from its body, not from an unavailable PE symbol table.
- **0x8f8b5a..0x8f8bba** adds the level terms into the same `applied_alpha` aggregate; **0x8f8bc4..0x8f8c02** adds throttle terms in the alternate branch. The common tail reads these same two components at **0x8f8c34 / 0x8f8c64**.

January's corresponding two-stage zero subtraction is independently visible at scout **+0x3f3..+0x421**, and its applied torque path at **+0x54c..+0x574**, with physics moments consumed at **+0x58c / +0x5b8**. This correspondence supports testing the real helper and aggregate operation graph. It does not establish the precise original C spelling or justify importing later layouts.

## Two measured shapes

| Shape | Padded / relocs | Instructions | Frame | Differing relocation-zeroed byte positions | Exact controls |
|---|---:|---:|---:|---:|---:|
| Frozen production | 2464 / 91 | 777 | 0xa0 | 1340 | 35 |
| `scout-two-dimensional-helpers` | 2464 / 91 | 777 | 0xa0 | 1342 | 35 |
| `scout-applied-alpha` | 2464 / 91 | 778 | 0xa0 | 1313 | 35 |
| January | 2464 / 91 | 775 | 0x94 | 0 | 39 target owners total |

Shape 1 replaces only four component subtractions by the two authenticated in-place `point_from_line2d` calls. It does not alter local declarations, branches, scope, or the shared math header. All input components are already assigned and the initialized result is reused in place.

Shape 2 builds on shape 1 and restores the single branch-owned applied-torque aggregate: it adds level torque or throttle into the existing initialized `control_torque`, then consumes that vector directly. The reconstructed separate `torque_a` / `torque_b` scalars are removed. Each control component is counted exactly once. This is the primary-recorded aggregate use, not a field-order or declaration-filler sweep. Its smaller byte distance remains **zero credit**.

The exact source snapshots, patches, preparation rationale, objects, complete gate receipts, normalized disassemblies, and full aligned diffs are saved under both stems. Full normalized hashes and relocation receipts are in `review.json`; no fuzzy measurement is treated as exactness.

## Containment and ownership

`review.json` proves for both shapes:

- All 35 inherited canonical exact functions remain exact; no new canonical exact function appears.
- Scout is the only changed existing code owner. Human plane, fighter new, ghost effect, and all other existing owners remain byte/relocation identical.
- All **87 nondebug noncode sections / 2,564 bytes** remain identical, including relocation identities. COMMON remains empty.
- `_point_from_line3d` remains absent, and the mandatory emitted-symbol guard passes.
- The authentic header call naturally emits one additional `_point_from_line2d` owner, **48 bytes / 0 relocations**. It has no incoming runtime relocation. This new object copy earns no credit and supplies no whole-object admission.
- The frozen baseline has 69 code owners against 39 canonical target owners; each shape has 70. Existing four vehicle-specific helper owners are inherited debt and were not changed or newly introduced.
- The baseline and both scratch files have zero `fake_match_scan.py` findings. No assembly, volatile steering, fabricated storage, foreign local prototype, anonymous stub, or compiler-flag probe was introduced.

## Rejected hypotheses and reopening boundary

The full vehicles ledger and its latest corrections were read before probing. Its historical exact-copy, operand-order, lifetime, direct-PD, and 778 fighter slot probes remain rejected. No `vehicles` record currently exists in `config/parked.json`; the documented source and point-ownership vetoes still apply.

The old ledger's categorical statement that January's zero-subtraction sequence is an inexpressible compiler artifact is too strong: a later primary build performs precisely that sequence through real helper calls and a named initialized vector. **The fresh evidence corrects that explanation, but both supported reconstructions fail January exactness.** It is not permission to resume scalar expression sweeps.

The remaining scout frame is still 0xa0 versus January 0x94, and scheduling/spill differences remain. Its direct relocated-call multiset already agrees with January. The later 17-local RTC map and HCEA's 12 optimized locals do not authenticate a January allocation rule. A third shape was not spent on declaration order, fabricated scope, helper suppression, or transcribing all later 3D calls; the point3d ownership restriction remains unresolved.

Human-plane and fighter-new fresh DIA/January measurements were retained as controls only. No common newly proved mechanism justified a source shape in either. Fighter-new retains the explicit point-owner veto. Reopen only with independent January source/context evidence or an authenticated shared mechanism that predicts the remaining frame/operation difference and preserves the 35 controls and full ownership constraints. No zero-gain cleanup is proposed.

## Reproduction

Run from the repository root; these commands only read primary/build inputs and write this scratch packet:

```powershell
python scratch/astra-wave7/vehicles/collect_scout_raw.py
python scratch/astra-wave7/vehicles/review.py
```

The two already executed compiler commands, recorded for reproducibility rather than an invitation to sweep, are:

```powershell
python tools/campaign/gate.py source/units/vehicles --source scratch/astra-wave7/vehicles/scout-two-dimensional-helpers.c --all --out scratch/astra-wave7/vehicles/scout-two-dimensional-helpers.obj --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/units/vehicles --source scratch/astra-wave7/vehicles/scout-applied-alpha.c --all --out scratch/astra-wave7/vehicles/scout-applied-alpha.obj --forbid-emitted-symbol _point_from_line3d
```

Do not rerun these under another agent's transient header experiment. `collect_primary.py` preserves the fresh DIA command lines and initial read-only primary census; `run_ghidra.py` records the optional actual Ghidra export. Coordinate access to the shared Ghidra project before repeating that export.
