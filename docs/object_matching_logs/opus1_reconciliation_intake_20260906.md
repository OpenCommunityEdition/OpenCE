# Opus 1 rasterizer/widgets reconciliation against canonical `d574f4ff`

Date: 2026-09-06. The initial inventory was read-only; subsequent integration
and source-admission findings are recorded below. Donor trees remain untouched.

## Verdict

The lane is real and its headline progress survives comparison with the current
canonical baseline: the 14 affected target units gain **30,074 meaningful code
bytes, 124 credited functions, and 128 strict owners / 34,496 padded bytes**.
Canonical has no overlapping strict gain in these units. However, the donor tip
is not safe to merge wholesale:

- the donor branch lacks 54 strict owners (13,952 padded bytes) added elsewhere
  on canonical after the common base;
- nine modified TUs newly emit 35 helper COMDAT owners (1,616 padded bytes) that
  are absent from their corresponding January target objects;
- the final handoff's per-object padded-byte table has two arithmetic errors;
- two zero-gain source files and several genuine-but-residual bodies should not
  be imported as if they were strict closures.

Use function/unit-scoped integration batches with a frozen current-baseline
whole-tree sweep. A broad Git merge cannot substitute for source/semantic
admission and linewise config reconciliation; do not replace canonical files or
configs wholesale with donor versions.

### Subsequent verified integration

The [Devices four-function packet](devices_obj_opus1_four_function_reconciliation_20260906.md)
admits **1,242 meaningful bytes / four functions** with zero regressions,
after rejecting the unsupported flag-width fifth gain. This leaves **28,832
of the lane's originally measured 30,074 meaningful bytes not admitted** by
this packet, including source/ownership-blocked work; it is not a promise
that all those bytes will pass admission. Canonical reaches 922,106 meaningful
exact bytes, 6,234 credited functions and unchanged 391/833 Matching objects.
Frame Statistics and naming findings below refine the initial proposed order.

## Identity and frozen evidence

