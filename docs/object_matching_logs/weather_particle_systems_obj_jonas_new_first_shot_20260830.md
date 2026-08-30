# `weather_particle_systems.obj` Jonas new-system first shot (2026-08-30)

This additive ledger records the sole bounded production candidate for
`_weather_particle_system_new` in
`source/effects/weather_particle_systems.c`. The candidate reproduced the
complete 352-byte padded envelope and all 19 relocation destinations, but the
final random-density call region had a different legal argument-evaluation
schedule. Strict equality therefore fails closed. Production source was
inverse-restored, no function or object credit is claimed, and no candidate
source is retained.

## Scope, authority, and prior-attempt census

- Isolated starting tip:
  `4d53b40aa259613da06b76a9f537cdf9952dcf91`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `weather_particle_systems.obj` SHA-256:
  `3ebaff7040870aff906a53c36fd84c8dccaa33b6c4be1b15101b42e1b1354c89`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Natural flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`, with the unchanged
  XDK 3911 and repository include paths.
- Baseline/restored source Git blobs: `weather_particle_systems.c`
  `b1fbbfab204aeb856c4f6e7c272c18ccbc342a99` and header
  `565bb84abe07cb2f73ae793e7d8cd18d19c2db97`.
- Frozen pre-edit whole-TU manifest:
  `build/audit/weather_particle_system_new_pre_20260830.json`, SHA-256
  `08078658f3565294bb04abb5e7b46a578c2aa262762d0cbfcdc7483a37781f1f`.

Every applicable `AGENTS.md`, `CLAUDE.md`, campaign/tooling document, tree
checkpoint, and object ledger was read before selection. There was no existing
Weather Particle Systems object ledger. Complete source history, all
registered worktrees, retained audit artifacts, research caches, HCEA, and
Claude documentation contained no earlier bounded candidate or exact artifact
for this owner. Registered-tree hits were inventory declarations/comments,
not definitions. Scratch and untracked material was excluded from provenance.

Two independent public-source lines authenticate the natural behavior and
layout:

- HCEA commit `aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`,
  `src/weather_particle_system_new.c`, Git blob
  `5b86d1b345e78d42e54061157ff12d18a61d3877`, file SHA-256
  `7632960a2b73a052f8b4ad70de14a43b0c755a2704d3e7bd7b54198ca45baf04`;
- Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`,
  `src/halo/effects/weather_particle_systems.c`, Git blob
  `c61734f6fb6f17af888b444c9fbe1ac459a2b035`, file SHA-256
  `493087109592d9cd5329d8ba393459468a596478b9aa88b9224c50d3d8d8fb05`.

January COFF, disassembly, line-number strings, and relocations remain the
authority. The donors establish semantics only.

## Baseline and authenticated natural candidate

Before and after this lane, `weather_particle_systems.obj` has 4/20 strict
functions and 224/5,936 strict padded code bytes. The four inherited exact
owners are:

- `_weather_particle_systems_initialize`;
- `_weather_particle_systems_initialize_for_new_map`;
- `_weather_particle_systems_dispose_from_old_map`;
- `_weather_particle_systems_dispose`.

January establishes a typed local-player system at `_bss_0043d590+4`, a
`'rain'` tag lookup, source-line assertions at 91, 177, and 102, a 0x9C
per-player system datum, eight 0x10 type-state entries, and a tag block of
0x25C type definitions. The natural candidate used named and size-asserted
structures for those layouts. It initialized the definition, scale, time,
time delta, active-system count, first-particle index, particle count, random
density range, and box width in the independently authenticated order.

The candidate was link-closed through ordinary typed declarations. Its calls
resolve to `_display_assert`, `_system_exit`, `_tag_get`,
`_tag_block_get_element_with_size`,
`_get_global_local_random_seed_address`, and
`_real_seed_random_range`. No private register ABI or absent callee was
introduced.

