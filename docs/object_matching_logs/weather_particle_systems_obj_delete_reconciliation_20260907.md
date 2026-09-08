# Weather delete reconciliation — 2026-09-07

## Accepted scope

Baseline: published `62b3cec7c55c27ebc5aaf28f664a61763ac16f00`.
The first natural integration adds **444 meaningful / 464 padded exact bytes**:
`weather_particle_system_delete` (271/272), `weather_particle_system_get`
(55/64), `weather_particle_system_get_type` (71/80), and
`weather_particle_system_type_delete_particle` (47/48).

Canonical reaches **931,272 / 2,198,102 meaningful code bytes**, **6,289 / 11,060
credited functions**, and **6,335 / 8,245 strict rows**, four gains and zero
regressions. Objects remain **391/833 Matching**; data remains
**2,061,024 / 4,176,062**. Weather is **8 exact / 0 residual / 12 unwritten**,
not a completed object. New-system, rendering and update work are excluded.

Donor: read-only `C:/halo-worktrees/fable-50k-resume-20260906`, tip
`740bbc33aeaa7c2c6ee86304b7a52279df44a930`; selected evidence in
`scratch/reports/weather_particle_systems.md` and
`scratch/hdr/weather_particle_systems.md`. The prior canonical new-system
first-shot hold remains intact. No whole-file/config import or residual
source-shape search occurred.

## Source, schema and naming

The proper definitions header owns the 0x30 `rain` tag record and typed getter;
the systems header owns the public delete prototype. Full TU-private runtime
records replace the inherited prefix facade: particle type 0x10, system 0x9C,
particle 0x54, globals 0x274. January proves four systems, unlike later HCEA's
two. Root read all four January bodies/relocations and the selected HCEA
definition, runtime-layout and deletion references. Those references corroborate
semantics/layout, not original January source or identical cross-build behavior.

Deletion saves the full next datum handle before calling destructive deletion,
then decrements the short count and updates the head. It asserts valid local
player before tag access; type validation follows the tag lookup. The natural
private ABIs are SI→EAX, EDI/SI→EAX, and ESI respectively. Their genuine public
caller naturally preserves January's inline schedule; no forced inline,
synthetic caller or extra math owner is introduced.

`weather_particle_system_get_type` is corroborated by the January symbol atlas;
the other private/global spellings are documented semantic reconstructions.
Three private code metadata rows gain names and `static: true`. The BSS row is
renamed only. **The inherited linkage gap remains: target BSS storage 2,
candidate storage 3.** The preliminary intake's claim that both were static was
incorrect. Absence of external references does not establish original linkage;
this packet neither conceals that gap nor claims whole-object admission.

## Verification

- Full Ninja passes; all 572 base and 833 split objects audited. Only Weather
  changes beyond timestamps. Game, the other systems-header consumer, is
  whole-object identical. Fresh dependency lists are valid (Weather 22/Game 114).
- All 10 inherited runtime records survive under one explicit in-memory BSS
  name mapping. Exactly four code owners, one genuine 61-byte assertion literal,
  and three imports (`tag_get`, `datum_get`, `datum_delete`) are added. Gate and
  ordinary output have the same complete 15-record inventory. No new global,
  COMMON, math helper or `point_from_line3d` owner is emitted.
- All 30 target sections, raw payloads, relocation tables and auxiliary records
  remain unchanged; only four names and three code storage fields change.
- Root independently replayed the reviewed actual-x86 harness: **354 executions
  (59 cases × 2 x87 modes × 3 roles)** and two specifically rejected instruction
  mutations. It covers real private ABIs, invalid indices/assertion ordering,
  salted handles, freed-record poisoning, complete arenas/guards, the target's
  legitimate argument-slot overwrite, nonvolatile registers and stack state.
  External tag/allocator/diagnostic APIs are modeled; this is not full-engine,
  allocator or final-link equivalence. Baseline is metadata-only for new bodies.
- Full tools suite: **1,120 passed, 2 skipped, 26 subtests passed**. Parks:
  297 active, zero stale/invalid. Admission: zero candidates/contradictions,
  five inherited rejections, zero revocations. Source scan and diff check pass.

## Frozen evidence

All paths below are canonical-relative. All **2,303 final capture hashes equal
the first ordinary capture**; source/config/compiler/report/object inputs did
not drift during verification.

| Artifact | SHA-256 |
| --- | --- |
| `scratch/weather-delete-before-20260907.json` | `cc7e7848a5ea594f8999b274ea2868cd150d913d5237fd6a09afbd42ca53d3ce` |
| `scratch/weather-delete-first-20260907.json` | `535d620b7dae3a35c4baa50516bfb3caf2450cd466006dcd637e0d6ec52009aa` |
| `scratch/weather-delete-final-20260907.json` | `e6f7c0812ebcaac1494b7c4bfe335c969c4204520cdeb7de9e1f4c084fc906e2` |
| Original target | `3ebaff7040870aff906a53c36fd84c8dccaa33b6c4be1b15101b42e1b1354c89` |
| Ordinary base object | `e850dafa71ac22dffc2c37270746abf48730cf205ebf37a81200ef888031aba3` |
| `scratch/weather-delete-root-gate-20260907.obj` | `12ee8f4e56554d34c77cf7c3caa41adda3c1ae0ecabc182f432675a40dae8673` |
| `scratch/weather_delete_independent_owner_audit_20260907.py` | `be01f1b2361bbc2f24b6a92d1e516876ed99473852ffd29c4f9a7b91d08d0e35` |
| `scratch/weather-delete-independent-owner-audit-v2-20260907.json` | `f8c780aa6999855519224e8a2c9cad8febd0e758f8541b456ba89dc6adcaeaa5` |
| `tools/audit/weather_delete_runtime.py` | `db7d69ec294b16c5bd673358d6a02cd363cf3a0524c0cfe264e79d0c9881877b` |
| `tools/test_weather_delete_runtime.py` | `1b4bdb97e57d3333a063c87a088339be6df05238860c8921c5aa637366c1ef7f` |
| `scratch/weather-delete-runtime-root-final-20260907.json` | `101757d047d64f9da6080b5cade10578ff6a31053df504e6caa3b13ebf4ef191` |

Publication is verified after commit; only source, tools and documentation are
published, never original binaries, private reference assets or scratch objects.
