# `contrails.obj` Jonas initialize-CFG wave (2026-08-25)

## Result

This bounded source-only wave starts from cumulative integration commit
`ad1a152c933f98052bac310f2886b1d00302d9a2` in the isolated branch/worktree
`jonas/contrails-init-cfg-20260825`. It changes only
`source/effects/contrails.c` and this ledger. The first and only normal
code-producing compile makes `_contrails_initialize` strict exact. The object
advances from 5/19 to 6/19 exact functions, from 117 to 226 exact meaningful
code bytes, and from 144 to 256 exact padded code bytes. It remains
`NonMatching`; the thirteen absent target bodies and their 3,994 meaningful
bytes remain outside this wave.

The retained source is ordinary typed C. It contains no assembly, volatility,
`register`, optimizer pragma/barrier, attribute/intrinsic, raw offset or
address, pointer or union pun, inactive-union access, undefined behavior,
synthetic anchor, object-byte patch, comparator exception, or compiler/config
change.

## Frozen provenance and tools

- Baseline source blob: `f5acdb5dbf6a52203697612c3250b18fc9391d13`.
- Retained source blob / SHA-256:
  `9457d3ec44edfa41886fdef765e84f178c7181e9` /
  `b14de2f45902c6045389deaed095e2f04858d5540f393b10fa262b9870d0cbdd`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Pinned split `contrails.obj` SHA-256:
  `4285b4c7ede8ae9a36dce1b3bf6259f3ec839df869fdf465cd5ac9c3fb3b1955`.
- Source-identical baseline candidate-object SHA-256:
  `7fb91f46d4100dc4db0a7b37054c8304a6cd3477e3ef3d5c59d1c91d78513ebb`.
- First-shot candidate-object SHA-256:
  `dd6221ffeda7c7719a775f3aff77e9029918df89f60cc18b9911a2e388e519c5`.
- XDK 3911 `CL.Exe` 13.00.9254.1 SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Compiler edge: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged
  generated include list.
- csplit SHA-256:
  `708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`.
- objdiff-cli SHA-256:
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
- Frozen regression manifest:
  `build/audit/contrails_init_cfg_baseline_20260825.json`, 294,398 bytes,
  SHA-256
  `bfafa36338245144df6fae355523b255a6cc473bc8a92e1421fd7b7445fb8621`.

Whole-object raw digests are artifact sentinels only. Strict authority is the
function-section fingerprint: meaningful and padded bytes, relocation address,
type, resolved destination and addend, symbol ownership, and runtime non-code
sections.

## Validated baseline

The baseline emits six of nineteen target functions. Five are inherited strict
exact; `_contrails_initialize` is the only emitted residual. The remaining
thirteen target functions are absent, not partially credited:

`_contrail_delete`, `_contrails_reconnect_to_structure_bsp`,
`_code_000860c0`, `_code_00086150`, `_local_random_range`,
`_local_random_vector_in_cone3d`, `_contrail_scale_random_value`,
`_code_00086420`, `_code_000864b0`, `_code_00086870`, `_contrail_new`,
`_contrail_owner_collision`, and `_contrails_update`.

| Function | Meaningful / padded | Relocations | Baseline normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_contrail_scale_value` | `31 / 32` | `0` | `b5aef6d77a20a8ca082d21250ac322e27141ae8d3596983afafb9901db17d3c3` | inherited exact |
| `_contrails_disconnect_from_structure_bsp` | `1 / 16` | `0` | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | inherited exact |
| `_contrails_dispose` | `31 / 32` | `4` | `57c2ce5afbda11a72425091cebf748417b9bccf805b372d1aa9ba35baf61818d` | inherited exact |
| `_contrails_dispose_from_old_map` | `27 / 32` | `4` | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` | inherited exact |
| `_contrails_initialize_for_new_map` | `27 / 32` | `4` | `f46019b6b2911cc9b501f1f00bce5914007e4274a822670cbc98cd64d56a2272` | inherited exact |
| `_contrails_initialize` | `109 / 112` | `13` | `66707b33ab60f9d1a0786b62712d65d424838296153def8ab9a46ddfb7da0415` | sole emitted residual |

For `_contrails_initialize`, target and baseline are both 109 meaningful / 112
padded bytes with thirteen relocation identities. The target hash is
`9d6ad7a4dad684659cb1a240c99a0c3bf30fd94861902060910f25727a1b7cad`.
The first true normalized divergence is `+0x3E`: January branches from the
both-allocated test directly to the return at `+0x53`, while the nested source
branches to the later common return at `+0x6C`. The target then gives the
point-only state its own clear/error/return block and shares the bottom error
between the contrail-only and neither-allocated states. Baseline shares the
opposite pair. This also explains the six displaced tail relocations without
changing their identity multiset.

