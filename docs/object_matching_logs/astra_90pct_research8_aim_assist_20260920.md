> Research only: zero admission credit; no retained source/header/configuration change.

# Packet 8: player projectile aim / typed point setter

**CLOSED: owner prerequisite rejected; zero admitted credit.** The lane prepared one primary-supported setter proposal but tested **zero setter source shapes**. It remains uncompiled and unlanded. Root's separate real_math owner-header probe preserved the owner's 84 exact controls but regressed `_decals_delete_permanent_from_cluster`, the same historical first falsifier. Root restored the complete header and all 272 frozen consumer source/base-object hashes. See `../cone-owner/probe.json`.

## Why the owner is still missing

The current public API `game/aim_assist.h` declares `player_aim_projectile(long, real_point3d const *, real_vector3d *)`, but `aim_assist.c` has no body. This is a real missing section, not a zero-diff artifact. January `aim_assist.obj` selects `_player_aim_projectile` at file offset0x947a0, with 820 meaningful /832 padded bytes, 42 relocations, normalized SHA256 `79f83e1a1b6ec5a6a83191382cd17fdb82c6da8f216f050b4c4379a4be525ea7`. The historical map-atlas entry is fuzzy and does not authenticate a new inline owner.

The four unit logs (Jonas helper, fresh-graphs, 150K w1 and w3), the September12 reconstruction note, the fresh-graphs worker notes and the P9 header disposition were read before work. They preserve a complete 832/42 candidate, SHA256 `68607485f783d5000d94b7e70c70f1b33142754bdb5e67c904c3a128d47e5f02`, that differs from January by one argument-push placement. January's impact-direction subtraction completes its three stores before the third normalize call's push at +0x289; the candidate moves that push to +0x26e. At least15 impact-expression, local/scope, interpolation-buffer and statement-order variants already failed. They were not repeated.

The other blocker is genuine API ownership: the existing `pin_normal_to_cone3d` definition belongs to `source/math/real_math.c`, but no header declares it. January calls it out-of-line at function+0x2e4; a consumer-local prototype is inadmissible. Historical real_math.h insertion regressed three unrelated exact functions. This lane does not propose a microheader, façade, prototype-position experiment, declaration-count remedy, or metadata change.

## Frozen current baseline

Production baseline is `76dfc1fb779a082752550f795ca356d8c5f4b93f`. Source SHA256 `b1859a4b9d0e8a0c9aced333d67a5134f70860751439a0fd8bc925b4f4a610df`. `baseline.c`, `baseline.obj` and `baseline.json` freeze it. A normal-header baseline-only scratch gate produced `baseline-fresh.obj`: **14 exact,1 residual,1 unwritten of16**, point guard passes. `audit.py` verifies all27 emitted code owners and all12 non-debug noncode sections agree with the frozen build. COMMON is empty; no point helper is emitted. The other residual is `_local_player_aim_assist`.

These are baseline controls, not an uncompiled-candidate preservation claim. The setter candidate has no gate or ownership result.

## Fresh first-party evidence

`collect.py` ran actual read-only Ghidra12.1.2 for the supplied Win32 function at `0x589db0` and fresh DIA2Dump queries for HCEA `player_aim_projectile` and `pin_normal_to_cone3d`. `collection.json` records successful extraction; `ghidra_00589db0.asm/.c` and the `hcea_*_{sym,lines}.txt` files preserve raw output. The later Win32 executable is SHA256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Same-source-revision identity is **unverified**; the later builds corroborate narrow typed operations, with January remaining final byte/relocation authority.

Fresh HCEA DIA proves three public parameters and typed locals including `target_direction`, `collision_direction`, `camera_position`, `camera_direction`, `target`, `camera_displacement`, `collision`, `camera_vector`, and `desired_direction`. Target/collision share PPC offset0xd0; camera-displacement/desired-direction share0x70. Those optimized homes do not by themselves recover original braces or require the same January variable reuse. No declaration-order or scope variant was based on them.

