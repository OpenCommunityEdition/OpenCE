# AI Profile `_ai_profile_change_render_spray` strict first shot (2026-08-30)

> Superseded ownership claim (2026-09-04): the 236-byte csplit span below
> establishes adjacency, not a single original C aggregate. The current
> reconstruction uses the natural 224-byte meter-definition array and a
> separate private 12-byte spray-name table. A target symbol at the independently
> referenced +224 boundary names the latter. The earlier wrapper and its
> purported unknown short (ordinary alignment padding) are removed; the original
> measurements below are retained as historical evidence, not ownership proof.

## Outcome

`source/ai/ai_profile.obj::_ai_profile_change_render_spray` is recovered in
readable typed C89 and passes the hardened January gate on complete padded
bytes and every ordered relocation identity.

| Measurement | January target | Candidate |
| --- | ---: | ---: |
| meaningful code bytes | 56 | 56 |
| padded COMDAT bytes | 64 | 64 |
| ordered relocations | 6 | 6 |
| normalized SHA-256 | `e4e3fc4f00585b05400d569c411e35adc7c28bf8dfc26f4d1e8b144123639864` | `e4e3fc4f00585b05400d569c411e35adc7c28bf8dfc26f4d1e8b144123639864` |

The sole production candidate object is preserved at
`build/audit/ai_profile_render_spray_first_candidate_20260830.obj`; its
whole-object SHA-256 is
`8c08a716e88a3f59105438d2a2649847004cb8fab3bb765299cac76a65f6b555`.
The final generated object has the same whole-object hash.

## Provenance and preflight

The January object at `build/split/source/ai/ai_profile.obj` is the byte and
relocation authority. Its disassembly proves the complete natural topology:
signed 16-bit load from `ai_profile+2`, increment, signed remainder by three,
16-bit store, signed table index, `console_printf`, and a final 16-bit return
in AX. The target data owner independently proves that
`global_ai_meter_definitions` is 236 bytes: 28 eight-byte typed meter
definitions followed at `+0xE0` by the three pointers to `"none"`,
`"actions"`, and `"activation status"`.

Stian's authenticated October/XBE reconstruction at commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`,
`src/halo/ai/ai_debug.c` Git blob
`10ec13825807640fba4160ba3fa340f161d32533`, payload SHA-256
`0d4df27fdef050b909eeed26e29ff7329b92fd171f69194eacedc98e0f7c3803`,
independently records the same three-state signed-remainder operation, names,
format string, console call, and short return. Its `hs.c` documents that the
script evaluator discards that short result, matching January's caller.

Before source mutation, the applicable repository/Claude/agent documentation,
acceleration playbook, all AI Profile ledgers, parked/frozen manifests, current
source/header/caller topology, ancestry, registered worktrees, local donor
trees, target code, target relocations, and target data owner were inspected.
No prior January body, measured attempt, parked entry, or frozen boundary was
found. The two current callers are a debug-key wrapper and the no-argument HS
evaluator; both use ordinary cdecl and discard the result, so the function is
link-closed.

The pre-edit source blobs were
`ba1b55f0b016b19377d0eeedd949f2f97c1ee16e` for `ai_profile.c`,
`b3ace0c97693f4d59c0e91a7d6d686a221cfc824` for `ai_profile.h`, and
`184fc67391f7ff0a64d9201ae5bfdbd32cd22bea` for `hs.c`. The retained blobs are
respectively
`23de57255755480847a8a7b3b9429afab30c7456`,
`2145859e5cd5e643d7f62812b8a2169840148b78`, and
`dae19bd89160dd90443135c1afe1e43ea32e0dbb`.

## Typed closure and exact packet

The local January profile view now splits the previously opaque first dword
into two signed shorts and names the proven second field `render_spray`.
Evidence-backed meter types model 28 `{ meter_id, unknown short, sample_proc }`
records followed by the three render-spray name pointers. That typed layout
produces the target's `global_ai_meter_definitions+0xE0` relocation naturally;
there is no raw offset access.