The baseline owns three runtime literal COMDATs used by the emitted lifecycle
cluster, all already strict exact and unchanged by this wave:

| Literal owner | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `"contrail"` | `9` | `82f44cfc2e5bf9da8dcb29805ccae135e447bf860996f9af728930d7fb1303dc` |
| `"contrail point"` | `15` | `bf32606d93a9cefe8fc0359aaa7f768c8b1c2485a92eb1b62d717c2e5fc46666` |
| allocation error text | `35` | `ad5be53b92e2365564f0efec03db373e31da5440b67b303ab73cc53e07529e18` |

The target contains 448 runtime non-code bytes. This partial translation unit
claims no new data progress and does not change storage ownership.

## Claude and durable-history audit

Claude's reachable `claude/untried-objects-20260816` frontier was read before
candidate selection. `research/residual_frontier.json` records
`source/effects/contrails` as a one-function, 112-byte defined residual and
classifies `_contrails_initialize` as `UNKNOWN`, `unclassified`,
`recipe: null`, and `confidence: NONE`. It supplies no source recipe or prior
rejection. Claude's shape-transfer documentation discusses only the absent
`_local_random_range` donor and explicitly treats that transfer as a
false-positive lesson; it contains no initialize experiment.

All reachable history for `source/effects/contrails.c` has four commits. The
initialize body was introduced once at `01e22d8b` with the exact nested shape
present at this baseline and is unchanged in every later source blob. A
cross-ref search of Claude research, repository ledgers, branches, and durable
source versions found no flattened four-state form. The candidate is therefore
genuinely novel, while Claude's null recipe is treated only as a census—not as
positive evidence.

The wider Claude direct-hit audit also excluded the tempting TIFF, HS, actor
perception, LRUV, periodic, IMA, debug-memory, HS-runtime, props, and stale R5
leads as already tried, exhausted, rejected, flag-confounded, or
reconstruction-scale. In particular, the retracted `0x10715873`
register-assignment claim and the retracted thread/profile data-layout screen
were not used.

## Fixed candidate and one-shot result

The retained source exposes the four allocation states directly:

1. both allocations succeed: return;
2. only point data succeeds: clear point data, report the error, return;
3. only contrail data succeeds: clear contrail data and fall through to error;
4. neither succeeds: report the same bottom error.

This is the simplest natural form supported by the target CFG. It adds no
temporary state, cast, declaration, signature, literal, header, or data owner.

| ID | Source shape | Target | First-shot result | Siblings | Decision |
| --- | --- | --- | --- | --- | --- |
| C01 | Flatten the nested success/failure tree into the four natural allocation states above | `109 / 112`, 13 relocs, hash `9d6ad7a4...` | `109 / 112`, all 13 relocation addresses/types/destinations/addends exact, hash `9d6ad7a4...` | all 5/5 inherited exact functions remain exact | **retained** |

The target/candidate relocation tail is exact at `+0x41` to
`_contrail_data`, `+0x46` to the allocation-error literal, `+0x4C` to
`_error`, `+0x59` to the same literal, `+0x60` to
`_contrail_point_data`, and `+0x65` to `_error`. The complete thirteen-entry
ordered relocation sequence is exact, not merely the six changed entries.

The frozen candidate passed an XDK `/Zs` syntax-only pass. The Ninja dry run
then contained exactly one `CL build/base/source/effects/contrails.obj` edge.
That edge was invoked once. The emitted object was copied immediately to
`build/audit/contrails_init_cfg_first_shot_20260825.obj`; no spelling,
candidate, configuration, compiler, or retry followed.

## Regression, admission, and progress validation

- Hardened full emitted-function sweep: 6/6 strict exact, including all five
  inherited functions and the newly exact initialize body.
- Frozen pre-wave regression check: `changed_nonexact: []`, all 5/5 inherited
  functions in `still_exact`, and only `_contrails_initialize` in
  `newly_exact`.
- The sole non-runtime gate delta is `.debug$S`, 183 to 184 bytes, caused by
  source-line metadata. `.drectve`, all function COMDATs, all three runtime
  literal COMDATs, relocation semantics, and meaningful symbols remain
  stable.
- Full Halo and libcmt graphs: no work after the isolated candidate edge; the
  complete copied cumulative graph remains green.
- Strict semantic audit: 470 units, 4,239 functions evaluated, 4,098 semantic
  exact, 114 hidden exact / 64,705 hidden code bytes, 4,159 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 4,133/11,060 accepted exact
  functions, 501,918/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds exactly one function
  and 109 meaningful code bytes. Halo becomes 3,966/7,574 functions and
  489,004/1,770,166 code bytes. Object and data counts do not change.