The frozen candidate source had Git blob
`ee53721a40876413d6c1fd887963415b9ebe05e9` and working-file SHA-256
`cee27e840605b8c7499a3200825421d52f73cd8fd1976bcc6325d47290cdf172`.
It was readable typed C89, with parameters one per line and an explicit
terminal return. A source-policy audit found no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw offset or address,
pointer/integer reconstruction, representation pun, undefined behavior,
byte-forcing device, object patch, or compiler/tool exception.

The production compiler's `/Zs` syntax-only gate passed without object
emission. Exactly one normal Ninja production edge then emitted candidate
code. No spelling, declaration, expression, local-lifetime, argument-order,
or control-flow variant was compiled.

## Sole production measurement

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 349 / 352 | 348 / 352 |
| Relocations | 19 | 19 |
| Normalized SHA-256 | `e45cfa272c4995c969c91a0aea395fdb31be22ea51965d081a7389937eb6d529` | `08e93bac534aa8ff07fc3fb527f3e2733d0cc301c588411c1c7ccff192fd3201` |

The ignored evidence object is retained at
`build/audit/weather_particle_system_new_first_shot_20260830/weather_particle_systems_candidate.obj`.
It is 4,291 bytes and has raw SHA-256
`f3cddc21343f7af612222e9d4477a21ed560f9b1fb991655797e2b7485ec922b`.

All first 17 relocation addresses, types, and symbolic destinations agree,
including the shared assertion string whose producer is undefined in the
January split but defined in the candidate TU. The final two destinations also
agree, but their addresses do not:

| Ordered destination | January offset | Candidate offset |
| --- | ---: | ---: |
| `_get_global_local_random_seed_address` | `+0x12C` | `+0x123` |
| `_real_seed_random_range` | `+0x132` | `+0x131` |

The fixed boundary is the final density-range region. January loads the upper
and lower density bounds, records both locals, pushes the upper and lower
bounds, and only then obtains and pushes the seed. The frozen natural source
obtains the seed first, reloads the two bound locals, and then pushes the
upper bound, lower bound, and seed. Both subsequently call the same random
range owner and store the x87 result, but January schedules the box-width load
after that call while the candidate schedules it before the loop-tail state.
The candidate consequently has one fewer meaningful byte despite the same
padded envelope.

The candidate whole-TU regression preserved all four inherited exact
functions. Its only changed nonexact owner was
`_weather_particle_system_new`; the two additional assertion-string COMDATs
accounted for the expected temporary data/symbol-set changes.

## Do not repeat and reopening condition

- Do not retry this typed HCEA/Pastudan candidate topology.
- Do not sweep seed locals, density-bound locals, declaration order,
  expression order, helper spelling, loop shape, or include order around the
  measured fixed point.
- Do not encode the observed instruction order with raw offsets, casts,
  aliasing, byte views, barriers, volatile/register steering, or another
  prohibited device.
- Reopen only when new authenticated source provenance independently proves a
  different source-level seed/bounds evaluation topology. The January schedule
  is measurement evidence, not permission for a tuned retry.

## Restoration and validation

All candidate includes, typed layouts, field rename, and body were removed by
inverse `apply_patch`. The final source/header Git blobs exactly equal their
baseline identities above. The restored raw base object SHA-256 is
`08852bfa58a018b5be8a81c53a14b474c948b246cb8ce0fb3174ab6477863fad`.
The frozen whole-TU regression reports all four inherited exact functions
`still_exact`, with no changed nonexact, newly exact, failure, or warning.

- Aggregate `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,810 accepted exact, one
  ordinary-only, one ordinary structural, zero ordinary rejected, and zero
  unit errors.
- Progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Strict board: 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Protected Units remains 189/189 and 54,864/54,864 padded bytes. Sentinel
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Python tooling suite: 212/212 pass; the sole warning is the sandbox-denied
  optional pytest cache.
- Admission audit: zero candidates, contradictions, or revocations.
- Parked audit: 12 active, zero stale, zero invalid.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- Final `git diff --check`, source-policy review, and protected-source review:
  pass.

No production source, header, configuration, compiler flag, semantic
exception, parked entry, object patch, or candidate object is tracked by this
lane. Nothing was pushed.
