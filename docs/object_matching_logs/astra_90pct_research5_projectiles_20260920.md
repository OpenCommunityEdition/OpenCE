> Research-only: zero new exact credit; no production changes.

# Wave 5 projectile evidence — no new admissible shape

This packet produces **zero new candidates, zero exact credit, and no production changes**. Fresh primary evidence does not establish a missing January operation or an untried source mechanism that plausibly closes either principal residual. It instead sets a firm limit on using the later debug build as a local-order oracle. The current source already contains the shared aggregate types, aggregate extents, and translation/velocity commit boundary examined here.

The frozen baseline is `7a6ae71e614638753d04d6d04acbec56c7b8f636`; the copied and current `source/items/projectiles.c` SHA-256 is `6033f3955d4835d62712746af962b81a39e538860da671ae37cb83c7fa6a4876`. All evidence below is under `scratch/astra-wave5/projectiles/`. No shared header, compiler flags, build rules, comparator, or point owner was changed.

## Baseline and admission

Fresh scratch compilation yields **26 exact / 4 residual / 0 unwritten**, with the forbidden `_point_from_line3d` emission guard passing. The complete baseline census records 52 function definitions, 109 named code/data owners, 56 non-debug noncode sections, and zero COMMON definitions. The 26 exact controls are enumerated in `primary-evidence.json`; complete noncode contents and relocation identities are in `baseline-comparison.json`. There is no changed object to admit and no claim that the residual baseline's noncode or ownership is newly exact against January.

| Residual | January size / relocations | Baseline size / relocations | Result |
|---|---:|---:|---|
| `_projectile_update` | 4064 / 179 | 4064 / 179 | SHA mismatch |
| `_projectile_collision` | 2928 / 100 | 2928 / 100 | SHA mismatch |
| `_projectile_collision_test_line` | 512 / 10 | 496 / 10 | Residual, not reopened |
| `_projectile_new` | 432 / 16 | 432 / 16 | Park retained |

Unchanged `alndiff` produces 1167 target versus 1168 baseline instructions for update, 920 equal instructions and 167 changed hunks. Collision has 914 versus 909 instructions, 593 equal and 175 changed hunks. These preserve stack displacements, addresses and full relocation identities; they are diagnostic counts, not exact credit. The first update mismatch is already at `+0x35`: `time_remaining` lives at target `[ebp-0x18]` versus baseline `[ebp-8]`; the following collision-count initialization is target `[ebp-0x34]` versus baseline `[ebp-0x38]`. These are observed allocation differences, not evidence authorizing arbitrary declaration permutations.

## Fresh primary evidence

`collect.py` ran actual Ghidra 12.1.2 headlessly against a read-only domain object, decompiling update at `0x69c9e0` and collision at `0x697ae0`; both completed. It also ran DIA2Dump against the HCEA PDB for both symbol and line records. No GUI, Ghidra transaction or project save was used. `ghidra.log`, `collection.json`, both `ghidra_*.asm/.c`, and `hcea_*_{sym,lines}.txt` are fresh receipts. IDA remains `IDA_AUTOMATION_UNAVAILABLE` under the parent's authorization.

`primary_evidence.py` independently reads the PE bytes with pefile/Capstone, validates complete instruction spans, decodes the two RTC descriptors, hashes the PE/PDB and receipts, and extracts January proofs from the original COFF object. It does not use decompiler text as raw-instruction proof.

The PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Update's RTC descriptor `0x69e8ac` contains 23 aggregate records; collision's descriptor `0x699128` contains seven. HCEA has 14 typed update locals and 128 line records, and seven typed collision locals and 115 line records. Its source receipt is `d:\projects\code\hcex\sources\items\projectiles.c`, MD5 `1FE8A7D87D93FC7F075CA924B6764412`. These are later PPC type/line records, not January x86 scalar allocation records.

The useful shared observations are already implemented:

