# EditorFlyingCamera owner-graph reconciliation (2026-09-08)

## Result

This packet reconciles the completed Fable editor-camera work against
canonical `7f7b72cc3`, corrects its owner graph, and admits five exact code
owners. It adds **662 meaningful / 704 padded exact code bytes**:

| January owner | meaningful | padded | verdict |
| --- | ---: | ---: | --- |
| `_translate_flying_to_orbiting` | 98 | 112 | exact |
| `_translate_orbiting_to_flying` | 93 | 96 | exact |
| `_editor_camera_set_mode` | 213 | 224 | exact |
| `_editor_camera_set_position` | 146 | 160 | exact |
| `_editor_camera_set_unit_focus` | 112 | 112 | exact |

The two translation names follow behavior and dispatch direction, not the
reversed draft labels: entering orbit mode saves the flying slot and restores
or initializes the orbiting slot; leaving orbit mode saves the orbiting slot
and derives the flying camera from the renderer.

## Runtime and data ownership

The translation unit now owns the semantic 124-byte explicit-zero
`editor_camera_globals` BSS rather than leaving an address-named unresolved
extern. The 48-byte constant section, 52-byte/9-relocation data dispatch and
124-byte BSS section compare exactly with January. The dispatch includes the
real private `editor_camera_orbiting_update` pointer; a reviewed draft with a
NULL slot was rejected because it changed data ownership and could call NULL.

The genuine `flying_camera` layout is `real_euler_angles2d facing` followed by
a separate `real roll`. The repair preserves all three exact functions in
`flying_camera.obj` and all 25 exact owners in `director.obj`, including
`director_load_camera`. Four private code symbols carry semantic names and
private metadata in `config/symbols.json`; the data and BSS anchors likewise
use semantic, non-address-derived names.

## Honest residuals

Two complete ordinary-C updates are retained and parked with **zero exact
credit**:

| owner | target/candidate padded | relocations | objdiff |
| --- | ---: | ---: | ---: |
| `_editor_camera_flying_update` | 1408/1408 | 60/60 | 99.25616% |
| `_editor_camera_orbiting_update` | 1120/1120 | 49/49 | 99.49045% |

The flying body uses component-wise position sums to preserve January's
inline schedule without emitting `point_from_line3d`. The orbiting body is
cross-build-supported and completes the player-camera, input, distance,
orientation, velocity and command-validation graph. Neither body uses source
pressure, representation casts, invented branches, or fake dependencies.

## Admission evidence

The frozen focused object reports `15 exact / 2 residual / 4 unwritten`; all
10 inherited exact functions remain exact. The five additions compare equal
including relocation identity. The full data/BSS comparisons above pass, as
do the `flying_camera` and `director` consumer gates. Changed-source fake
scanning reports no findings and the hard emitted-symbol gate finds no
`_point_from_line3d` definition. The Editor candidate also emits a 128-byte
SELECT_ANY `_valid_real_vector3d_axes2` body that January's Editor object leaves
undefined. It is byte-identical to the already-canonical fallback emitted by
earlier objects and does not alter link selection; it receives no credit and
prevents any whole-object-complete claim for this packet.

Final exact credit remains conditional on the enclosing full Ninja build,
rename-stable whole-tree sweep, park validation, admission audit and complete
test suite. The rejected `geometry.c` pair in the originating scratch packet
is not part of this admission because it introduced five surplus SELECT_ANY
math owners and an unsafe point/vector representation cast.
