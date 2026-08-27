# `hs_library_external.obj` Jonas orient cluster first shot (2026-08-27)

This ledger records one frozen production experiment for the object-create-anew
wrapper and the complete same-TU object orientation/teleport cluster. All five
functions are strict on the sole code-producing compile and are retained.
`hs_library_external.obj` advances from 19/36 to 24/36 accepted functions and
from 1,312 to 2,208 padded exact bytes. It remains `NonMatching`; no whole-object
or data-completion claim is made.

## Authority and scope

- Cumulative base: `7da35b5b07b70b64363465abe3e7a44c68b9c57f`.
- Baseline source blob: `2b30b4f3c2ad004e5d36d6c160387c78c2c8fc74`.
- Retained source blob before commit: `88453837cb8f4566d14b83468c399c2a3fa442f1`.
- Retained physical source SHA-256:
  `4b2da855928efe13cb6b97e35f161961063686e4948982526fa99f7d0c3648b1`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged ordinary Ninja
  production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and ownership
authority. The only tracked implementation change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked path.
No header, configuration, completion status, semantic allowlist, parked entry,
compiler flag, comparator, admission rule, or tool changes.

The local and GitHub tools trees were explicitly screened for the user's
publication request: all 121 `tools/**` paths are tracked, with zero changed or
untracked tool paths. There is therefore no separate tools payload to publish.

## Prior work and provenance screening

The applicable Claude guidance and the existing object-matching ledgers were
read before mutation. In particular, the 2026-08-21 five-body ledger documents
one rejected `hs_object_create_anew` candidate derived from the longer HCEA
behavior. That 64-byte miss was removed without a retry. The current attempt is
not tuning that candidate: complete January disassembly independently proves a
shorter 48-byte three-call implementation with exact calls to
`object_index_from_name_index`, `hs_object_destroy`, and `hs_object_create`.

The semantic/source-topology donors were:

- `build/audit/refs/halocea/src/blam/hs/hs_object_create_anew.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_orient.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_teleport.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_object_set_facing.c`;
- `build/audit/refs/halocea/src/blam/hs/hs_teleport_players_not_in_trigger_volume.c`.

HCEA supplied readable behavior only. January supplied the exact local data
layout, branch shape, assertion strings and lines, call identities, private ABI,
padding, and relocation evidence.

## January packet and exact results