- Collision's private signature is `void(long, collision_result *, real_point3d *, real_vector3d *, float)`. HCEA identifies `effect_points` as `real_point3d[5]` and `effect_vectors` as `real_vector3d[5]`; RTC gives both 60-byte extents. The current arrays and parameter types agree. Two separate 84-byte `damage_data` RTC records do not authorize a new lifetime or declaration-order edit.
- Update's shared records authenticate vectors, points, collision and sound-location aggregates. They omit the complete scalar local topology needed to resolve `time_remaining`, `collision_count`, `moved`, `flyby` and timer-flag allocation.
- The velocity result is committed after object translation. January calls `_object_translate` at `+0xdfa`, then stores local components to object velocity at `+0xe11`, `+0xe16`, `+0xe19`. The PE calls its translate thunk at `0x69e47e`, then stores RTC `new_velocity` components at `0x69e48f`, `0x69e494`, `0x69e49a`. Current source already does this. The physics late-axis correction therefore supplies no missing aggregate-copy operation here.

## Provenance limits verified in actual instructions

A 2020 timestamp and compiler mismatch alone would not disprove a same-source-revision claim. This packet instead records concrete differences in the compiled function graphs. It does not claim to distinguish changed source from build-conditional source for every difference.

- January update's timer cases 1 and 2 both reach the stopped-bit `0x10` test at `+0xd2`. The PE switch has an additional case 3, a case-1 collision-bit `0x4`/definition-bit `0x80` path, and a case-3 `0x800` test. The PE's `0x69cb87..0x69cc4e` instructions prove this directly. Importing this graph would contradict the January target.
- The later guidance graph tests definition bits `0x2000`, `0x800`, `0x1000`, and `0x400` at `0x69cffe`, `0x69d1d1`, `0x69d460`, and `0x69d51b`. RTC includes `ideal_point`, `vector_to_target_normalized`, `basis`, `forward_vector`, `up_vector` and `right` within this larger graph. Their existence cannot authenticate missing January locals or a full declaration order.
- January's contrail collision time is `(1.0f - time_remaining) * seconds_per_tick`, retained at `+0xe48..0xe6a`. The PE uses a per-iteration value at `[ebp-0x1c]` minus `[ebp-0x10]` at `0x69e4e1..0x69e4e6`, then passes it without that multiplication. This is another reason to restrict cross-build inferences.
- Before detonation, the PE has object-role tests, a `0x1000` flag test and a call to `0x69a9d0` at `0x69e615`; January `+0xeee..0xf36` contains the arming gate, detonation and deletion without that intervening graph. No operation is imported from this later path.
- Collision really calls the magnitude-squared helper twice, at `0x698729` and `0x698744`, through thunk `0x4090e3 -> 0x42df70`. The helper's primary bytes compute the three squared components. January retains one computation and an x87 value at `+0x5a1..0x5e4`. The previously rejected duplicate-call shape is therefore not reopened merely because it exists in this later build.

The older September 19 wording treating RTC as a same-revision full local-order receipt is too strong for these functions. The precise correction is: RTC supplies authentic local names/extents for its own build; narrow shared facts require independent January correspondence. This scratch packet records that correction without altering older logs or production source.

## Exhausted work respected and reopening evidence

Before considering a shape, this pass read the September 14 PDB/IDA, w2 and w3 projectile logs, the September 19 evidence closeout and point-link-selection log, relevant runtime/callback and Jonas exact-quad history, current parks and source history. Prior results include U3 timer spellings, U4 explicit scaling, U5 declaration/loop-scope permutations; collision speed/noise scopes and duplicate magnitude computation; and 287 candidate files / 183 unique source candidates without improvement beyond 26 exact. The mixed inline/external point ownership problem was not probed again.

No new compiler-shaping declaration order, fake lexical scope, manual helper expansion, changed point linkage, or later-source branch was tested. A justified reopening needs one of:

1. January-compatible full scalar/local lifetime evidence with actual lexical boundaries for the update slot and flag groups.
2. A source-backed VC7 aggregate/alias/lifetime rule that predicts both named residuals' allocation or retained x87 value and survives exact sibling/owner controls.
3. New original compiler-context evidence for the mixed point inline/external ownership law, sufficient to distinguish it from the already exhausted global header sweep.

The packet is research only. It recommends no production edit and supplies no candidate JSON.

Reproduce from the checkout root with the `gate_command` recorded in `projectiles/primary-evidence.json`, then `python scratch/astra-wave5/projectiles/compare_projectiles.py` and `python scratch/astra-wave5/projectiles/primary_evidence.py`. To refresh primary tooling, run `python scratch/astra-wave5/projectiles/collect.py` first. Use the existing canonical compile flags; no Ninja invocation is necessary.
