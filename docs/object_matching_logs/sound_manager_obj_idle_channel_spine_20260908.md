# `sound_manager.obj` idle/channel spine reconstruction (2026-09-08)

## Result

This packet advances `source/sound/sound_manager` from **32 strict-exact,
8 residual, and 25 unwritten** January function owners to **40 strict-exact,
13 residual, and 12 unwritten** owners.  It adds eight genuine exact functions,
**2,034 meaningful / 2,112 padded exact code bytes**, and retains five newly emitted, complete
source reconstructions as measured fuzzy results with zero exact credit.  No
inherited residual was tuned.

The requested caller-connected spine covers 3,792 January padded bytes.  With
its two direct private dependencies, this packet reconstructs 4,576 January
padded bytes in total.

| semantic source function | January owner used for the gate | January padded bytes | result |
|---|---|---:|---|
| `sound_idle` | `_sound_idle` | 112 | **exact** |
| `sound_update_time` | `_sound_update_time` | 48 | **exact** |
| `update_channels` | `_update_channels` | 864 | **exact** |
| `update_channel_for_looping_sound` | `_update_channel_for_looping_sound` | 1,360 | fuzzy: candidate 1,376, same relocation count, SHA differs; **zero credit** |
| `update_channel_for_impulse_sound` | `_update_channel_for_impulse_sound` after reviewed rotation | 528 | fuzzy: same size and relocation count, SHA differs; **zero credit** |
| `channel_set_properties_hardware` | `_channel_set_properties_hardware` | 160 | **exact** |
| `channel_queue_sound` | `_channel_queue_sound` | 128 | **exact** |
| `channel_get_state` | `_channel_get_state` | 240 | **exact** |
| `limit_pitch` | `_limit_pitch` | 96 | fuzzy: candidate 112, SHA differs; **zero credit** |
| `sound_manager_master_gain` | `_sound_manager_master_gain` | 48 | **exact** |
| `sound_set_definition_end` | `_sound_set_definition_end` after reviewed rename | 208 | fuzzy: same size and relocation count, SHA differs; **zero credit** |
| `sound_channel_summary_build` | `_sound_channel_summary_build` | 512 | **exact** |
| `sound_find_like_channel` | `_sound_find_like_channel` | 272 | fuzzy: same size and relocation count, SHA differs; **zero credit** |

The exact rows have the following January normalized hashes:

| owner | padded bytes | relocations | normalized SHA-256 prefix |
|---|---:|---:|---|
| `_sound_idle` | 112 | 14 | `085dc0186fa101c9` |
| `_sound_update_time` | 48 | 5 | `33deb9bf16ce2e64` |
| `_update_channels` | 864 | 51 | `a51542452d0006ed` |
| `_channel_set_properties_hardware` | 160 | 12 | `e647272b731fcf12` |
| `_channel_queue_sound` | 128 | 9 | `fa9faabe6c779a25` |
| `_channel_get_state` | 240 | 15 | `e9fb1d4c81e46eae` |
| `_sound_manager_master_gain` | 48 | 2 | `6a2cc8c25eac16c9` |
| `_sound_channel_summary_build` | 512 | 32 | `f4ae27c691076057` |

## Reconstruction evidence

January target disassembly, relocation order, object layout, and assertion
strings are authoritative.  The HCEA reconstruction beneath
`scratch/_halocea-reference-20260907/src/blam/{cache,sound,game}` supplies the
semantic control-flow and field-name map, while donor commit
`e4c48546384d1c11c7c1463db0c6676d283aff7e` supplies an independent repository
source checkpoint.  Later-build behavior was discarded wherever the January
object disproved it.  In particular, this packet uses the January 0x3C sound
platform vtable shape and the older two-argument channel queue / one-argument
channel update calls rather than the later HCEA interfaces.

The exact `_sound_idle` body naturally inlines the exact
`sound_update_time` arithmetic.  The source-level cast precedes the subtraction,
which produces January's `fild`/`fisub` sequence both in the standalone helper
and at the expected inline site.  `_update_channels` closes without compiler
steering: the January expression order for the relative-position magnitude and
the nested mouth-aperture call reproduce all 864 bytes and all 51 relocation
sites.  No `point_from_line3d` helper or COMDAT is emitted.