- Canonical comparison commit: `d574f4ff458c71ae4011b7796d80e3e67dfed55f`.
- Donor: `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, branch
  `opus/30k-rasterizer-widgets-20260905`, tip
  `bd8c008e85a1543877d0843bab3f1c2dffab9a13`.
- Tracked donor state is clean; only `scratch/` is untracked. The last source and
  config change is `bc8a7f78526b5cd8ca7f13172dc284f827464db3`;
  `bd8c008e...` changes only the final handoff document.
- Merge base: `e98e3ff62852c1031f9ff74101eb0396136b42d9`.
- Donor baseline `scratch/BASE-e98e3ff62.json`:
  `3606652db05f85f152e84bbf76533e67d87108a206db9213f3121c6de4efefd9`
  (8,245 functions / 6,222 strict).
- Donor final strict snapshot is **`scratch/FINAL11.json`**, not `FINAL10.json`:
  `e3cef934320d44ee34735c9fd6241b941fde6cf05e88b0afdfc80c6719e201cc`.
- Donor final park audit is **`scratch/W13-parks.json`**, not
  `FINAL-parks.json`: `d8d97398af50ff2ec9833cd5d25091d82ca80dcf12c53918ad12f1fc530c78a9`
  (274 active / 0 stale / 0 invalid). `FINAL-parks.json` is the superseded
  wave-12 result (277 active).
- Donor admission result `scratch/FINAL-admission.json`:
  `5f5e663c561c927412308e0e86563946e17c7329e45f40af9af1fe499f42ab83`
  (unchanged from baseline; no new whole-object admission).
- Frozen canonical comparator:
  `scratch/server-source-packet-final-stable-20260906.json`, SHA-256
  `1c171e8c2f3c7a80e3bee8e5511b2f2abc6b04e1f8f59a0e7c4381ac8db5347c`.
  Its frozen report is SHA-256
  `8ea68c86dad7cf4e9a9d2445297f90a500e5f9c496017b087e0380b536941631`.
- Donor final `build/report.json` SHA-256:
  `974134a36317677c8d2489476b08462eac7f997c098e4134170181c4cd46bd49`.

All four lane handoffs were read completely. Final handoff SHA-256 is
`1022b87ced2b4742f9f1dd4cef11c00e480071389f3a0bec187b14b047a124c4`;
wave 1-2 `7fc2a5b653442f1a78cafe65d2660a96a618b609f8f91c866b8f5fdb7473ac4e`;
wave 3-4 `5c426be70906320926919b9a6a3302526026472cb27b917889366335e73d4bcf`;
wave 5 `9e74e2164c9c2b55e806bbedc04febcb7d6dd49f428cadffaa67c454293d4e94`.

## Exact tracked donor delta

Configuration:

- `config/symbols.json`: 82 target records renamed; 19 carry `static:true`.
  None of these 82 renames is already present at the same offset in canonical
  `d574f4ff`.
- `config/parked.json`: nine exact-function parks removed (three Devices, six
  `rasterizer_xbox`).

Source (13 files):

1. `source/devices/devices.c`
2. `source/objects/widgets/flags.c`
3. `source/objects/widgets/glow.c`
4. `source/rasterizer/rasterizer_frame_statistics.c`
5. `source/rasterizer/rasterizer_lights.c`
6. `source/rasterizer/rasterizer_text.c`
7. `source/rasterizer/xbox/rasterizer_xbox.c`
8. `source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c`
9. `source/rasterizer/xbox/rasterizer_xbox_environment.c`
10. `source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`
11. `source/rasterizer/xbox/rasterizer_xbox_models.c`
12. `source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`
13. `source/rasterizer/xbox/rasterizer_xbox_widgets.c`

Docs (five): the four Opus handoffs above and
`rasterizer_xbox_screen_effect_and_xbox_naming_gap_attestation_20260905.md`.
No header is changed by the donor lane.

## Gains against current canonical, per unit

The meaningful/credited columns are direct differences between the frozen
canonical and donor final `build/report.json` unit records. The strict columns
come from the stable target-section diff, not from the lane summary.

| Unit | meaningful | credited fn | strict fn | strict padded |
| --- | ---: | ---: | ---: | ---: |
| devices | +1,956 | +6 | +7 | +2,496 |
| widgets/flags | 0 | 0 | 0 | 0 |
| widgets/glow | +4,200 | +14 | +14 | +4,288 |
| rasterizer_frame_statistics | +4,165 | +1 | +1 | +4,176 |
| rasterizer_lights | +722 | +3 | +4 | +1,344 |
| rasterizer_text | 0 | 0 | 0 | 0 |
| rasterizer_xbox | +2,079 | +42 | +43 | **+3,200** |
| rasterizer_xbox_active_camouflage | +2,084 | +10 | +10 | +2,160 |
| rasterizer_xbox_environment | +4,185 | +7 | +8 | +5,872 |
| rasterizer_xbox_environment_fog | +2,615 | +9 | +9 | +2,672 |
| rasterizer_xbox_models | +2,132 | +6 | +6 | +2,176 |
| rasterizer_xbox_screen_effect (config only) | +1,317 | +10 | +10 | +1,392 |
| rasterizer_xbox_transparent_geometry | +1,957 | +9 | +9 | **+2,016** |
| rasterizer_xbox_widgets | +2,662 | +7 | +7 | +2,704 |
| **total** | **+30,074** | **+124** | **+128** | **+34,496** |

The final handoff table says `rasterizer_xbox +1,904` and transparent geometry
`+2,032`; the stable snapshots prove the corrected figures above. The headline
34,496 total is correct.

The unscoped stable diff also reports 54 “regressions” / 13,952 padded bytes.
Every one is post-`e98e` canonical work outside this Opus scope (AI, Path,
Bitmaps, Particle Systems, HUD, Weapons, Geometry, Network Server, RenderDebug,
ErrorGeometry). That expected branch divergence proves that replacing/resetting
canonical to the donor would lose work; it does not claim that Git's ordinary
three-way merge necessarily drops the disjoint commits. It still rules out
using an unreviewed broad merge as a substitute for scoped admission.

## Newly introduced target-absent code owners

This is the main omitted house-rule boundary. Each name below is newly emitted
relative to the frozen canonical object and absent from the corresponding
January target object. Every section is an external COMDAT (`storage 2`, section
flags `0x60501020`). Stable-verdict accounting ignores such candidate-only
sections, so “zero regressions” does not clear them.

| Donor object | new target-absent owners (padded bytes) |
| --- | --- |
| devices | `point_from_line3d` 48 |
| widgets/flags | `cross_product3d` 64 |
| widgets/glow | `cross_product3d` 64; `distance3d` 64; `magnitude3d` 48; `magnitude_squared3d` 48; `normalize3d` 96; `real_local_random_range` 32; `scale_vector3d` 48; `square_root` 16; `vector_from_points3d` 48 |
| rasterizer_lights | `cross_product3d` 64; `point_from_line3d` 48; `subtract_vectors3d` 48 |
| rasterizer_xbox_environment | `point_from_line3d` 48 |
| rasterizer_xbox_environment_fog | `dot_product3d` 32; `fast_ftol` 32; `magnitude2d` 32; `magnitude_squared2d` 32; `normalize2d` 80; `real_alpha_to_pixel32` 112; `real_local_random` 16; `real_local_random_range` 32; `scale_vector2d` 32; `square_root` 16; `vector_from_points3d` 48 |
| rasterizer_xbox_models | `dot_product3d` 32; `plane3d_distance_to_point` 48; `point_from_line3d` 48; `subtract_vectors3d` 48; `vector_from_points3d` 48 |
| rasterizer_xbox_transparent_geometry | `dot_product3d` 32; `fast_ftol` 32; `plane3d_distance_to_point` 48 |
| rasterizer_xbox_widgets | `fast_ftol` 32 |

Total: **35 target-absent sections / 1,616 padded bytes**. Frame Statistics,
`rasterizer_xbox.c`, Active Camouflage, and Rasterizer Text add no such owner.
Some affected helpers have legitimate owners elsewhere, but that does not prove
January emitted them in these TUs or settle mixed COMDAT selection/link order.

The Devices case is already explicitly rejected in canonical documentation:
`devices_obj_debug_renderer_inline_owner_boundary_20260904.md` proves
`device_render_debug` itself exact at 432 bytes / 27 relocations but rejects the
extra 48-byte `point_from_line3d`. Donor final reproduces exactly that forbidden
owner. It also adds a TU-local `extern boolean debug_objects_devices`, while
`hs_globals_external.c` declares the same linker-common byte as
`extern byte debug_objects_devices[]`; an owning declaration remains unresolved.

## Additional Xbox source/ownership findings

- `_SetupSmartStates` is external (`storage 2`) in the current January split but the donor forward
  declaration and definition are `static` (`storage 3`); its donor symbol record
  does not say `static:true`. Stable-verdict equality does not validate this
  owner-storage mismatch, so its 128-byte byte match is not yet admissible.
  Split storage can reflect default csplit metadata: establish original linkage
  independently, then repair source or split/config ownership as warranted.
- `__rasterizer_window_begin` closes only after the donor changes
  `suppress_window_begin_end` and `previous_window_index` to file-local statics.
  The current split/config exposes both as external members of the large BSS
  owner. This is an unresolved linkage disagreement, not by itself proof that
  the source statics are historically false. Establish original ownership before
  accepting the change; the object also retains a 2,230-byte data gap.
- Environment Fog synthesizes a 664-byte aggregate and aliases former globals
  onto its fields; January's BSS span is 661 bytes. Exact callers do not make
  that aggregate an authentic data owner.
- Active Camouflage declares `group_draw__internal` non-const while Transparent
  Geometry defines it const; Transparent declares `active_camouflage_draw`
  const while Active defines it non-const. Model/group/skinning parameter types
  are also separately redefined across TUs while the headers only forward
  declare them. Pointer ABI happens to agree, but type ownership does not.
- Environment adds 14 pad/unknown arrays; across the Xbox donor files there are
  94 pad/unknown arrays and 71 `offsetof` assertions. These are not raw pointer
  arithmetic, but the locally reconstructed facades still need January-backed
  owning-type review before wholesale source admission.
- Some added BeginScene/EndScene/Unlock statements call empty low-level XDK
  force-inlines: the containing Halo owner remains unchanged while a wrapper
  COMDAT materializes. Treat those as wrapper-source attribution requiring
  caller evidence, not automatically as a genuine source dependency.

## Proposed integration batches

1. **Devices four-function no-new-owner packet (reviewed down from five).** Integrate
   `device_can_change_position` (128), `device_effect_new` (176),
   `device_update` (736), and static `create_initial_device_groups` (224):
   1,264 strict padded / 1,242 meaningful bytes. All three
   external functions already have their matching contracts in `devices.h`.
   Retain `word flags` in `device_group_new` and the scenario-assignment park;
   remove only the two accepted closure parks after verification. The donor's
   claim that four-byte argument pushes prove the original source type is too
   strong: a promoted word also occupies a four-byte x86 stack slot.
   The target allocator reads/stores only a word; its two ordinary calls pass
   4 or 5. Independent HCEA and PC reconstructions also use 16-bit flags.
   Reject the unsupported width change even though it closes 288 padded bytes.
   See [the scoped Devices admission](devices_obj_opus1_four_function_reconciliation_20260906.md).
   No `symbols.json` change.
   `device_export_function_values` is independently strict (512) and introduces
   no surplus code, but its only declaration is caller-local in
   `object_types.c`; admit it separately with a genuine owning `devices.h`
   prototype/removal of the caller-local declaration and a header-consumer
   audit. Omit the render-debug include, debug flag extern, renderer body, and
   `point_from_line3d` owner until both the math owner and public/global
   declaration ownership are resolved.

2. **Pure naming attestation packet.** Review and apply the 29-record
   `fde5bd48e...` packet linewise (19 `rasterizer_xbox`, 10 screen-effect). It
   changes no source and accounts for existing byte-identical XDK/helper
   sections. Do not replace canonical `symbols.json`; retain per-offset evidence
   from the naming-gap ledger.
   Independent follow-up narrows the immediately supported scope to 26 XDK
   wrappers (22 named-callee wrappers and four SDK dispatch expansions).
   Their source owners are private under `D3DINLINE`, not external: all require
   independently justified `static:true` split metadata, rather than just the
   one static record in the donor patch. Hold the two callless cube-texture
   Unlock names and the uncorroborated `vector2d_reciprocal` source identifier.
   No naming metadata is changed in the Devices packet.
3. **Frame Statistics source-admission hold (updated audit).** Although the
   donor's 4,176-byte draw owner adds no target-absent section, the isolated
   draw-only candidate is residual under current canonical flags. Exactness
   depends on retaining the previously rejected `get_fps` predecessor. The
   donor's experiment history also contains declaration-order permutation
   searches. Do not import the exact donor version or use its byte match as
   evidence authenticating that compiler-state dependency. Preserve the natural
   draw-only reconstruction as research with zero exact credit until independently
   source-authentic integration and whole-owner verification are available.
4. **Bounded `rasterizer_xbox.c` body packet.** Start with
   `rasterizer_set_texture` (496), `rasterizer_set_model_skinning` (320),
   `rasterizer_set_frustum_z` (320), and `rasterizer_set_stencil_mode` (784).
   Hold `_SetupSmartStates` and `__rasterizer_window_begin` for the storage/BSS
   owner corrections above. Audit wrapper call-site provenance separately; do
   not present the current 95/95 byte count as owner-complete or as whole-object
   Matching.
5. **Widget begin/sprite pair.** `__rasterizer_widget_begin` (1,152) and
   `__rasterizer_widget_draw_sprite2d` (528) are the cleanest Xbox source pair,
   with declarations in `rasterizer_widgets.h` and no new pad facade. Three XDK
   wrappers (992) accompany the pair; the two 16-byte visibility wrappers belong
   only to the still-residual 640-byte occlusion-submit body and must not be
   attributed to this pair.
6. **Active Camouflage cache only.** The 832-byte cache body is standalone. The
   112-byte `real_alpha_to_pixel32` section and eight wrappers are retained by
   the still-fuzzy 2,016-byte draw body, so do not claim/import them standalone.
7. **Owner-blocked remainder.** Glow, Lights, Environment, Fog, Models,
   Transparent Geometry, the remaining Widget work, and Flags need either exact-body isolation
   that stops the listed emissions or a genuine shared-helper/COMDAT ownership
   reconciliation. Do not import their full donor files merely because selected
   target sections are exact. Omit zero-gain Flags and Rasterizer Text outright.

The donor's large fuzzy `group_draw`, `model_draw`, lightmap and transparent
submit bodies remain blocked by January's per-caller `real_math.h` inline
schedule and shader assert-name layout. No broad `static __inline` header change:
the donor measured that variant losing 119 functions across 21 objects.

## Reproduction (read-only)

```text
git -C C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905 diff --name-status e98e3ff62852c1031f9ff74101eb0396136b42d9..bd8c008e85a1543877d0843bab3f1c2dffab9a13
python -B -m tools.campaign.stable_verdicts diff scratch/server-source-packet-final-stable-20260906.json C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905/scratch/FINAL11.json
```

The second command reports the 128 Opus gains plus 54 reverse losses from
unrelated canonical work; filter/report by the units listed above rather than
using the unscoped net total.
