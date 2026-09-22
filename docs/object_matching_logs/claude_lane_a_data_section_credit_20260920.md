# Lane A data: credit is all-or-nothing per section - 2026-09-20

**The finding.** The build's Data metric comes from objdiff's `report.json`, and
objdiff credits a data SECTION only when it matches 100%. A section at 99.5%
earns **zero**. So a single tiny gap blocks an entire section's credit.

Discovered by accident: defining one missing 1-byte constant,
`ai_script_squad_separator = '/'`, moved the board's Data line from 2,360,994 to
**2,364,914 (+3,920)** - the whole of `ai_script`'s `.rdata` became creditable at
once.

## Lane data, per unit (authoritative, from build/report.json)

    unit                       data   matched     pct
    props                       964       964  100.00%
    encounters                 3316      3316  100.00%
    ai_script                  3920      3920  100.00%   <- closed this session
    actors                     2288      2288  100.00%
    actor_perception           2068      2068  100.00%
    actor_moving               1960      1960  100.00%
    actor_looking              1700      1700  100.00%
    actor_firing_position      1260      1260  100.00%
    ai_debug                   5874      5744   97.79%   .bss 95.9, .data 60.7
    ai                         3324      1764   53.07%   .data 97.8
    ai_communication          11892        16    0.13%   .rdata 99.5, .data 50.0
                              -----     -----
    LANE                      38566     25000   64.8%

**13,566 lane data bytes are blocked, and none of the blocking sections has a
single differing CONTENT byte.** Every one was diffed byte by byte
(`scratch/orch/secdiff.py`), with relocation sites reported separately.

## What blocks each section

| section | size | content diffs | blocker | reachable by |
|---|---:|---:|---|---|
| `ai_communication .rdata` | 11,600 | - | two string literals ABSENT (55 B) | **the header prerequisite** |
| `ai .data` | 1,560 | 0 | January's split has no name for `_ai_sound_volume_names` | symbols.json (naming gap) |
| `ai_communication .data` | 276 | 0 | six string literals defined elsewhere by the splitter | nothing - see below |
| `ai_debug .bss` | 74 | 0 | January's split has no name for `_global_ai_debug_path_render_id` | symbols.json (naming gap) |
| `ai_debug .data` | 56 | 0 | three private globals lack `"static": true` | symbols.json (linkage) |

### `ai .data` - measurement trap #4, in data

January's reference to the volume-name table is spelled
`_ai_update_section + 0x5f8`; ours is `_ai_sound_volume_names + 0`. **Same
address** - `_ai_sound_volume_names` sits at +1528 = 0x5f8. January spells it
against the preceding symbol only because its split has no name for the table.
`config/symbols.json` has `_ai_update_section` at file offset 2845936 and
`_ai_sound_volume_enum` at 2847484, and nothing between; the table belongs at
2847464.

### `ai_communication .data` - NOT reachable, and why

Six relocations target string literals (`"none"`, `"shout"`, `"damage"`,
`"cover"`, `"flee"`, `"berserk"`) that are **undefined** in January's split
object but **defined** in ours. The linker folded identical literals and the
splitter attributed the single survivor to another object - the same COMDAT
ownership situation already ruled on for `_point_from_line3d`. Under that ruling
the split's attribution is authoritative, but unlike `_point_from_line3d` there
is **no way to avoid emitting a string literal that an initialiser uses** in C.
So these 276 bytes are blocked by splitter attribution, not by source.

### A correction to my own census

My first pass (`scratch/orch/lanedata.py`) reported 1,658 "different" data bytes
across four symbols. That was wrong. It compared whole symbols by raw bytes, so a
BSS symbol (no file bytes) never equalled January's materialised zeros, and a
relocation site's in-place addend counted as content. Diffing properly: **zero
content differences** in all four. The real blockers are the naming and linkage
items above.

## The recommendation this CHANGES

`claude_lane_a_deferred_header_prerequisite_allegiance_20260920.md` recommended
**not** landing the `game_allegiance_get_incidents` prototype on its own, because
it closes only 72 of `_ai_communication_event`'s missing code bytes and so
"banks zero bytes". **That was wrong once data is counted.** The two string
literals it would let us emit - `"allegiance %s, %d incidents (thr..."` and
`"still holds"` - are exactly the 55 bytes keeping `ai_communication .rdata` at
99.5%. Emitting them should make that section creditable: **roughly 11,600 data
bytes** from a one-declaration header edit, before counting any code.

That is new evidence on a held item, which is the standing condition for
re-raising it.

## Scope and rules

- The header edit is still NOT made - it needs a ruling.
- The symbols.json items need **authenticated name provenance** under the lane
  brief, and symbols.json must be edited by line surgery only - its alias
  precedence at shared offsets has flipped 46 functions before. Not yet done.


## RESULT - 2026-09-20: +17,080 lane data bytes, zero regressions

| commit | change | data bytes |
|---|---|---:|
| `94c32737` | define and use `ai_script_squad_separator` | +3,920 |
| `264ddd8a` | owner-approved header + the decoded allegiance block | +11,600 |
| `b423909b` | name `ai_sound_volume_enum_strings` from HCEX | +1,560 |
|  | **total** | **+17,080** |

Board Data line: 2,360,994 -> **2,378,074**. Lane data goes from 25,000 / 38,566
(64.8%) to **38,160 / 38,566 (98.95%)**. Every lane unit is now 100% except
`ai_debug` (130 B) and `ai_communication .data` (276 B) - the 406 bytes below.

