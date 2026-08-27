# `render_objects.obj` cached-lighting closure disposition — 2026-08-27

## Outcome

This evidence-backed caller-closure wave corrected the earlier caller mapping
and made four of six recovered functions byte-exact on the first and only
production compile.  The result is not an admissible link-closed subset:

- the three exact file-private interpolation helpers are emitted only because
  the nonexact state-refresh function calls them; and
- the exact public lighting wrapper calls the nonexact cache allocator, which
  in turn calls the nonexact state refresh.

The entire closure was therefore rejected under the accepted-only and
link-closure rules.  `source/render/render_objects.c` and
`source/render/render.h` were restored with no textual diff.  No body was
tuned and no second code-producing candidate was compiled.

The one-shot candidate object is preserved outside tracked source as
`build/audit/render_objects_closure_first_shot_20260827.obj`, SHA-256
`53281BC3B26888CB6C96A71B23B4F54EAABCF9C6A856BC3032FDFBCC91C29F58`.
The pre-wave cumulative object is preserved as
`build/audit/render_objects_closure_baseline_20260827.obj`, SHA-256
`F36A4C1774E1E41B395386F5CE1A431DC45ED6702215BB5D1A89B21FFC368CFB`.

## Corrected topology and provenance

The previous RGB-helper disposition named `_code_0017a9e0` as the likely
state-refresh caller.  Target disassembly, translation-unit ordering, and the
Xbox HCEA reconstruction instead establish this closure:

1. `_code_0017ae60` — clamped RGB interpolation;
2. `_code_0017af00` — clamped ARGB interpolation;
3. `_code_0017afd0` — clamped normal interpolation plus normalization;
4. `_code_0017b4b0` — cached object render-state refresh;
5. `_code_0017b7d0` — cached render-state lookup/allocation/reuse; and
6. `_object_get_cached_render_lighting` — public cached-lighting wrapper.

The relevant HCEA sources are:

- `src/blam/rasterizer/interpolate_real_rgb_color.c`;
- `src/blam/rasterizer/interpolate_real_argb_color.c`;
- `src/blam/math/interpolate_normal.c`;
- `src/blam/render/object_render_state_refresh.c`;
- `src/blam/objects/object_get_cached_render_state.c`; and
- `src/blam/objects/object_get_cached_render_lighting.c`.

HCEA is cross-build semantic and provenance evidence, not the January PC byte
oracle.  Strict acceptance remained the January split object, including
padded bytes and ordered relocation identity, address, type, addend, and
ownership.

The recovered typed state layout is 0x100 bytes: datum header and pad,
`object_index` at 0x04, refresh/frame/scene stamps through 0x10,
`render_lighting` at 0x14, desired lighting at 0x88, and LOD pixels at 0xFC.
The refresh target uses 400/100-pixel LOD thresholds, 0/3/10-frame refresh
intervals, the static-light recompute flag, dynamic/static light preparation,
the exact eight-call interpolation sequence, machine type mask 0x80, and the
two January assertion expressions and source lines 0x27B and 0x2B2.

## First and only production candidate

The pinned XDK 3911 compiler first passed a no-output `/Zs` syntax check.
Exactly one normal Ninja edge then produced the candidate.  Its strict results
were:

| Function | Target meaningful / padded | Relocs | Candidate padded | Result | Target / candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_code_0017ae60` | 158 / 160 | 0 | 160 | exact | `be99b78c69d2f2b41d791415f13baa7dbf5d74f2d061695fc1d7cd0f810de4b6` |
| `_code_0017af00` | 200 / 208 | 0 | 208 | exact | `7f843e3fc6ed74394806b07c67ef76e3bf54cc29b3fd93ac546bb79e035205c7` |
| `_code_0017afd0` | 161 / 176 | 1 | 176 | exact | `2730e593f90e3ba8443d5cb1eb99416cc336352d0485643897f31906cd303365` |
| `_code_0017b4b0` | 786 / 800 | 38 | 816 | rejected | `3ab7188f61980610a00bda7923133d2649b40e8d273a03b20f6987e00b3d9f91` / `070edbee61ecc221fb86be84959f60f6bc7ff4c06e76cf00b28724a239297c1a` |
| `_code_0017b7d0` | 291 / 304 | 16 | 288 | rejected | `f04152918053aabe11b85642c1092a0de3e581da80dd922bde60b30dd3012ffc` / `a1ea3436de3ed364d65df71c325f4bce6b667b91bffa5ed218ab712d6e57f3bf` |
| `_object_get_cached_render_lighting` | 80 / 80 | 8 | 80 | exact, not link-closed | `f8986339ebe9831741b4fb3cd2c12516a38042c334410cdc62a9a4974bd22c82` |

The normal interpolator's sole relocation is an exact `_normalize3d` REL32 at
offset 148.  Both rejected callers had the correct relocation count, but
their code sizes and relocation addresses diverged broadly; this is a caller
topology/code-generation miss, not a local literal or declaration correction.
The candidate semantic report temporarily rose from 4,255 to 4,259 accepted
exact functions, exactly accounting for the four strict first-shot matches.

All four inherited exact functions remained unchanged:

- `_render_objects_initialize` — 64 bytes, 7 relocations;
- `_render_objects_initialize_for_new_map` — 16 bytes, 2 relocations;
- `_render_objects_dispose_from_old_map` — 32 bytes, 2 relocations; and
- `_render_objects_dispose` — 16 bytes, 1 relocation.

No external-linkage spelling, synthetic caller, anchor, pragma, volatile or
register qualifier, barrier, intrinsic, raw address, pointer/integer trick,
undefined behavior, compiler substitution, object patch, comparator exception,
or admission exception was used.

## Reopen condition

Do not repeat this six-function source candidate.  The three interpolation
bodies and wrapper are already proven exact.  Reopen only with genuinely new
evidence that resolves the two state-management callers, especially original
January-era source/PDB/map evidence or an independently exact source-shape
donor for `_code_0017b4b0` or `_code_0017b7d0`.  Admission then requires the
entire reachable closure to compare exact in one ordinary build.