- Admission audit: zero candidates, zero revocations, and only the inherited
  `source/shell/shell_xbox` completion-label contradiction.
- Parked-function audit: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.

No semantic exception, regression adjudication, park entry, symbol-ownership
entry, object-status change, or configuration edit is needed.

## Committed-state replay

Implementation commit `0cddfec893012c494372b48dcb53e0492e0f2316`
was clean before replay. Its strict manifest is
`build/audit/contrails_init_cfg_committed_20260825.json`, 294,398 bytes,
SHA-256
`fa5adfb20e6f34502892305be746354dae3701f829a8cbdcc52332d021660819`.

The resolved replay target was verified inside the isolated worktree and
`build/base/source/effects/contrails.obj` was literally deleted. Ninja then
ran exactly one compiler edge to recreate it. The rebuilt object was preserved
as `build/audit/contrails_init_cfg_committed_replay_20260825.obj`, 2,920
bytes, raw SHA-256
`d18cba5ca4d9627a85c06f46a1c44df817b297954c726d7e17e829c9766a43ad`.
It differs from the first-shot raw object only at COFF header offsets `+4` and
`+5`, the low bytes of the compile timestamp. Every modeled section, symbol,
meaningful and padded code byte, and relocation is identical.

The committed-manifest regression check is fully clean: no failures, warnings,
newly exact, or changed-nonexact entries, and all 6/6 functions are in
`still_exact`. Direct hardened comparison again proves all six emitted
functions strict exact. Full Halo and libcmt dry-run closure reports no work.

## Corrected cumulative-HEAD closure

The isolated commits integrate on `jonas/units-integration-20260824` as
implementation commit `ecf73592` and replay-ledger commit `1adee65b`. At
corrected HEAD `1adee65b508cdbdee577219f40c5ecd67d51bed9`, the resolved
cumulative `contrails.obj` path was verified inside the worktree, literally
deleted, and rebuilt through the one expected XDK edge.

The first corrected-HEAD replay object is 2,919 bytes with raw SHA-256
`c691f5d81498be25ac1516405030461371a35ae9d3adc7a33050ee80a5c2367a`.
After reports were regenerated, a clean corrected-HEAD manifest was written as
`build/audit/contrails_init_cfg_corrected_head_20260825.json`, 294,398 bytes,
SHA-256
`abd730449d2cbd9b4c1dc52c3fd1c24a78fc06ec0b447ba7345d6080c22a12e6`.
The object was then literally deleted and rebuilt a second time. That replay
is 2,919 bytes with raw SHA-256
`ff716ea43acd9b2bb64fb0e77833f218a54cd81ed70cacabfb2b1f9def4529a3`.
The two raw objects differ only at COFF header offset `+4`, a compile-timestamp
byte.

The corrected-manifest check is fully clean after the second rebuild: no
failures, warnings, newly exact, or changed-nonexact entries, with all 6/6
functions in `still_exact`. Direct hardened comparison again proves the six
function sections and all twenty-five relocations exact. Unlike the isolated
LF worktree, the normal cumulative CRLF checkout reproduces the baseline
183-byte `.debug$S` section; the pre-wave gate therefore has no failures and
only the expected `NEWLY_EXACT` review warning.

Final cumulative closure regenerates csplit, ordinary and semantic reports,
then confirms the same +1 function / +109 meaningful-byte gain. Full Halo and
libcmt dry-run closure reports no work; admission remains zero candidates and
zero revocations; parks remain 3 active / 0 stale / 0 invalid; and the complete
tooling suite remains 179/179 passing. The cumulative worktree is otherwise
clean.

## Do-not-repeat and residual disposition

Do not retry nested/flattened Boolean-equivalent spellings of the accepted
initialize topology: C01 is already exact. Do not treat Claude's
`_local_random_range` shape-transfer false positive or its null initialize
recipe as authority for the thirteen absent bodies. Those bodies remain
reconstruction work and need authenticated semantics, types, call graph, and
strict target evidence before a future bounded wave.

Do not use assembly, volatile or `register` scheduling, pragmas, intrinsics,
attributes, raw offsets/addresses, type or union puns, undefined behavior,
synthetic callers/anchors, byte patches, comparator exceptions, or alternate
compiler flags. Reopen `_contrails_initialize` only if the pinned target,
compiler, or strict comparator changes; it is otherwise closed exact.

`contrails.obj` remains active and `NonMatching` at 6/19 exact functions,
226/4,220 meaningful and 256 padded exact code bytes, with no new data credit.
Corrected cumulative-HEAD replay evidence follows after integration; this
committed-state evidence is the ledger-only closure for the isolated branch.
