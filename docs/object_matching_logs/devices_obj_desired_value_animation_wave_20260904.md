# Devices desired-value and animation pair — 2026-09-04

## Frozen worker result and integration boundary

Final integration update: the root subsequently added the correct owner-header
declarations, removed Object Types' caller-local duplicates, and verified all
inherited exact functions in the full-tree sweep. Both functions in this
packet are retained. The frozen-worker boundary below describes the historical
handoff, not an unresolved current header blocker. Final source hash, full
verification and publication scope are in
`hud_devices_input_next_integration_batch_20260904.md`.

Starting from canonical `4795da4089557f919fd9877b3c0a3b5f1e648931`, this
scratch-gated packet advances Devices from **20 exact / 2 residual / 8 unwritten**
to **22 exact / 2 residual / 6 unwritten**. The two inherited residuals already
have parked records and remain byte/relocation-identical to this wave's baseline.
The gain is **640 padded / 623 meaningful strict code bytes**:

| New function | Padded | Meaningful | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_device_group_set_desired_value` | 288 | 275 | 10 | `fdafd716f65ff0120ec248d89cf76a09f26b7963e01846249d2a3ffa926a1b9f` |
| `_device_preprocess_node_orientations` | 352 | 348 | 11 | `1ff9691f15f6976f08c22b242c9f25a38e3ff30c33f7c72cf5af184d408ef17a` |

All 20 prior exact functions remain exact. Complete candidate code ownership
contains no owner absent from January, including no `point_from_line3d` code.
There are no new DATA/BSS/COMMON owners and no data-credit claim.

**This is a frozen, uncommitted worker packet, not yet publication approval.**
The user switched to requesting a refreshed external-worker prompt. No further
targets or header changes were attempted. Root must first finish the proper
preprocess API ownership change described below, then run the full dependent
build, stable per-function sweep, and normal admission checks.

Worker-owned dirty files are only `source/devices/devices.c` and this new ledger.
No header, config, park, compiler option, build directory, or existing Markdown
was modified. No Ninja/configure, commit, or push was run by this worker.

## Prior work and target selection

All 13 existing `devices_obj*` object ledgers were read. They establish prior
rejections for `device_frontfacing` (2026-08-21), `device_effect_new` and
`device_can_change_position` (2026-08-29), and `device_export_function_values`
(2026-08-30). Those functions were not recompiled or rewritten in this packet.
Neither selected function had a prior attempted body in that ledger set; both
also remain unwritten in the frozen Opus source at
`C:/halo-worktrees/opus-small-families-30k-20260902/source/devices/devices.c`.
The prior group-creation parks were not revisited.

January `build/split/source/devices/devices.obj` is the sole byte and ABI
authority. The read-only HCEA repository `scratch/halocea-full-audit`, commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, supplies semantic corroboration:

| Donor path | Blob |
| --- | --- |
| `src/blam/devices/device_group_set_desired_value.c` | `9ec8429d1f67d7e9aa3da0c291413b1f8581dabd` |
| `src/blam/devices/device_preprocess_node_orientations.c` | `91aedff71a80fff90477324fa213a22d286a8cc1` |
| `src/headers/animation_graph_device_animations.h` | `fc5faa8424cb5c00e73cdd8048bb93e77c652603` |

Pastudan's preprocess draft is a register-placeholder stub with an incorrect
zero-argument signature; it was rejected as implementation evidence. Marathon
`scratch/marathon-source/marathon2/platforms.c::try_and_change_platform_state`
and `set_platform_state` were consulted as ordinary source-style examples of
one-shot state transitions and named activation flags, not as Halo ABI/layout
authorities.

## Desired group value

January proves `boolean(short group_index, real desired_value)`, already
declared in `devices.h`. The body clamps finite out-of-range values to [0,1],
rejects an unassigned group, and changes the value only when it differs and the
group's change-only-once restriction allows it. It marks the changed-once flag
and visits device objects, firing the corresponding effect for members whose
power group matches.

The existing eight-byte `device_group_datum`, typed device/definition accessors,
object iterator, semantic group flag names, and `FLAG`/`TEST_FLAG` are reused.
January independently requires a definition lookup for every visited device,
before checking the power-group index. It also calls the genuine separate
`device_effect_new` helper; no HCEA-inlined effect/sound dispatch was imported.
That helper already has its owner prototype and remains unwritten.

The effect selection uses `desired_value > 0.0f` to choose repowered, otherwise
depowered. This follows January's unordered comparison behavior as well as
ordinary finite inputs; HCEA's reversed `<= 0` ternary is not substituted.

An initial parse failed because the canonical members are `tag_reference.index`
and `object_iterator.index`, not the initially assumed names. Correcting those
member names produced the first object, which was exact without a body retry.

## Node animation overlays

The function has the ordinary callback signature
`void(long device_index, struct real_orientation *node_orientations)`.
It obtains the checked device, its definition and animation graph, then the
first device-selector record. The position overlay honors the reversed-position,
looping, and noninterpolated flags. The power overlay uses the animation's frame
count times device power. Existing complete graph/animation definitions and
`overlay_animation_apply`/`overlay_animation_apply_continuous` owner declarations
come from `models/model_animation_definitions.h`.

The only new private layout is the authentic 96-byte device-selector tag record:
21 schema-unused longs followed by its animations tag block at +0x54. January
passes size 0x60 to checked `TAG_BLOCK_GET_ELEMENT`; the pinned HCEA header
independently documents that schema. Assertions also verify graph device-block
+0x30, animation-block +0x74, animation size 0xB4, and frame count +0x22.
The selector names position/power agree with the existing two-entry
`device_animation_list_entries` in `model_animation_definitions.c`.

The first ordinary candidate had equal padded extent and relocation count but
selected an integer loop/nonloop frame count before a shared real conversion.
January instead converts each conditional arm to real. Making those ordinary
frame-count conversions explicit with project type `real` closed the function.
Exactly those two formulations were compiled. No declaration-order, lifetime,
register, optimizer, helper-inlining, or mathematical-identity search was used.

## Required owner-header work before integration

Add the preprocess declaration to `source/devices/devices.h` in its Devices API
section, with one parameter per line. `struct real_orientation` is already
available through the existing math dependencies; no layout include is needed.
Then include that owner from `source/objects/object_types.c` and remove its
identical caller-local declaration (currently around line 279). The existing
object-type callback contract already agrees. Do not retain the caller-local
prototype as the sole public declaration.

At freeze, direct `devices.h` consumers are five C files: Devices, device light
fixtures, HS, Players, and Objects; plus `device_controls.h`,
`device_light_fixtures.h`, and `device_machines.h`. These nested headers add
indirect consumers. Root must validate the entire transitive blast radius,
including the new object-types include, before accepting the packet.

## Verification artifacts

```text
python tools/campaign/gate.py source/devices/devices --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/devices-desired-preprocess-real-20260904.obj
```

Direct owner-deduplicated comparisons report `lost=[]`, exactly the two new
owners above, and `extra=[]`. Both old parked candidate sections compare equal
before/after. The emitted-symbol guard passes over the complete COFF table.
`fake_match_scan.py` reports zero review leads; `git diff --check` is clean.
Full-build and test-suite results are intentionally left for root.

| Artifact | SHA-256 |
| --- | --- |
| January split Devices object | `91dfed01de745b321749480297beaa123d214b91404ada9c52c220828fc6e916` |
| `scratch/devices-next-wave-baseline-20260904.obj` | `5d6667af15e11d69738a3ebba45fa7c6ce33a2c80455b6330fc159641c706c36` |
| `scratch/devices-desired-preprocess-real-20260904.obj` | `e438a1a005a7ca887f58cec10b7f7bc611544c16a6d1da066dd8d7094549e22d` |
| Frozen `source/devices/devices.c` | `f34fb7c8ff9969f1c1d48d90cd1de6852e9c9d08d544c79981bc2da1691fbe29` |
| `xbox/bin/vc7/CL.Exe` | `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11` |

Baseline source Git blob: `d1689b5134282d22a235f207ba7191a75908e012`.
