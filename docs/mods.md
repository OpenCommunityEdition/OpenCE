# Mods

A mod is a folder of Halo PC tag files that the game plays over a map's own
tags on your machine, as a resource pack does: sounds (a high-res audio
pack), bitmaps (a texture pack, a HUD redraw) and text (a translation). Only
the machine that has a mod plays it. Nothing of a mod is sent to another
machine, and a mod changes nothing that the machines of a network game must
agree on, so you can play with mods in any game, with players who have
other mods or none. (A host's mod sounds of other lengths change how long
the host's scripts and AI wait for dialogue to end; the host decides those
for every machine, so they still agree.)

The game's code is `port/linux/game/mods.c` (with `mod_sounds.c`,
`mod_bitmaps.c` and `mod_strings.c`, one for each kind of tag) and
`port/linux/src/port_mods.c` (the mods folder and its files).

## A mod's folder

The mods are in the `mods` folder of the data root (the folder with
`maps/`; on Android the app's data folder, on the web the browser's storage
for the site). Each mod is a folder in it, with a `mod.toml` and a `tags`
folder:

```
mods/
  hd_audio/
    mod.toml
    tags/
      sound/sfx/weapons/assault rifle/fire.sound
      sound/dialog/...
  german/
    mod.toml
    tags/
      ui/hud/hud_item_messages.unicode_string_list
```

The files in `tags` have the paths of the tags they replace: a tag's name,
then its group's extension, as the Halo Editing Kit and Invader keep them.
`tags/sound/sfx/weapons/assault rifle/fire.sound` replaces the sound tag
`sound\sfx\weapons\assault rifle\fire` of every map that has one. Names are
not case-sensitive. The game only looks up the names of the map's tags among
a mod's files: a mod cannot add tags, and no tag name of a map makes a path
outside the mod.

`mod.toml` says what the mod is. Every key is optional:

```toml
name = "High-res audio"       # what the Mods menu shows (else the folder's name)
id = "hd_audio"               # what other mods name it by (else the folder's name)
version = "1.0"
authors = ["Someone", "Someone else"]   # or one string
description = "The game's sounds at 44 kHz"
maps = ["a10", "bloodgulch"]  # the maps it is for, by their files' names (else every map)
load_after = ["hd_textures"]  # mods it plays over: they load before it
load_before = ["fixes"]       # mods that play over it: they load after it
incompatible = ["other_audio"]  # mods it cannot be played with
format = 1                    # the mods' format it is made for
```

`load_after`, `load_before` and `incompatible` name other mods by their `id`
or their folder's name, not case-sensitive. A mod they name that is not in
the mods folder is passed over. `maps = []` is every map.

A folder without `mod.toml` is not a mod. A mod made for a later `format`
than the build's, or whose `maps` names no map, is not played. The game
logs anything else wrong in `mod.toml` (an unknown key, a value of the
wrong type) and uses the rest; a `mod.toml` that is not TOML at all is
logged, and the mod played without what it says.

The game's log is its output: the terminal on Linux, `halo.log` beside
`halo.exe` on Windows, logcat on Android, and the page's log on the web.

Limits: 256 mods, folder names of 63 bytes at most, 262144 files in all, 24
folders deep under `tags`, and 64 maps in a mod's `maps`.

## The load order

Every mod in the mods folder is on until you turn it off. The mods load in
an order, and where two mods have a file for the same tag, the one loaded
later plays.

- The default order is the folders' names in alphabetical order (not
  case-sensitive), except that the mods a mod must load after (those it
  asks to load after, and those that ask to load before it) are taken to
  just before it. Mods that ask in a ring (a after b, b after a) load
  alphabetically, and the game's log says so.
- The main menu's **Mods** (in the PC version's menus, `display.menus =
  "pc"`, the default) is laid out as the game lobby is: the mods on the
  left as a resource pack list has them, the one at the top playing over
  all those below it (the load order upside down: the mod loaded last is at
  the top), and the chosen mod's details in the frame on the right (its
  name, what is wrong with it, whether it is on, its priority, 1 the top,
  and its version, authors, folder, maps and description). Choosing a mod
  (A, or a click on the chosen one: the first click chooses it) turns it on
  or off (ON, OFF); left and right, or **Move up** and **Move down**, move
  the chosen mod (up: over one more mod). **OK** keeps the order and plays
  it at once (on a map that is loaded, too); **Cancel** leaves it as it was.
