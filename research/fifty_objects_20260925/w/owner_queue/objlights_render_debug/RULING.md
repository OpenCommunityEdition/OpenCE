# RULING - owner_queue/objlights_render_debug (base cdc8ebd3)

Two held objects. Every side effect of each landing is listed (lesson of ruling 20260921 #2b).

---------------------------------------------------------------------------------------------------------------

## Q1 - object_lights: admit the per-TU `__FILE__` surplus COMDAT?

**Question (yes/no).** Admit `object_lights`' surplus `_object_get_bounding_sphere` COMDAT as-is? It matches
January's selected copy (action_vehicle) in size (144), normalized bytes, all 9 relocation sites/types and 7 of 9
targets. Only its two `__FILE__` DIR32 targets differ. They name `"c:\halo\source\objects\objects.h"`, which is
January object_lights' own 33-byte .rdata literal. January's selected copy names `"..\objects\objects.h"`.
Rule 6 as written ("byte-identical to January's selected provider") cannot be met by object_lights in ANY source:
- object_audit requires that January-owned literal; and
- VC7 emits the literal only when this COMDAT references it.

**YES** -> apply 01, 02, 03_YES, 04_YES. object_lights becomes Matching (+1 object, +1,541 data).
**NO** -> 01 + 02 can still land (zero-credit storage cleanup), and optionally 03_NO_ALT (+1,541 data under
`allow_incomplete_unit`). object_lights stays held.

**Side effects of YES (complete list).**
- No new COMDAT emission. Production already emits the same c:\ copy (the OBJECTS_H_FILE packet b807f9f6 landed).
- 4 functions become `static` (storage only; all 43 rows stay exact):
  - should_render_lights
  - light_attenuation
  - cluster_get_first_light
  - cluster_get_next_light
- The two `if (lights_game_globals->render_lights && game_engine_allow_dynamic_lighting())` tests become the
  /Od-attested `should_render_lights()` calls. January emits the non-public static only because it is used.
- `object_lights_rendering.h` loses the `should_render_lights` prototype. Its two other includers, render.c and
  render_objects.c, stay objcmp-IDENTICAL.
- config/symbols.json gets 4 in-place `"static": true` edits. csplit then changes only object_lights.obj.
- One semantic_data_matches entry (`_lights_section`, the '$'-literal scorer defect) and a config.json status flip.

**First-party evidence FOR.**
1. The January string census names a single owner for each spelling:
   - The c:\ literal is defined ONLY in object_lights, and no relocation anywhere references it.
   - The ..\ literal is defined by action_vehicle and imported by render_objects.
   - `_object_get_bounding_sphere` is defined by action_vehicle and is an UNDEF import in the other 8 users,
     including object_lights.
2. NEW census over all 833 split objects (9,420 `??_C@` literals): exactly ONE January string has no referrer. It
   is this c:\ objects.h literal. Its only possible referrer is object_lights' own discarded COMDAT copy.
3. The later first-party /Od build asserts in object_get_bounding_sphere with the plain header `__FILE__`
   (`D:\P4\Halo1\source\objects/objects.h`), which shows the inline used `assert()`/`__FILE__`.
4. The board literal census (w/bounding_sphere F1) shows how January spelled header paths:
   - a header in the TU's own directory got the absolute spelling;
   - a header from another directory got `..\`.
   objects.h is the only header with both spellings, and object_lights is the only objects/ TU that uses an
   asserting objects.h inline.
5. Link order is consistent with the selection. action_vehicle code is at image offset ~0x9100 (module 461) and
   object_lights at 0x128730 (module 152). The image runs in descending module order, so a pick-first link keeps
   action_vehicle's ..\ copy, which is what January selected.
6. Control P2: forcing one spelling (dropping object_lights' OBJECTS_H_FILE) makes the audit report the January
   33-byte .rdata MISSING, drops .rdata to 96.93% and costs -1,100 data credit.

**AGAINST / risks.**
- The rule-6 text is not literally met. This is a new exception class: an "authentic non-identical surplus
  COMDAT, proven by an orphan January literal".
- A whole-program relink that put object_lights before action_vehicle would select the c:\ copy. Its asserts would
  then print the other path. That cannot happen with January's link order and changes no January-owned section.

**Disclosure comments as they would land.** None new. The per-TU comment already landed in b807f9f6:
- object_lights.c: `/* objects.h lies in this unit's own directory, so January's __FILE__ for it was the absolute
  path (see OBJECTS_H_FILE in objects.h) */`
- objects.h: the OBJECTS_H_FILE block comment.

The 03_YES reason text records the ruling ("admitted by owner ruling on the per-TU __FILE__ surplus copy").

**House-rule classes touched:**
- rule 6 (surplus COMDAT identity): the exception class asked here;
- storage (static per cachebeta publics);
- prototypes in the genuine owner: a static's prototype moves from the public header into the .c prototype block;
- semantic data entry ('$'-literal defect, single-section);
- optional (e) COMMON tentative definitions, patch 07 (see Q2e).

---------------------------------------------------------------------------------------------------------------

## Q2d - render_debug: the invented `render_debug_globals_definition` aggregate

**Question (choose one).** How should render_debug's 29,708-byte cache state be spelled?
- **D1**: land seven separate zero-initialised file statics in the aggregate's member order, with descriptive
  names (no first-party names exist):
  - `render_debug_cache_strings`
  - `render_debug_cache_entries`
  - `render_debug_cache_game_time`
  - `render_debug_cache_entry_count`
  - `render_debug_cache_string_offset`
  - `render_debug_cache_entry_overflow_reported`
  - `render_debug_cache_string_overflow_reported`

  This also needs a symbols.json rename (`_render_debug_globals` becomes the first static) plus 6 new static rows.
- **D0**: admit the existing aggregate, with its two opaque pad arrays, as it is.
- **HOLD**.

Either D0 or D1 makes render_debug complete once 05 (+ optional 06) and 08 land (+1 object).

**Evidence.**
- January's own layout is short / 2-byte gap / short / 2-byte gap / short / bool / bool, at 0x7400..0x740B.
  - An aggregate needs the pad members `opaque_after_game_time[2]` and `opaque_after_entry_count[2]`. That is the
    law (j) signature of an invented aggregate.
  - Separate zero-initialised statics give exactly this layout, with no pads.
- The D1 lab (lab/, never lands) builds the whole TU with the seven statics and a lab split:
  - 36/36, and object_audit PASS (69 January symbols, 0 differ);
  - mini objdiff identical to production.
- Strip test of the initialisers: uninitialised statics land in name-hash order and FAIL the audit. The `= 0`
  initialisers are load-bearing through VC7's declaration-order .bss law. This is the same proof accepted for the
  periodic_functions statics (w/data_gap P2).
- The later /Od build lays the cache variables out as separate globals (0xf0afb4, b8, bc, c0). Two hs toggles,
  debug_bsp (b6) and debug_input (b7), sit INSIDE that span, so it cannot be one struct.
- The names are not recoverable. None of these sources has them:
  - cachebeta publics (the statics are non-public);
  - HCEX.pdb (it has no render_debug data);
  - the 2001 linker maps (they list no render_debug data rows);
  - the Sept-2001 map and atlas (functions only).

**AGAINST / risks.**
- D1 introduces 7 non-first-party names. Loose statics with new descriptive .bss names are an existing held class
  (rasterizer_xbox_profile, hs_scenario_definitions).
- D1's zero initialisers are layout-evidenced, not text-attested.
- D0 keeps an aggregate that January's layout and the /Od build both contradict.

**Disclosure comment for D1, exactly as it would land** (it is in lab/D1_OPTION_render_debug_separate_statics.patch):

    /* January's names for the render cache state are lost (no first-party symbol, map or PDB row), so
       these are descriptive. They are separate file statics, as in January's .bss and the later /Od
       build; the zero initialisers give VC7's declaration-order layout (strings, entries, game_time,
       entry_count, string_offset and the two overflow flags at 0x740A/0x740B) */

For D0, the integrator should add a note that the aggregate and its pad members are a layout convenience, not
January's structure.

**House-rule classes:**
- rule 8 (name globals; descriptive names in D1);
- law (j) invented aggregate (D0);
- no-fake-matching (load-bearing initialisers, layout-proven).

## Q2e - render_debug's six consumer-local `extern boolean debug_*` (is there a definer?)

**Answer: yes. January's definer is render_debug.c itself**, as tentative definitions that the linker pooled into
COMMON. This needs no new policy ruling, only integrator confirmation of patch 06, because the evidence is
per-symbol and not pool adjacency alone.

The evidence:
1. January references the six (linker_common records 50..55, 1 byte each) only from render_debug.obj and
   hs_globals_external.obj.
2. hs_globals_external is excluded as a definer:
   - it references 81 pool records and is the position-admissible owner of none of them;
   - the pool law holds on 102 of 105 single-referencer records.
   The six sit between render_objects' cluster and render's `_render`. Among the referencers, only render_debug
   (module 87) falls in that range.
3. The 2001 Bungie linker maps show that the six exist exactly when render_debug.obj is linked:
   - 2001-08-15 beta: 5 toggles, and render_debug.obj has no render_debug_structure_decals.
   - 2001-09-25 beta: `_debug_permanent_decals` appears together with its only user,
     `_render_debug_structure_decals`, in render_debug.obj.
   - 2001-09-25 retail cache.exe: render_debug.obj is not linked and all six toggles are absent. The neighbouring
     render_objects and render records, and other hs toggles, are still present.
4. In the later first-party /Od build, the six are INTERLEAVED byte-by-byte with render_debug's own cache variables
   at 0xf0afb0..0xf0afc0. Only one object's .bss can do that.

**Form.** Bare tentative definitions (`boolean debug_bsp;` ...) in render_debug.c. This is exactly the objects.c
precedent (`boolean debug_objects_physics;` etc., Matching) and the batch-4b players COMMON landing.
- Byte-inert: objcmp IDENTICAL, and the six symbols become COMMON(1).
- Links clean with January's linker_common and with hs_globals_external in both orders, and resolves the six
  (production leaves them unresolved).

**Scope note.** The same class is in object_lights (5 externs); patch 07 is the same form with the same kinds of
evidence:
- pool cluster 108..113 around its own tentative `lights_globals`;
- /Od: debug_object_lights and debug_lights sit between object_lights-only addresses, next to lights_game_globals.

Neither is a blocker by this lane's own precedent. render_objects was admitted with `extern boolean
debug_objects;` / `extern short debug_rasterizer_light_count;`, and scenario is Matching with
`extern boolean debug_sound_environment;`. So 06/07 are optional hygiene.

**AGAINST.**
- The 2026-09-22 canonical reconciliation excluded a 116-definition COMMON wave. Its reason: pool adjacency cannot
  prove TU ownership, and rasterizer_frame_statistics is the counterexample. Patches 06/07 do not rely on adjacency
  alone, but the integrator should confirm the evidence standard.
- The /Od build is a later revision.

**Disclosure comments.** None; the objects.c precedent carries none.

## Advisory, not asked for a ruling
- render_debug has 6 C4013 implicit declarations (production too). ai/ai_debug.h and physics/collision_debug.h are
  not included, and 4 callees have no header prototype anywhere. render.c (Matching) has 2 C4013. Fixing this needs
  owner-header edits with a declaration-count sweep; it is outside this packet.
- render_debug.c has a stale "symbols in this file" comment with _code_/_bss_ placeholders. It is comment only;
  admitted objects carry the same kind of comment.