The public prototype and the existing HS declaration are corrected from
`void` to the target-proven `short`. Full-width cdecl calling convention is
unchanged. Independent strict checks prove both dependent callers remain exact:

- `source/main/debug_keys.obj::_debug_key_show_actor_spray`: 32 padded bytes,
  one relocation, hash
  `217415e87ea516dc372c69e9c65746375d544f2c2872cb1764cb4b215515e861`;
- `source/hs/hs.obj::_code_000b1500`: 32 padded bytes, two relocations, hash
  `49e44f141c4b5cf32a43fc2f7341462fefd3ba643e95f9def8d828c29fadf36b`.

The complete ordered relocation schedule is identical:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x03` | `DIR32` | `_ai_profile+2` |
| `+0x13` | `DIR32` | `_ai_profile+2` |
| `+0x1D` | `DIR32` | `_global_ai_meter_definitions+0xE0` |
| `+0x23` | `DIR32` | `"AI line-spray: %s"` `.rdata` |
| `+0x2A` | `REL32` | `_console_printf` |
| `+0x30` | `DIR32` | `_ai_profile+2` |

There is no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
barrier, raw address or runtime byte offset, representation pun, undefined
behavior, byte forcing, compiler change, tool exception, or admission
exception. Exactly one natural production source form was compiled; no body,
declaration, ordering, control-flow, or compiler variant was tried.

## Translation-unit and full gates

The clean pre-edit regression manifest is
`build/audit/ai_profile_render_spray_pre_20260830.json`, SHA-256
`27463a9729df48ceb70d388a0783a5d741bb056f2cbad17a0658383445885327`,
captured from canonical commit
`b654ee4f9e1a84181d5b6eeed816f2d2c1fa198a`.

The candidate check reports `_ai_profile_change_render_spray` as the sole
`newly_exact` function and `changed_nonexact: []`. Restoring the target-order
COMDAT before the five already exact count getters necessarily renumbers their
function/debug sections, so the frozen pre-addition manifest reports five
`UNKNOWN` evidence changes and the corresponding anonymous `.debug$F`/symbol
set shifts. These are additive target-order structural findings, not packet
losses: independent hardened comparison proves all five getters remain exact,
along with the four initialization/disposal siblings. All ten current exact AI
Profile functions pass complete padded-byte and ordered-relocation comparison.
The newly emitted format-string owner is the only new `.rdata` owner. The
pre-existing `_global_ai_meter_definitions` owner remains exact at 236 bytes,
10 ordered relocations, and normalized SHA-256
`3a69a8b8a83037c3277fa86c09c7aa4e9b9da8b480e3facb87275548ee4c88b8`.
No regression or admission exception suppresses these findings.

Complete `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
`progress` targets pass. Semantic audit scans 470 units and 4,940 functions,
reports 4,823 semantic-exact and 4,833 accepted-exact functions, and has zero
unit errors. Campaign progress is 384/833 linked objects, 4,794/11,060
functions, and 609,120/2,198,102 code bytes; Halo is 282/468 linked objects,
4,627/7,574 functions, and 596,206/1,770,166 code bytes. Final
`build/report.json` and `build/semantic_report.json` SHA-256 values are
respectively
`4adce7a8642cd185d2dbcf85c21da82ce7f9c3402e094d1e8f5906c9f0da2759`
and
`bf631c1778c06d0277d363790db573c03bdab59e92482d9a823f94c77a688e3c`.

Admission remains zero candidates, zero contradictions, and zero revocations.
Park validation remains 12 active, zero stale, and zero invalid. All 212
tooling tests pass. `git diff --check` and the changed-source policy scan pass.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Vehicles, Units, Matrix Math, AI Debug, and all other excluded or concurrently
owned source paths remain untouched. No GitHub push is performed.