The newly exact channel helpers use project datum/tag accessors and the
January platform interface.  The channel state machine releases finished
cache permutations, promotes queued permutations, and advances the estimated
tick count.  The master-gain helper preserves January's dialog-class exception
and global non-dialog multiplier.  A root audit restored the four target-proven
capacity assertions missing from the initial donor lift and the two genuine
`sound_class_get` calls.  That closes the complete 512-byte channel-summary
builder exactly; it is a semantic bounds repair, not allocator tuning.

## Required symbol corrections

The current symbol labels cannot be used as semantic source names.  These are
target-symbol corrections for the owning symbol map, not aliases to publish in
C:

1. January address `0x001BCA70`, previously `_sound_find_best_channel`, is
   semantic `update_channel_for_impulse_sound`.  Its 528-byte body constructs
   platform properties, asserts the permutation is cached, queues it, records
   the playing channel, and updates the hardware channel.  The exact
   `_update_channels` relocation at function offset `0x02E1` targets this owner.
   An initial isolated diagnostic used an alias; production now carries the
   corrected target label directly.
2. January address `0x001BD3A0`, previously `_sound_find_channel`, is semantic
   `sound_find_best_channel`.  Its 416-byte body scans compatible hardware
   channels, immediately accepts a free one, and otherwise uses
   `sound_preempts_sound` and distance to select the best victim.
3. January address `0x001BE6A0`, previously
   `_update_channel_for_impulse_sound`, is semantic `sound_find_channel`.  Its
   496-byte body performs speech/source reuse and instance-limit selection.  Its
   calls at offsets `0x0167` and `0x01D3` target the 416-byte owner currently
   labeled `_sound_find_channel`, confirming the semantic best-channel helper
   in item 2.  It does not update impulse hardware properties.  Rotate all
   three labels atomically only after checking every call relocation; no credit
   is claimed for the 416- or 496-byte owners in this packet.
4. January address `0x001BCDB0`, previously `_looping_sound_new_sound`, is
   semantic `sound_set_definition_end`.  It swaps to the queued end definition,
   reselects pitch range/permutation, applies instance limits, and stops the
   selected sound.  `_update_channel_for_looping_sound` relocates to it at
   function offset `0x03D7`.  Production now carries the corrected target label
   directly.  This is distinct
   from January's exact `_update_potentially_audible_looping_sound`, which is the
   allocator called `looping_sound_new_sound` by the later HCEA reconstruction.

These mappings are supported jointly by the HCEA release-PDB semantic names,
the January bodies, and the January caller relocations.  The source retains the
semantic names and deliberately does not claim an exact or fuzzy result under
a knowingly wrong semantic label.

## Cross-TU ownership

The three temporary gate declarations were removed from `sound_manager.c`.
Their genuine owners and exact signatures are now published in
`sound_definitions.h` and `game_sound.h`:

```c
/* sound_definitions.h; definition is sound_definitions.c:167 */
real sound_permutation_get_real_mouth_aperture(
	struct sound_permutation *permutation,
	long tick_index);

/* game_sound.h; definition is game_sound.c:911 */
void game_sound_set_mouth_aperture(
	long object_index,
	real mouth_aperture);

/* game_sound.h; declaration game_sound.c:224, definition game_sound.c:1009 */
boolean track_object_impulse_sound(
	long object_index,
	void const *attachment_data,
	struct sound_source *source);
```

January `game_sound.obj` defines `_track_object_impulse_sound` with
COFF storage class 2 (external), and January `sound_manager.obj` carries an
undefined class-2 external for it.  The production definition is therefore
external and the old duplicate static declaration is gone.  The shared
16-entry instance-limit constants likewise moved from `sound_classes.c` to
their genuine `sound_classes.h` owner; the full consumer rebuild verifies the
migration.

## Validation

The final isolated production command was:

```text
python -B tools/campaign/gate.py source/sound/sound_manager --all --forbid-emitted-symbol _point_from_line3d --out %TEMP%\sound_packet_final.obj
```

It reported **40 exact / 13 residual / 12 unwritten of 65**, and the forbidden
emitted-symbol guard passed.  `relocdiff` independently reports that exact
`_update_channels` has 864/864 bytes, 51/51 relocations, an equal normalized
hash, and zero differing relocation rows.  The summary builder is also exact
at 512/512 bytes and 32/32 relocations.  The source fake-match scan reports
zero review leads, and `git diff --check -- source/sound/sound_manager.c`
passes apart from Git's existing LF-to-CRLF advisory.

The aggregate root checkpoint records the shared symbol map, park entries,
full build, rename-stable sweep, tests, commit and publication results.