### What remains, and why it stays

- **`ai_debug .bss` (74 B) and `.data` (56 B)** - blocked on PROVENANCE. The fixes
  are a name for `_global_ai_debug_path_render_id` and `"static": true` on three
  globals, both symbols.json edits that the lane brief allows only with
  authenticated provenance. **`ai_debug.obj` is absent from BOTH HCEA PDBs** - the
  SHIP and the RELEASE_CACHE compiland lists - because it is debug-only code
  compiled out of shipping configurations, and the symbol atlas covers code not
  data. Nothing can attest these names, so they stay.
- **`ai_communication .data` (276 B)** - blocked by splitter attribution of six
  folded string literals; unreachable in C, as recorded above.

## The method, for reuse

1. Read `build/report.json`, not a symbol census. objdiff credits a data section
   all-or-nothing, so find sections at 95-99.99%.
2. Diff the section with `scratch/orch/secdiff.py`, which separates relocation
   sites from content. **In every case in this lane the content was identical**;
   the blocker was one of four things:
   - an ABSENT small symbol (1 B `'/'` blocked 3,920 B);
   - an absent string literal whose emitting code is missing (55 B blocked 11,600 B);
   - a NAMING GAP - January's split has no name for an object, so a relocation is
     spelled against the preceding symbol (trap #4 in data);
   - folded COMDAT literals the splitter attributed elsewhere (unreachable).
3. For a naming gap, get provenance from the HCEA PDB:
   `DIA2Dump.exe -compiland "..\..\..\build\x360\SHIP\halo\<unit>.obj" HCEX.pdb`
   lists every static with name, type and size. It found our name was wrong.
4. Edit symbols.json by line surgery only, at an offset nothing occupies, then
   hash all 833 split objects before and after the re-split to prove only the
   intended one changed.

## Board-wide: this is not a lane-specific finding

Across the whole board, **43 data sections sit at 95-100% match and hold 109,111
bytes that earn zero credit today**. 42 of them - 109,037 B - are outside this
lane. The largest: `effects/decals .bss` 30,930 B at 99.376%;
`interface/ui_widget .data` 16,700 B at 99.377% and `.rdata` 7,880 B at 99.947%;
`hs/hs_compile .rdata` 6,576 B; `cache/xbox_texture_cache .bss` 5,656 B. Flagged
for separate work rather than done here, since it is outside this lane's scope.


## UPDATE 2026-09-20 - re-examined after the board-wide data lane finished

The board-wide data lane (branch `claude/data-section-credit-20260920`) closed
+119,772 data bytes using this finding, and brought back three methods this note
did not use: January's OWN `cachebeta.pdb` public-symbol list for linkage, the
literal-address law, and `.bss` layout ordering. All three were applied to this
lane's remaining gaps. **None of the 406 bytes moves, but two of this note's
diagnoses were wrong and are corrected here.**

### Merge with that branch: clean

The two branches touch exactly one common file, `config/symbols.json`, on
different lines. `git merge-tree` reports no conflict. All 13 of the data lane's
`symbols.json` lines name objects outside this lane (observer, director, decals,
the teleporter game engine, rasterizer). Its header rename of the
`bitmap_group` field `bitmap_data` -> `bitmaps` touches no Lane A file, and no
Lane A file uses `bitmap_data`. A full ninja on the merged tree is still the
right check before trusting the combination.

### `ai_debug .data` (56 B) - the "three globals lack static" diagnosis was WRONG

January's own PDB (`cachebeta.pdb`, DIA2Dump `-p`) settles linkage for all eight
of `ai_debug`'s data globals, and it **vindicates our source**: the two debug
arrays and `global_ai_debug_firing_position_color_count` are public; the other
five - including `postcombat_type_strings` and the two squad globals - are
absent from the publics, i.e. file-static, exactly as `ai_debug.c` declares
them. So the source was already right. Adding `"static": true` for them in
symbols.json was measured and **moved nothing** - the re-split changed only
`ai_debug.obj` and no report row moved - so it was reverted rather than left in a
file another lane has to merge.

The real blocker is the **literal-address law**: objdiff matches a pointer to a
string literal only when the literal sits at the same address in the combined
`.rdata`. `postcombat_type_strings` is eleven pointers. Ten of its literals keep
January's exact order and spacing, but January emits them FIRST in the object's
`.rdata` (offset 0) while ours start at offset 288, behind other literals. The
eleventh, `"none"`, is not defined in January's `ai_debug.obj` at all - the
folded copy belongs to another object - so that pointer can never match. **The
section therefore cannot reach 100% even with the order fixed, and earns zero
data credit either way.** Not pursued.

### `ai_debug .bss` (74 B) - still blocked on the name

January's PDB lists the 2-byte slot at +72 as not public, so it is file-static,
consistent with our `static short global_ai_debug_path_render_id = 0`. But no
January public symbol, assert string or HCEA record names it, so the name still
cannot be attested for symbols.json.

### `ai_communication .data` (276 B) - still blocked

It has no entry in `config/semantic_data_matches.json`, so it is not credited
semantically either; the folded-literal attribution recorded above stands.

### Corrected diagnosis, for the record

The table earlier in this note lists `ai_debug .data` as "three private globals
lack static: true". That is **wrong** - the true blocker is literal placement
plus the folded `"none"`. Linkage was never the problem.