Fresh Win32 RTC descriptor0x58a224 independently records11 named aggregate locals: parameters24, camera_position12, camera_direction12, target_direction12, collision_direction12, target56, camera_vector12, collision80, camera_to_unit12, camera_displacement12, desired_direction12. `primary.py` rereads the descriptor and verifies it against the earlier master census; scalar order/lexical scopes are not inferred from RTC.

The new concrete distinction is the camera-point update:

- At `0x58a031`, the displacement copy is normalized; at `0x58a057`, it is scaled by the camera-to-unit distance.
- `0x58a05f..0x58a098` computes camera-position x/y/z plus displacement i/j/k and builds **four arguments**: point destination, then three floats.
- `0x58a09c` calls thunk0x4070f4, whose actual E9 resolves to0x4ac810. Its raw body writes the three float arguments to destination+0/+4/+8 and returns the destination pointer.
- Thus this is a typed point-setting operation, rather than the archived candidate's `add_vectors3d` through point/vector representation casts. Fresh HCEA `set_real_point3d` symbol output authenticates `(real_point3d *p,float x,float y,float z)` and inline/was-inlined attributes; line output places it in `math/real_math.h:920–926`. The same genuine API exists in the current header at line1046.

The exact 2020 helper symbol name is **not** recovered from a matching PDB. A three-float setter's empty structural signature is not unique to point types. The typed API correspondence is supported by the authentic RTC `camera_position` destination, the observed call arguments/body and the independent HCEA API record; it is not asserted from a guessed Ghidra name. `primary.json` and the `*.raw.asm` receipts preserve those limits.

January +0x1f9..0x237 independently has the same scaled displacement and camera-point update before the128-unit trace. This makes the setter a bounded source-backed proposal. It does **not** prove the candidate would close the distant normalize push. At `0x58a0eb..0x58a10e`, the /Od build calls vector-from-points and normalize separately and immediately cleans each call; this supplies no general VC7 scheduling law.

## Prepared proposal; no compiler test

`prepare_setter.py` takes only the archived `player_aim_projectile` body from
`C:\halo-worktrees\opus5-150k-house-clean-20260914\scratch\workers\aim_assist\player_aim_projectile_candidate.c`, replaces its casted point addition with:

```c
set_real_point3d(
    &camera_position,
    camera_position.x + camera_direction.i,
    camera_position.y + camera_direction.j,
    camera_position.z + camera_direction.k);
```

It inserts that body into the frozen current source. All other archived body statements/local names remain unchanged; the current file outside the inserted function is unchanged. No foreign prototype is inserted. `typed-point-setter.c/.json/.patch` and `setter-proposal.json` preserve the exact uncompiled proposal, source SHA256 `9383fa83b4829710d6df1de2a3a5e30448a1e9f9857eda99d738a939a2766b77`.

The prior pap_v9/v12 experiments were direct component assignments, not the recovered setter-call boundary. Their archived files were checked before preparing this proposal. The one prepared setter form is not a scalar-order sweep or a manual arithmetic-helper expansion.

## Owner prerequisite and stop condition

Historical first falsifiers are recorded in
`C:\halo-worktrees\opus5-30k-fresh-graphs-20260914\scratch\h5\probe.sh` and h1/h5/h5b snapshots:

1. `effects/decals::_decals_delete_permanent_from_cluster`;
2. `rasterizer/xbox/rasterizer_xbox_draw_primitives::_rasterizer_dynamic_geometry_initialize`;
3. `tool/error_geometry::_error_geometry_polygon`.

The script also names four historical parked drifts. Root enumerated all272 current consumers and tried the single genuine prototype in its true owner header. The real_math owner preserved84 exact controls; decals preserved29/30, losing the first listed control. The probe stopped there. No setter compilation was authorized, and none occurred.

Reopen only when a separately authenticated, coherent real_math ownership/context change preserves the inherited controls, then assess the one supported setter form with the full unit and owner/data/COMMON/point gates. Do not use a consumer-local prototype, microheader, declaration shuffling, fake scope, force-inline, or another normalize spelling to evade the failed prerequisite. Even after that prerequisite, the setter's exactness remains unmeasured and must not be promised.

No source/header/config/metadata/Ninja/commit changes were made by this lane. One baseline-only C compile, zero setter candidate compiles, one uncompiled proposal, zero exact-byte credit.
