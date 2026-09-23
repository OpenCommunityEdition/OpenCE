# Five Halo object admissions and one CRT draft (2026-09-23)

## Credited result

Five Halo-owned units moved from `NonMatching` to `Matching` only after guarded function comparisons, target-owned data/owner checks, and ordinary selected-provider link checks:

| Unit | January functions | How this batch closed it |
| --- | ---: | --- |
| `source/cache/cache_files_decompress_windows` | 46/46 | Five newly exact functions; first-party debug types support volatile `flags` and `read_progress`, and the flag parameter is narrow. |
| `source/units/biped_limp_noodle` | 6/6 | Two newly exact constraint/relaxation functions using existing typed math helpers and `realcmp`. |
| `source/ai/actor_type_infection` | 5/5 | Newly exact `_infection_swarm_control`; direct component arithmetic was replaced by existing vector/point helpers. |
| `source/ai/actor_type_flood` | 2/2 | Existing exact code and data admitted after the shared-helper provider link was repaired. No new code bytes claimed. |
| `source/structures/structure_visibility` | 21/21 | Existing exact code and data admitted after the shared-helper provider link was repaired. No new code bytes claimed. |

The current strict COFF snapshot versus `scratch/claude_structural_merged_20260923.json` gains nine functions / **9,109 padded bytes** with **zero regressions**. Eight of those functions are Halo-owned: **8,925 meaningful code bytes** (cache 2,117; biped 3,199; infection 3,609). Halo credited code is now **1,547,308 / 1,770,166** (87.41%). The ninth is CRT `__ioinit` (101 meaningful bytes); it does **not** close its object. Flood and visibility were already strict-exact in isolation, so their admission is an ownership/link result, not newly matched code.

## Source and ownership proof

The cache `volatile` qualifiers have independent HCEX/HCEA type-record support, not a register-forcing guess. They preserve January's field layout. Biped's apparently repeated distance-epsilon test and two queue-tail blocks are attested by January's distinct branches at 0x59a, 0x5d7, 0x6d4, and 0x713; the second enqueue has real effects. Infection retains typed vector access and removes redundant casts. All changed Halo source passed the fake-match scanner with no new leads.

`action_charge.c`, `action_obey.c`, `geometry.c`, and `decals.c` now obtain their relevant shared math helpers from the genuine `real_math.h` inlines instead of hand-written external duplicates. The previously exact functions in those provider objects remain exact. The selected helper definitions are byte-identical and use coalescible SELECT_ANY COMDATs. Bounded ordinary VC7 base/base links pass both tested input orders for limp/infection with ActionCharge, Geometry, and ActionObey, and all six orders tested for flood and visibility with their selected providers. These partial links check duplicate ownership; `/force:unresolved` is used only because they deliberately omit unrelated objects, so they are not proof of a complete executable link.

Named target-owned noncode sections, sizes, flags, bytes, relocations, and symbol storage/offsets match for all five: cache 5,945/5,945 data bytes; limp 6/6 sections including the 44,808-byte BSS; infection 12/12; flood 7/7; visibility 25/25. The raw objdiff function percentages for flood and infection remain below 100 despite strict `coff_compare.section_infos_equal` gates and exact target-owned sections; the `Matching` label is based on the stricter COFF/owner checks, not those fuzzy percentages alone. Candidate-only coalescible helpers receive no target-byte credit.

## Held CRT result

`libs/libcmt/ioinit.c` is now present as `NonMatching`: `__ioinit` is strict exact (101 meaningful bytes), and `__ioterm` has byte-identical 35-byte instructions but one unresolved *relocation spelling* difference. January names `___piob+0`, while our natural 64-entry table loop names `___pioinfo+256`; the final image address is the same. A trial spelling with `__piob` changed code size to 50 bytes and encoded linker adjacency, so it was rejected. The 40-byte `.data` and 4-byte `.CRT$RII20` sections match. The descriptive initializer name `io_initialize_handles` replaces an address placeholder in the symbol map, but the object remains uncredited as a whole until its relocation identity is reconciled without source fakery.

## Verification

Full `ninja` passes. Strict snapshot: 8,245 functions, 7,574 exact; +9 exact, 0 lost. Parks: 103 active, 0 stale, 0 invalid; the five cache and two biped entries were retired only after exact gates. Tool tests: 1,154 passed, 5 skipped, 26 subtests. `git diff --check` passes. Admission audit has the same 26 pre-existing data-incomplete status contradictions as the incoming baseline; none of the five units is newly contradicted. No whole-object or data credit is claimed for `ioinit`.
