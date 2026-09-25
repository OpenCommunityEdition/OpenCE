# MANIFEST - source/hs/hs_runtime storage packet (12 converters)

- Base: f6d00a8c (worktree HEAD b7c1b7d9 = round-2 lane marker; `git diff f6d00a8c HEAD -- source/hs/hs_runtime.c config/symbols.json` empty).
- Tip: candidate `scratch/w/hs_runtime_storage/cand.c`; patch `scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch`
  (LF, 12 hunks, 24 changed lines; `git apply --check` clean in the worktree).
- Claim: sub-key `source/hs/hs_runtime (storage: 12 converters)` claimed by this worker; whole unit `source/hs/hs_runtime`
  is held by sibling `r2w1-hs_runtime::_render_debug_trigger_volumes (1088 B)` -> integrator must sequence the two patches
  (verified composable: offset -8, byte-inert on the sibling's c2 object).

## Changed construct and evidence
Only change: `static` storage-class keyword on the 12 prototypes (hs_runtime.c:469-491) and 12 definitions (:1541-1647) of
hs_long_to_boolean, hs_short_to_boolean, hs_string_to_boolean, hs_data_to_void, hs_short_to_real, hs_long_to_real,
hs_enum_to_real, hs_real_to_short, hs_real_to_long, hs_long_to_short, hs_object_name_to_object_list, hs_object_to_object_list.
- cachebeta.pdb publics: none of the 12 is public (January = file-static). Code publics in RVA 0xB9C00-0xBA600 are only
  _hs_runtime_dispose, _hs_can_cast, _hs_cast; converters sit at 0xBA220-0xBA38F. Controls typecasting_procedures,
  hs_cast, hs_can_cast are public and external in source.
- HCEX.pdb: all 12 `static function: true`, UndecoratedName `static long hs_<name>(long)` (hcex_all.txt); hs_cast has a public.
- symbols.json:3203-3214 already `"static": true`; split storage 3.
- References: grep over source/ and libs/ finds the 12 names only in hs_runtime.c (the typecasting_procedures matrix,
  address-taken) -> no private ABI possible, no header/consumer changes.
- The 20260904 "Deliberate linkage exception" (static helpers dead-stripped until the genuine matrix exists) no longer
  applies: the matrix is present and its .data section (owner _hs_type_inspectors, 9804 B) is object_audit ok.

## Per-function (target vs candidate)
| function | size | relocs | norm sha256 (12) | target storage | cand storage | gate |
|---|---|---|---|---|---|---|
| _hs_long_to_boolean | 32/32 | 0/0 | d3d4fa4df898/d3d4fa4df898 | 3 | 3 | EXACT |
| _hs_short_to_boolean | 32/32 | 0/0 | bb79c2783b6b/bb79c2783b6b | 3 | 3 | EXACT |
| _hs_string_to_boolean | 32/32 | 1/1 | 9f0feb2eae2c/9f0feb2eae2c | 3 | 3 | EXACT |
| _hs_data_to_void | 16/16 | 0/0 | 33cb7c6b5695/33cb7c6b5695 | 3 | 3 | EXACT |
| _hs_short_to_real | 32/32 | 0/0 | edad63c921e9/edad63c921e9 | 3 | 3 | EXACT |
| _hs_long_to_real | 16/16 | 0/0 | 081250b595ba/081250b595ba | 3 | 3 | EXACT |
| _hs_enum_to_real | 32/32 | 0/0 | d59469372c5e/d59469372c5e | 3 | 3 | EXACT |
| _hs_real_to_short | 32/32 | 1/1 | 178756d045bd/178756d045bd | 3 | 3 | EXACT |
| _hs_real_to_long | 16/16 | 1/1 | bbe139825c1f/bbe139825c1f | 3 | 3 | EXACT |
| _hs_long_to_short | 16/16 | 0/0 | 19871109ce6a/19871109ce6a | 3 | 3 | EXACT |
| _hs_object_name_to_object_list | 64/64 | 3/3 | 558f8479d239/558f8479d239 | 3 | 3 | EXACT |
| _hs_object_to_object_list | 48/48 | 2/2 | 7c6008d88c3a/7c6008d88c3a | 3 | 3 | EXACT |

Whole unit gate (cand): exact 64 / residual 2 / unwritten 0 - rows identical to baseline:
- residual 528 _hs_evaluate_begin_random [sha] (T, parked private-register-convention; not touched)
- residual 1088 _render_debug_trigger_volumes [sha] (sibling's claim; not touched)

## Byte-inertness
objcmp base.obj vs cand.obj: 172/172 sections identical (bytes, flags, section_infos_equal), 505/505 symbols, only the
12 storage classes 2 -> 3. base.obj == build/base/source/hs/hs_runtime.obj (0 diffs). Composed with sibling c2.c:
0 section diffs vs sibling c2.obj, 12 storage diffs, gate 65/1/0.

## Audits
| check | base | candidate |
|---|---|---|
| object_audit symbols | 129, 12 differ | 129, 0 differ |
| object_audit total | FAIL (14) | FAIL (2) = the 2 .text residuals |
| pdb_storage (cand via pdb_storage_cand.py) | 12 disagreements | 0 |
| provider_link | FAIL (12) NO JANUARY PROVIDER | PASS (25 surplus rows, both orders) |
| surplus_identity (4 COMDATs; cand sections identical to production) | 0 not identical | 0 not identical |
| data (.data _hs_type_inspectors 9804 B, .bss, .rdata) | ok | ok |

## House-rule review
Storage keyword only; prototypes stay in the object's own .c next to the other file-static prototypes
(hs_inspect_* are already `static`). No casts, no new declarations, no helper expansion, no header edit. Strip test N/A
(no decoration). Declaration count unchanged.

## Credit
- Meaningful code credit: 0 B (converters were already EXACT). Data: 0 B.
- Admission: removes 12 object_audit symbol rows, 12 pdb_storage disagreements and 12 provider_link failures.
  Object remains NonMatching (2 residuals). No symbols.json edit, no config status flip, no parks retired.

## Held / reopen
- Object completion depends on _render_debug_trigger_volumes (sibling) and _hs_evaluate_begin_random (retired T tie,
  4 ModRM bytes). The 4 surplus header COMDATs (_scale_vector3d, _set_real_point3d, _set_real_vector3d,
  _vector_from_points3d) are byte-identical to January's providers and pair-link PASS, but whether they are admissible
  depends on the render_debug_trigger_volumes route (rule 6: strict-exact caller required).
- HCEX gives `long f(long)` signatures; the 20260904 ledger records that forcing them loses January identity - not re-probed.
