> Archived research proposal: NOT LANDED; zero exact gain. Production remains at wave3.

# Independent physics review — PASS, zero credit

Reviewed `scratch/astra-wave4/physics/axis-and-force.c` against a fresh `git show 7a6ae71e614638753d04d6d04acbec56c7b8f636:source/physics/physics.c` baseline. Read the worker's primary evidence packet, then independently rebuilt both complete sources through PID-isolated `gate.py` scratch compilation. No new source shape, production/header/config edit, Ninja invocation, or compiler-option change was made.

The candidate contains exactly two substantive changes: separate initialized in-place force additions and angular-velocity capture after `object_translate`. Both are directly supported by January instructions and fresh reads of the later executable. Acceptable as source corrections; **neither closes `_physics_update_old` or earns coverage credit**.

## Independent primary verification

January target `+0xff6` computes `EDI = object + 0x3c`, the angular-velocity field under the current real object owner type. Translation is called at `+0x104a` with relocation `_object_translate` at `+0x104b`. Only after that call do `+0x104f/+0x1051/+0x1054` read the three dwords through EDI, store the axis locally, and normalize it. Moving the existing declaration initializer to an assignment after translation restores that cross-call value-capture boundary. The candidate does not read the uninitialized local before that assignment.

Raw PE instructions independently reread in `review.py` show `0x7bfc72` calling the translation thunk, followed by `0x7bfc7a..0x7bfc94` copying the later build's object angular velocity to frame offsets `-0x2b4/-0x2b0/-0x2ac`. Independently parsing the actual RTC descriptor at `0x7c015c` names this 12-byte frame object `axis`. The raw translation thunk resolves to `0x781fc0`. This confirms timing without treating decompiled source or DIA's PPC register spellings as x86 evidence.

January target `+0xb3d..+0xb6d` reads the existing force components at `+0x118/+0x11c/+0x120`, adds normal force at `+0x84/+0x88/+0x8c`, and stores each result back. `+0xb73..+0xba3` separately adds ground friction at `+0x90/+0x94/+0x98`. The actual owner `source/physics/mass_point_datum.h` has those fields. The candidate restores both floating-point additions and their intermediate stores instead of merging them.

Initialization is proven: January `+0xe5..+0xfb` multiplies the mass-point count by `0x130` and calls `_csmemset` with zero and the original mass-point pointer, before entering the point loop. The same initialization remains in the candidate. Reading the existing force therefore does not introduce an uninitialized read. Raw PE call sequences at `0x7bf773` and `0x7bf7a1` pass `(force, normal_force, force)` and `(force, ground_friction, force)` respectively. The thunk at `0x406339` resolves to `0x44e8e0`; independently disassembled body instructions establish component-wise additions.

The later executable SHA256 independently matches `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. It is a 2020/v140 cross-build; same-source-revision claims remain unverified. January alone establishes both proposed operation boundaries, with later raw instructions corroborating them.

## Independent compiler and ownership controls

The independently built baseline and candidate reproduce every function measurement in the worker's objects. Both gates pass the `_point_from_line3d` emitted-owner exclusion.

| Check | Result |
| --- | --- |
| Listed strict exact controls | All 13 preserved |
| Function owner count | 40 -> 40 |
| Defined code/data owner count | 86 -> 86 |
| Added/removed owners | None |
| Changed owners | `_physics_update_old` only |
| Non-debug noncode sections | All 42 strictly byte-and-relocation equal |
| COMMON | Identical, empty |
| Point helper emission | Absent |
| Fake scanner | Zero findings in baseline and candidate |

The full noncode check uses `section_infos_equal`, including relocation identities, not only hashes/counts. All other residual function bodies remain unchanged too.

Baseline is 5,456 padded bytes / 118 relocations, SHA256 `8e46f2b6ccf73fb4cbebd7ec52451a52d7b8b86030de04bfd467a592126ddcb9`. Combined is 5,504 / 118, SHA256 `6a50206dcd7fc7e9c120ee41c8d3f6bbd7378a6aa7ab7be6e90a6fde6420bc38`. January is 5,168 / 115, SHA256 `bee483283d8e9c3aa3c752e1a6097b5322604fc5c9379bf60528256d2dab08ed`. Thus these corrections remain residual despite their authenticated source behavior.

Exact controls: `_friction_evaluate`, `_get_material_type`, `_physics_compute_biped_collision`, `_physics_compute_unit_collisions`, `_physics_get_features_in_sphere`, `_physics_instance_new`, `_physics_test_point`, `_physics_test_vector`, `_physics_update`, `_physics_update_new`, `_pin_fraction`, `_render_debug_physics`, `_rotate_vectors3d_by_angular_velocity`.

No prohibited construct was added: there are no new casts, volatile locals, fake uses, assembly, forced inlining, owner surrogates, foreign prototypes, declaration inventories, or compiler directives. Source edits are fully confined to the two authenticated mechanisms. The stale production comment describing 5,280 bytes and out-of-line axes validation should be updated by the integrator, as the worker already noted; that documentation cleanup was outside this independent candidate test.

Reproducible receipts: `review.py`, `review.json`, `review.stdout.txt`, `baseline.c/.obj/.gate.txt`, `candidate.c/.obj/.gate.txt`, `candidate.patch`, and both fake-scan JSONs in this directory. All file ownership is released to the parent.
