# Fonts

The fonts the game's text is drawn with, at the display's resolution, in
place of the bitmap fonts of the maps (`port/linux/src/text_hires.c`).
`fonts.json` says which font draws each of the maps' font tags.

| File | Draws | From |
| --- | --- | --- |
| `Overpass-900.ttf` | `ui\large_ui`, `ui\interstate` | Overpass, weight 900 (Black) |
| `Overpass-750.ttf` | `ui\small_ui` | Overpass, weight 750 |
| `OpenCE-Regular.ttf` | the menus' titles, as pictures (`port/assets/titles`) | OpenCE: Newtown, respaced for this project (`tools/title_font.py`) |
| `Newtown-Regular.ttf` | (OpenCE's source) | Newtown by Roger White, unmodified |

The maps' fonts are Interstate (Tobias Frere-Jones, The Font Bureau), a
commercial typeface that cannot be shipped. Overpass (Red Hat, Delve
Withrington) follows the same Highway Gothic road-sign alphabet and is
under the SIL Open Font License 1.1 (`Overpass-OFL.txt`), without a
Reserved Font Name. Matched against the maps' glyphs, its weights 900 and 750 overlap
them as much as Interstate Bold does, at the same letter widths.

The menus' titles are pictures of text in the maps, set in another
commercial typeface. OpenCE (Open Community Edition) stands in for it:
Newtown, a typeface in the same style by Roger White (1994), which its author
released into the public domain (`Newtown-LICENSE.txt` quotes his statement
and where it is archived), respaced for this project. Only its spacing is
changed, its outlines are Newtown's: each letter's side spaces and the
kerning pairs are measured from where the letters sit in the maps' titles.
OpenCE is under the SIL Open Font License 1.1 (`OpenCE-OFL.txt`), without a
Reserved Font Name. It is not embedded: `tools/title_assets.py` draws the
titles with it, and the builds embed those pictures. To measure its spacing
again and rebuild it:

    python3 tools/title_font.py --measure    # paste the values into tools/title_font.py
    python3 tools/title_font.py
    python3 tools/title_assets.py --map assets/maps/ui.map

The Overpass files are static instances of Google Fonts' variable Overpass
(`ofl/overpass/Overpass[wght].ttf`, google/fonts commit
9710da1eacb3be272583c3224dcb70f9da6eadbb), made with fontTools:

    python3 -m fontTools.varLib.instancer 'Overpass[wght].ttf' wght=900 -o Overpass-900.ttf
    python3 -m fontTools.varLib.instancer 'Overpass[wght].ttf' wght=750 -o Overpass-750.ttf

The builds embed the Overpass files (`tools/embed_assets.py`), and the
release packages carry `Overpass-OFL.txt`, as the license asks.
