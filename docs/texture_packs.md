# Texture packs

A texture pack replaces the pictures of the maps' bitmaps with loose DDS
files of a higher resolution. It is a folder; the game reads it and nothing
else. Only the Mac build reads packs so far (`display.texture_pack`); the
loader (`port/linux/src/texture_pack.c`) has no platform code, so the other
builds can call it from their texture upload later.

## Using one

1. Put the pack in `<data root>/texture_packs/<pack>/` (the data root is the
   folder with `Maps`), by hand or with `tools/opence_texture_pack.py install`.
2. Set `display.texture_pack = "<pack>"` (or `HALO_TEXTURE_PACK=<pack>`).
3. The log names the pack, its author and licence, and at exit says how many
   pictures were used, left out for the budget, too large, refused or
   unsupported.

The pack name is letters, digits, space, `-`, `_` and `.`. Empty (the
default) is the stock game. A pack that has no `pack.toml`, or one without a
`name` and a `licence`, is refused whole and the stock textures are used.

## Layout

```
texture_packs/<pack>/pack.toml
texture_packs/<pack>/<tag path>/<bitmap index>.dds
```

`<tag path>` is the bitmap tag's path with `/` for `\`, as the maps name it
and lower case as the maps have it: the Blood Gulch ground is
`levels/test/bloodgulch/bitmaps/blood ground/0.dds`. `<bitmap index>` is the
bitmap's position in its tag, from 0 (most tags have one bitmap; the
particle and decal tags with sequences have several, and a sequence's
bitmaps are numbered in the tag's bitmap order). Only a plain character set
is read (letters, digits, space, `_ - .`), and no `.` or `..` component.

`HALO_MAC_TEXTURE_PACK_DUMP=<dir>` writes every DXT bitmap the game uploads
in this layout (existing files kept), to start a pack from, or to list the
names a map uses.

### pack.toml

```toml
name = "My pack"
version = "1.0"
author = "Someone"
licence = "CC0-1.0"            # required, as is name
description = "free text"

[files]                        # optional: SHA-256 of each file
"levels/test/bloodgulch/bitmaps/blood ground/0.dds" = "<64 hex digits>"
```

The game reads the top-level `name`, `version`, `author` and `licence` and
ignores the rest; the checksums are checked by `tools/opence_texture_pack.py`
(`verify`, `install`).

## Pictures

- DDS, with the 128-byte header (`DDS `), no DX10 header.
- DXT1, DXT3 or DXT5 (the preferred forms), or uncompressed 32-bit BGRA
  (`A8R8G8B8`, masks `00FF0000 0000FF00 000000FF FF000000`, or the same
  without alpha: opaque). Uncompressed is used only on cards with more than
  32 MB, and needs four times the memory.
- Width and height are powers of two up to 4096, and the file is exactly the
  size its header, levels and faces call for (a truncated or padded file is
  refused).
- The size is a power-of-two multiple of the bitmap's (`1x`, `2x`, `4x`...) with
  the same aspect, so the game's texture coordinates stay right. A smaller
  picture is refused.
- Mip maps are in the file, not made at run time. If the map's bitmap has
  mip maps, the chain must reach the bitmap's smallest level (a full chain
  down to 1x1 always does); if it has none (the HUD's), the file's first
  level is used alone.
- Cube maps (the maps' reflections) are one DDS cube map with all six faces
  (`DDSCAPS2_CUBEMAP` and the six face bits), faces in DDS order (+X, -X, +Y,
  -Y, +Z, -Z), each face's levels together. The bitmap must be a cube map too.
- Not replaced: volumes (3D bitmaps), the HUD meters, and any bitmap with no
  file. A picture's colours are what the game's shader should see (a bitmap
  that is not a colour picture, such as a multipurpose or bump map, keeps its
  channel meanings).

Any file wrong in any way (magic, header sizes, dimensions, size, mip count,
format, cube or not, aspect) is refused: the map's own bitmap is used and
the file named once in the log. Files over 96 MB are refused. Nothing in a
pack is trusted: no name in it is ever used to open another path.

## Memory (old Macs)

The pack's textures live in the game's texture cache and count against
`mac.texture_cache_mb`.

- A card of 32 MB or less takes no picture over 1024 pixels: the largest
  levels are left out (down to the bitmap's own size at least; a picture
  that still does not fit is not used), and uncompressed pictures are not
  used. Cards of up to 128 MB have a limit of 2048, larger ones 4096; the
  card's maximum texture size also limits it. `mac.texture_skip_levels`
  applies to a pack's pictures as to the maps'.
- One picture may take a quarter of the cache at most, and all the pack's
  pictures in use three quarters; a picture that would not fit is left out
  and the stock bitmap used.
- A card with no S3TC takes no DXT picture (and so, under 32 MB, none).
- Pack authors can offer tiers (`512/`, `1024/`, `2048/` packs); nothing is
  scaled at run time.

Diagnostics: `HALO_MAC_TEXTURE_PACK_MAX_EDGE=n` lowers the limit, to try a
small card's; `HALO_MAC_RETAIL_TEXTURE_CACHE_MB` sets the budget.

## The pack tool

`tools/opence_texture_pack.py` (Python 2.5 or later, or 3) manages packs. It is
separate from the game, which never opens an archive or the network.

```sh
python3 tools/opence_texture_pack.py [--data-root DIR] list
python3 tools/opence_texture_pack.py [--data-root DIR] verify <pack>
python3 tools/opence_texture_pack.py [--data-root DIR] install <zip-or-url> --sha256 <hash> [--name <pack>] [--force]
python3 tools/opence_texture_pack.py [--data-root DIR] remove <pack>
```

- `install` takes a zip file or an http(s) URL **that you give it** (there is no
  default), and a SHA-256 you got from the pack's author. It checks the
  archive against it, unpacks into a fresh scratch folder (names with `..`,
  absolute paths, backslashes, odd characters, symbolic links, encrypted or
  oversized members, duplicate names are all refused), runs `verify` on the
  result, and only then moves the pack into `texture_packs`. Nothing else is
  left behind, and nothing is run.
- `verify` checks `pack.toml` (name, licence), each file's name, its SHA-256
  if `[files]` lists it, and the DDS header and size by the loader's rules.
- Python 2.5 cannot check https certificates: on an old Mac, copy a pack
  installed on a modern machine, or download only from a URL you trust.

The project ships no pack and no pack URL. A pack's pictures are its author's
work under the licence in its `pack.toml`; do not use a pack whose licence you
have not read, and the repository takes no pictures from the Xbox, MCC or
Anniversary versions (see AGENTS.md).