The functions retained by the one-shot object are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_object_create_anew` | 48 | 3 | `5e1d387f1cd88d67179915c6700612a8edd4c565bef5bbb4bd16ef1ae42b57e7` |
| `_code_000b9500` | 656 | 35 | `b50c7cdbd1737cf758d9c6b8c50d10f03ea1d5ca15930e2a6dc32bcbc63b2de2` |
| `_hs_object_teleport` | 32 | 1 | `eaf785c4831944d32d332a48114c3a96bdf92e78ae620c502eaca6ea8e188c62` |
| `_hs_object_set_facing` | 32 | 1 | `58f169018b3f2faabac38aa32daa5c6b1f99bf4aae1c2a5dab0b17797c8f4f26` |
| `_hs_teleport_players_not_in_trigger_volume` | 128 | 8 | `a39ac0f46517dc1b291a3a81734eeebf9351874a56748bab36f525e1c609905e` |
| **Wave gain** | **896** | **48** | |

The hardened comparator reports identical normalized padded bytes and equal
relocation count, order, address, type, symbolic destination, addend, and
non-code ownership for each function.

The private helper takes the object index through VC7's inferred private EBX
convention while its three stack parameters remain ordinary typed C parameters.
The two public single-object wrappers and the multiplayer loop were compiled in
the same translation unit as the complete helper, so caller and callee jointly
prove that convention.

## Retained readable behavior

`hs_object_create_anew` ignores `NONE`, resolves an extant named object,
destroys it when present, then creates the named object. The orientation helper:

- obtains the object and typed scenario cutscene flag;
- validates `&flag->position` at January line `0x1CC`;
- exits a unit seat or detaches a non-unit parent before moving;
- derives and validates `&forward` at January line `0x1DF`;
- resets the object and transforms facing into parent-node space when needed;
- updates unit desired facing, aiming, and looking vectors;
- uses player teleport/control paths for player-owned units; and
- delegates the remaining optional position/facing update to
  `object_set_position`.

The public wrappers select `(TRUE, TRUE)` for teleport and `(FALSE, TRUE)` for
set-facing. The multiplayer wrapper walks `player_data`, tests each live unit
against the trigger volume, and teleports only units outside it.

The implementation uses C89 declarations, explicit terminal returns, typed
object/unit/player access macros, and typed tag-block access. It contains no
assembly, `register`, `volatile`, pragma, intrinsic, attribute, compiler
barrier, raw address, pointer/integer reconstruction, cast or union pun,
undefined behavior, synthetic anchor, byte forcing, object patch, comparator
exception, or alternate compiler control.

## Frozen first-shot proof

Before source mutation, the regression manifest was written to
`build/audit/hs_library_external_orient_cluster_baseline_20260827.json`,
SHA-256
`a3344e600828db637e7ad85b84b508ab588d789c89161e17f531126bf7ad7406`.
It freezes commit `7da35b5b` and the sole unit
`source/hs/hs_library_external`.

Exactly one code-producing edge ran after the source topology was frozen:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The compiler repeated the inherited C4047 diagnostic at the extant effect
marker call; both effect functions remain independently strict exact. The first
object was immediately preserved at
`build/audit/hs_library_external_orient_cluster_first_shot_20260827.obj`,
SHA-256
`7b66beed8f6addf13a01ba856ed0d8f5468d2fb86341d4a9465be1cb0c842238`.
No candidate retry or post-compile source adjustment exists.

## Regression adjudication

The fail-closed regression check reports exactly the five new functions as
`NEWLY_EXACT` and `changed_nonexact: []`. It flags the 19 inherited accepted
functions, runtime `.rdata`, debug data, and symbol set because inserting five
new COMDATs and target-owned constants changes section/evidence indices.

No adjudication file or waiver was used. Direct hardened comparison confirms
all 19 inherited functions remain strict exact:

- `_code_000b8eb0`, `_code_000b9330`;
- `_hs_damage_new`, `_hs_damage_object`;
- `_hs_effect_new`, `_hs_effect_new_from_object_marker`;
- `_hs_not`, `_hs_print`;
- `_hs_object_create_anew_containing`, `_hs_object_create_containing`,
  `_hs_object_destroy_by_name`, `_hs_object_destroy_containing`;
- `_hs_object_list_get_element`, `_hs_object_set_shield`;
- `_hs_objects_delete_by_definition`, `_hs_objects_predict`;
- `_hs_sound_get_gain`, `_hs_sound_set_gain`; and
- `_hs_trigger_volume_test_objects_any`.

Thus the gate findings are reviewed section churn, not inherited code
regressions or a matching/admission exception.

## Validation and progress

- Complete Halo and libcmt build targets: pass, with no work remaining after
  the selected edge.
- Split, ordinary report, semantic report, and progress generation: pass.
- Strict board: 260/619 exact objects, 4,296/8,246 functions, and
  556,727/1,922,669 padded bytes.
- `hs_library_external.obj`: 24/36 functions and 2,208/3,824 padded bytes.
- Semantic audit: 470 units, 4,456 functions evaluated, 4,305 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,365 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `bea7885a365b968cd08ae1d83fd49b065c947b46e810be780a2c8de4a451648d`.
- `build/semantic_report.json` SHA-256:
  `450dbd1b194fd401010c45463a612e24a91efc678ca23da12cb7805c49d174ec`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 194 passed, with only the known unwritable
  pytest-cache warning.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching` in `config/config.json`.
