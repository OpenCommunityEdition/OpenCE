# Batch 8 new-exact house-rule audit (2026-09-13)

## Scope

This audit compares `scratch/batch8-current-preaudit.json` with
`scratch/batch8-base.json`.  The stable ledger reports 31 newly exact sections,
totalling 7,408 padded bytes and 7,188 meaningful code bytes.  The review is a
source-admission audit; it does not treat byte equality alone as proof of
authentic source.

## Exact-body result

No newly exact body requires rejection for fake matching, uninitialized reads,
representation punning, compiler barriers, `volatile`/`register` steering,
forced inline/noinline annotations, address-derived source identifiers, or
nonsensical logic.  The exact bodies are ordinary typed C and the targeted
fake-match scan reports zero review leads.

| owner group | functions | padded | meaningful | disposition |
| --- | ---: | ---: | ---: | --- |
| Actor firing positions | 5 | 928 | 898 | accept |
| Actor looking | 1 | 80 | 77 | accept |
| AI script | 1 | 272 | 269 | accept |
| Encounters | 1 | 144 | 132 | accept |
| Bitmaps | 1 | 128 | 115 | accept |
| Effects | 1 | 128 | 128 | accept |
| Particle systems | 1 | 160 | 155 | withhold until prototype ownership is repaired |
| Weapon HUD | 4 | 464 | 420 | accept |
| Collisions | 1 | 544 | 533 | accept |
| Physics | 2 | 992 | 972 | accept |
| Xbox environment fog, including stock XDK wrappers | 5 | 1,152 | 1,109 | accept bodies; caller cleanup required |
| Render cameras | 3 | 880 | 863 | accept |
| Game sound | 1 | 240 | 236 | accept |
| Structures | 2 | 592 | 585 | accept |
| Bipeds | 2 | 704 | 696 | accept |
| **Total** | **31** | **7,408** | **7,188** | **30 immediately admissible; one owner hold** |

Thus the current publication split is 30 provisionally accepted functions,
7,248 padded / 7,033 meaningful bytes; one function is withheld, 160 padded /
155 meaningful bytes.  This is an ownership hold, not a source-authenticity
rejection.  After repairing the owner and rerunning the full sweep, all 31 may
be credited if exactness remains.  Hard source-authenticity rejections are
zero bytes.

## Independent semantic evidence

The HCEA reconstruction under `halocea-review/src/blam/` independently
corroborates the otherwise surprising exact shapes, notably:

- `interface/play_weapon_hud_sounds.c`: the apparently dead typed unit lookup
  and unused `weapon_state` really are the later-build function shape; this is
  not a fabricated relocation dependency.
- `ai/encounter_post_combat_add_possibility.c`: the no-break duplicate insert
  is an independently documented original bug, not coincidental matching.
- `structures/structure_clusters_in_sphere_recursive.c`: the flood-fill counts
  every visited cluster after output capacity is exhausted and advances its
  dormant output pointer accordingly.  Forming a pointer beyond the output
  object is formally undefined in standard C, but it is never dereferenced
  once the remaining count is non-positive.  January equality plus the
  independent source map makes this a preserved original bug, not a
  source-shape spell or an uninitialized-read trick.
- Physics, bitmap, biped, camera, actor-firing-position, particle-system and
  HUD helpers each have corresponding semantic files.  January remains the
  ABI, diagnostics and byte authority where the later build differs.

The `render_cameras.obj` `/Ob1` setting is admissible: it is narrowly scoped to
the translation unit and restores January's real out-of-line private helper
boundary while preserving the intentional math-inline schedule.  It is not a
source annotation or forced noinline trick.

## Required cleanup before publication

1. `particle_systems_render` is public but is declared in
   `source/render/render.c`, a consumer.  Move its declaration to
   `source/effects/particle_systems.h`, remove the consumer-local declaration,
   audit all header includers, and rerun the stable sweep.  Until then its 155
   meaningful bytes are withheld under the prototype-owner rule.
2. The complete but fuzzy
   `_rasterizer_environment_fog_screen_begin` caller introduces a local
   declaration for `rasterizer_water_get_visibility_for_window`.  Put that
   declaration in its existing owner,
   `source/rasterizer/xbox/rasterizer_xbox_water.h`, include that header from
   the fog owner, and remove the foreign local declaration.
3. In the same fuzzy caller, replace the discarded raw `tag_get` with the
   bitmap subsystem accessor.  Also verify the first-time
   `previous_camera_matrix` copy: it currently copies `sizeof(real_matrix4x3)`
   bytes beginning at `global_window_parameters.camera_position`, whereas the
   later update copies `global_window_parameters.camera_matrix`.  The former
   crosses unrelated fields in the current typed layout and must not be
   presented as settled semantic source without disassembly evidence or a
   correction.
4. Prefer `TEST_FLAG(_object_mask_vehicle, object->object.type)` over the two
   remaining `FLAG(type) & _object_mask_vehicle` expressions in
   `collisions.c` if the macro expansion preserves the exact sections.

These cleanup items do not justify retroactively rejecting the independently
exact helper bodies, but the full batch is not house-rule publishable until
they are resolved and the ordinary full build/stable regression sweep passes.

## Checks

- Stable delta: 31 gains, 7,408 padded bytes; one unrelated regression is
  separately reported for `rasterizer_dynamic_geometry_initialize` and must
  be resolved by the orchestrator.
- Meaningful sizes: read from `build/semantic_report.json`, total 7,188 bytes.
- `tools/fake_match_scan.py` over all 15 owning translation units: zero review
  leads.
- Existing packet gates document absence of an emitted `point_from_line3d`
  COMDAT for the affected collision, fog, particle, physics and camera work;
  the orchestrator must repeat the guard after the owner cleanups.

## Final orchestrator resolution

All four publication holds above were resolved before the batch-8 freeze:

- `particle_systems_render` now has its declaration in
  `effects/particle_systems.h`; the consumer-local declaration is gone and the
  function remains strict exact.
- The water-visibility declaration now belongs to
  `rasterizer_xbox_water.h`, and the fog owner consumes that header.
- The fog caller uses the typed bitmap accessor and the matrix copy was checked
  against January's field/relocation topology; no raw layout surrogate remains.
- Collision vehicle tests use the established flag macro with the authentic
  object mask.

The dynamic-geometry regression was repaired with the complete authentic enum
instead of preserving an opaque numeric or partial definition.  The final
stable census is 41 gains / 10,192 padded bytes / zero regressions; all 31
functions reviewed above are among the admitted strict gains.  The additional
strict packets and the separately audited 3-wide UI semantic-exact function
are summarized in the aggregate batch-8 ledger.