- A warning sign beside a mod means it is on the wrong side of a mod it
  asks to load before or after (both on): a mod that loads after another is
  above it in the menu. A red cross means a mod that is on says it cannot be
  played with this one, or that it cannot be played at all (made for a later
  build, or its `maps` names none): it stays off. The details say which.

The menu keeps the order, and the mods turned off, in `mods/load_order.toml`,
by the mods' folders' names:

```toml
order = ["german", "hd_audio", "hd_textures"]
disabled = ["hd_textures"]
```

A mod added to the folder later is put where the default order puts it,
after the mod before it there (but on the side it asks for of the mods it
names), and is on. A mod gone from the folder is left out. When the file
cannot be written, the game says so in its log and keeps the order until it
quits.

### Mods that cannot be played together

When a mod that is on names another that is on as `incompatible`, the game
asks as it starts, before the window opens:

- **Play anyway** plays them all (closing the box does too);
- **Turn off ...** turns off the mods named as incompatible (if mod A says
  it cannot be played with mod B, B is turned off), and keeps that in
  `load_order.toml`;
- **Quit** closes the game.

The Mods menu turns them on again. On the web, **Quit** starts the page
again. A run nobody watches (`debug.hidden_window`, `debug.null_renderer`,
`debug.exit_after` or `debug.network_test`) is not asked: it plays them all,
and the game's log says which conflict.

At the console:

| Command | What it does |
| --- | --- |
| `mods_list` | Lists the mods in the mods folder in their load order, and which are on, then how many of the map's string lists, bitmaps and sounds the mods play. |
| `mods_reload` | Finds the mods again and reads their files again: a changed file is read again (one that now fails a check leaves what was played before), a new one is played, and the map's tag comes back for a file that is gone. If a sound changed, all sounds stop. It does nothing while `mods_enable false`. |
| `mods_enable false` | Plays the map's own tags until `mods_enable true` or the next map. |

These commands work in any game, a client's too: they change nothing of the
game.

### Getting a mod onto each platform

- **Linux, Windows**: put the mod's folder in `mods/` next to `maps/`.
- **Android**: put it in `mods/` in the app's data folder,
  `/sdcard/Android/data/com.halo.decomp/files/mods`, for example with
  `adb push hd_audio /sdcard/Android/data/com.halo.decomp/files/mods/`.
- **Web**: on the page, **Mods**, **Add a mod folder…** copies a mod's
  `mod.toml` and `tags` folder into the browser's storage for the site.
  **Remove** deletes it again.

## What a mod can replace

The game plays these groups from a mod. It does not read the files of any
other group (a `.weapon` or `.scenario`, for example): they would change what
the machines of a network game must agree on.

| Group | Extension | What it is |
| --- | --- | --- |
| Sound | `.sound` | Sounds, music and dialogue |
| Bitmap | `.bitmap` | Textures, sprites, the HUD's pictures |
| Unicode string list | `.unicode_string_list` | The text the game shows |
| String list | `.string_list` | Other text |

A file must be a Halo PC tag file of its group, as the Halo Editing Kit's
tools and Invader write them. The game checks every count, size and offset
in it against the file, and every value against what the game reads, before
it uses the file. A file that fails a check is left out (the map's tag
plays), and `debug.txt` and the console say why.

### Sounds

- The samples can be 16-bit PCM, Xbox ADPCM or Ogg Vorbis, mono or stereo,
  at 22 or 44 kHz. The game converts them to the Xbox ADPCM that it plays,
  as it does for Custom Edition maps: mono at 22 kHz, stereo at its own rate.
- Empty values are filled in as a map's build fills them in: the defaults
  of the sound's class, the playback rates and the bend.
- A sound must have permutations: a file with an empty pitch range (some
  Halo PC translations have them) is left out.
- A permutation must fit in a block of the sound cache (one sound's samples:
  `MAXIMUM_SOUND_DATA_SIZE`).

### Bitmaps

