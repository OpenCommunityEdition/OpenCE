# Independent root review: perception source corrections

Admissible as an authenticated structural correction, with **zero exact
credit**. The root independently rebuilt committed wave1 source905e5e3b and
the worker's candidate through separate gate objects before landing.

The candidate changes only two source facts in actor_perception_update:

1. The orphan-selection accumulator is one initialized
   `interesting_orphan_index`, retained through the loop and final target
   veto. The inherited source invented a separate nearest-index local and a
   final copy. Fresh HCEA names one long interesting_orphan_index; the
   unoptimized cross-build maintains one local through initialization,
   selection and the final store. Every read remains dominated by its NONE
   initializer or a selecting write. Both versions choose the same index.
2. The acknowledgement decrement arm repeats `hostility==0` and
   `!attached_to_us` before testing the uncertain-combat timer. Ghidra's
   preserved primary disassembly0x476063..0x476084 explicitly repeats both
   conditions before0x476086..0x476096 tests NONE/60. The outer else arm
   already implies these field values; there is no intervening call/write.
   These are independently authenticated source-owned tests, not invented
   dependencies or a general license to duplicate conditions.

The excluded scope and 2D helper variants are not part of this landing. In
particular, the helper probe's two extra COMDAT owners are not introduced.
No declaration filler, new ABI/header, representation cast, volatile,
uninitialized read, forced inline or manual arithmetic rewrite is retained.

Independent script `scratch/astra_wave2_perception_review.py` proves all35
inherited exact functions remain exact; only the existing fuzzy update body
changes among the full function sections. Runtime named owners, noncode
payload/relocations and COMMON are unchanged, and the emitted-point guard
passes. Complete candidate fake scan has zero findings.

The candidate remains4720 padded bytes with170 relocations and a non-target
normalized hash. The worker's alignment-aware count improves from62 to26
differing instruction rows after excluding unchanged relocation display
notation; that diagnostic is not an admission criterion. Remaining return
and x87 schedules are documented in the perception packet. January bytes
and relocations remain final authority; no whole-object or function credit
is claimed here.

Evidence: `scratch/astra-wave2/perception-independent-review.json`, fresh
before/candidate sources/objects/gate logs and fake JSON. The later executable
is a2020/v140 cross-build, source revision unverified; HCEA is typed2011
corroboration. Neither is claimed as original January local debug data.
