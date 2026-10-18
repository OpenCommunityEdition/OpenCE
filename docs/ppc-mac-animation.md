# Native Blood Gulch skeleton and animation data

## Bounded retail decoding

`retail_skeleton.c` follows the globals multiplayer-unit reference to its model,
checking complete salted handles and tag groups rather than relying on fixed
indices. It copies model-node hierarchy, translation, quaternion, distance and
cached inverse matrix into native fields. Parent/sibling cycles, inconsistent
child ownership, out-of-range indices, unterminated names and nonfinite floats
are rejected. Geometry is still separate; this does not convert `mod2` parts
into engine model parts.

`retail_animation.c` decodes one uncompressed animation at a time from a checked
animation graph. It walks the node masks in engine order: rotation (four packed
16-bit components), translation (three float words), scale (one float word).
Default and per-frame streams are selected independently by the masks. Frame
info is converted separately. Allocated streams have native byte order and
alignment; original map bytes are never changed. All stream lengths are checked
against counts/masks before allocation and reading. Compressed animations are
explicitly unsupported. This is not full animation-graph conversion: nested
seat/weapon selection lists and sound references still need native ownership.

Retail conventions observed and preserved:

- Unused mask bits beyond node count can be set. They do not contribute bytes
  to a frame, and the engine ignores them.
- `land-soft` (animation 148) has loop frame 30 with 29 frames. The engine clamps
  the loop frame; a decoder must not reject that valid source data.
- Foot-event markers also exceed frame counts in some clips. They are comparison
  markers, not unchecked offsets into frame data.

## Observed on MacBook and both G4s

Apple GCC 4.0/10.4u SDK on the MacBook builds PPC and i386 executables. Both G4s
run the PPC binaries against their installed/staged retail Blood Gulch map.

| Data | Matching result on all three Macs |
| --- | --- |
| Multiplayer biped/model | `e2070093` / `e2080094` |
| Skeleton | 19 nodes, checksum `6cfa6498`, native-value digest `32e86515` |
| Animation records / total frames | 254 / 7292 |
| Frame-info / default / frame bytes | 40168 / 86928 / 857064 |
| Native stream value digest | `1fe96678` |
| Skeleton malformed-data checks | 28, zero failures |
| Mixed quaternion/float synthetic decoder checks | 8, zero failures |
| Private-map animation mutation checks | 29, zero failures |

Mutation checks change only a process-owned copy and restore each field. They
cover counts, types, masks/stride inconsistencies, invalid indices, compression,
lengths, address overflow and nonfinite data. They verify empty failure output,
cleanup and successful decoding after restoration. Host ASan/UBSan runs pass for
the skeleton, animation decoder and private-map mutations.

Reproduce with `sh tools/mac_rig_checks.sh <bloodgulch.map>` in the staged
MacBook source tree. Copy `skeleton_ppc`, `animation_ppc` and
`animation_mutation_ppc` from `build/mac-audit` to each G4 and run with its map
path. Evidence: `rig-build.txt`, `powerbook-rig.txt`, `ibook-rig.txt`,
`skeleton-sanitized.txt`, `animation-sanitized.txt`,
`animation-mutations-sanitized.txt` in ignored `build/mac-audit`.

These checks establish native data conversion. They do not create a biped,
advance a simulation, render an animated character or verify a playable session.

## Actual engine sampling verified

`engine_animation.c` copies decoded fields into actual `struct animation`,
`struct model` and model-node structures. Compile-time size/offset assertions
run under both native ABIs. It rejects model count/checksum mismatches before
calling the engine and validates stream sizes against masks again. Empty
streams use aligned dummy storage while retaining size zero, avoiding null
pointer arithmetic in the original engine. Synthetic all-default/all-animated
clips exercise both zero-length cases.

`tools/mac_animation_checks.sh` builds whole `model_animations.c`,
`model_animation_definitions.c`, `models.c` and `real_math.c` units with dead
stripping, existing native services and unchanged vendored musl math. Duplicate
identical layout-witness typedefs are given distinct build-local names for
GCC 4; the expressions and implementation sources are unchanged. A bounded
native tag-data accessor supports owned streams and aborts invalid access.
It must be consolidated with the full native tag registry later.

Observed on **MacBook Intel, PowerBook G4 and iBook G4** on 2026-10-15:

| Actual engine base-animation test | Result on each machine |
| --- | --- |
| Base clips sampled / other clips explicitly skipped | 174 / 80 |
| Every base frame sampled | 5868 |
| Node poses compared | 111492 |
| Checks / failures / engine errors | 899215 / 0 / 0 |
| Canonical pose digest | `04c12069` |

Every output component is compared bitwise with a separately traversed native
stream reference. This catches silent rest-pose fallback, not just nonfinite
values. Overlay/replacement application is explicitly outside this sampler;
those 80 clips decode but still need their respective pose-application paths.
No pose interpolation, root-motion integration or animation-state selection is
verified by this test.

No unresolved weak imports or system transcendental imports remain. The linked
binary still retains `floorf`, `lrintf` and `sqrtf` imports from compressed/
keyframe and other model/math branches. The tested uncompressed base path uses
integer validation, packed-quaternion multiplications and scalar copies; those
imports are not used by that path. This is not validation of the full PPC math
runtime or floating-point rounding environment. Audit/port those remaining paths
before enabling them in live gameplay.

Evidence: `engine-animation-build.txt`, `powerbook-engine-animation.txt`,
`ibook-engine-animation.txt`, `animation_imports_ppc.txt` and
`animation_imports_i386.txt` in `build/mac-audit`. Tested PPC binary SHA256:
`2234df6c8b1fb0674aaa9a5d9872a7426f988c5414a9f90e9fffc0c9aea67e28`.
Run `sh tools/mac_animation_checks.sh <bloodgulch.map>` on the MacBook; copy
`build/mac-audit/engine_animation_ppc` to each G4 and run with its map path.
Both G4 runtime tests were on Leopard; Tiger SDK targeting is not a Tiger
runtime result. The existing 22 reader/geometry unit tests also still pass.

On 2026-10-16, `aiming_screen_apply` gained a consumer-side guard for map
aiming grids: it checks the signed-short sums and available frame count before
forming grid indices, and clamps angle-derived frame coordinates before
float-to-short conversion. The generic graph decoder remains permissive, so
first-person-only looking-screen metadata is not rejected. The focused probe
checks a valid single-cell grid, a grid overflow and insufficient-frame cases,
negative counts, huge finite angles and NaN angles. Intel and both G4s report
6 checks, 0 failures; logs and the PPC binary hash are in
`build/mac-audit/aiming-guard/`.

Next: native graph node/seat/weapon selection blocks and overlay/replacement
application, then biped/collision-resistance data and actual object construction
with starting weapons. The first playable-map milestone remains open.