- A bitmap's types and formats are those the game draws: 2D, 3D and cube
  maps; A8, Y8, AY8, A8Y8, R5G6B5, A1R5G5B5, A4R4G4B4, X8R8G8B8, A8R8G8B8,
  DXT1, DXT3, DXT5 and P8 bump. Sides are 4096 at most (512 for a 3D
  bitmap). A bitmap that is not a power of two on each side is drawn
  linear, as a Custom Edition map's is: it must be an uncompressed 2D
  bitmap.
- One bitmap's pixels may take a quarter of the texture cache: 64 MB on
  Linux and Windows, 5.5 MB on Android and the web (a 2048 x 2048 DXT5 with
  its mipmaps fits on all of them).
- A bitmap replaces the map's bitmap of its name, and the map's other tags
  use it as the map's: it must have at least as many bitmaps, of the same
  types, and as many sequences, each with at least as many sprites. It can
  be larger: a redraw at twice the size is drawn at the map's size, sharper.
- The HUD draws a bitmap at its own size. So for each HUD element that draws
  a mod's bitmap, the game scales the element by the map's size of what it
  draws (its sprite, or with no sprites its bitmap) to the mod's, when that
  is the same ratio for all of them. A bitmap the HUD draws that is not the
  map's redrawn so (Halo PC's HUD bitmaps lay their sprites out to other
  sizes) is left out. The HUD's digits can be the map's size or twice it.
- The menus draw a bitmap in their widgets' bounds, whatever its size.
- Halo PC keeps the channels of a model shader's multipurpose maps and of
  the HUD's meters in another order than the Xbox. The game finds those
  bitmaps among the mod's and draws them in the Xbox's order, as it does for
  Custom Edition maps (`custom_edition_bitmaps.c`).
- The pixels are read from the file when the texture cache loads them, not
  kept in memory.

### Text

- A list keeps the map's number of strings: a mod's strings past it are
  left out, and the map's own strings stay where the mod's list is shorter.
- The game uses many strings as the format of what it prints
  (`%s was killed by %s`). A mod's string replaces the map's only if it has
  the same conversions in the same order (`%s`, `%d`, ...); otherwise the
  map's string stays. A translation keeps the conversions of the original.
- Unicode strings are UTF-16, as the tool writes them.
- The main menu is the port's own (the PC version's menus, `port/assets/menus`),
  whose text is in tags named `pc\...`. A mod's `ui\shell` lists change the
  Xbox's menus (`display.menus = "xbox"`) and the text shown while playing
  (HUD messages, the multiplayer messages, the pause menu).

## Making a mod

1. Get the tags you want to change as tag files. `invader-extract` extracts
   them from a map, and the Halo Editing Kit's tags are Halo PC's own.
2. Change them with the Halo Editing Kit or Invader (`invader-bitmap` and
   `invader-sound` build bitmaps and sounds from images and audio files).
   Keep each tag's name.
3. Put them in `tags/` of a folder in `mods/`, with a `mod.toml`.
4. Load a map (a new mod is on). `mods_reload` at the console reads the
   files again as you change them, without loading the map again.

`debug.txt` has a line for every file the game leaves out, and why, and a
summary of what each kind of tag plays from the mods:

```
mods: 9 string lists from their files (9 new), 0 given back, 0 left out
mods: 66 HUD placements scaled to draw larger bitmaps at the map's size
mods: 354 bitmaps from their files (354 new), 0 given back, 6 left out
mods: 313 sounds from their files (313 new), 0 given back, 2 left out
```

## How it operates

- When a map's tags load, at `mods_reload` and at the Mods menu's **OK**,
  the game lists the files of the mods that are on, once, in a table of their tag paths. Each of the
  map's sound, bitmap and string list tags is looked up in it.
- A tag replaced by a mod takes the file's values in place, so everything
  that holds the tag keeps it, and the map's own values come back when the
  file goes. A sound playing or a texture in the cache is never changed
  under the game: before a reload changes anything, the sounds stop and the
  caches let go of what they hold.
- The mods' tags keep at most 512 MB in memory (192 MB on Android and the
  web): their files, their sounds' samples and their bitmaps' tags.
- A Custom Edition map takes mods as an Xbox map does.
