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
| `NotoSansCJK-TC-Medium-subset.otf` | the characters Overpass lacks, for `chinese` subtitles | Noto Sans CJK TC, Medium, a subset |
| `NotoSansCJK-SC-Medium-subset.otf` | the same, for `chinese_simple` | Noto Sans CJK SC, Medium, a subset |
| `NotoSansCJK-JP-Medium-subset.otf` | the same, for `japanese` | Noto Sans CJK JP, Medium, a subset |
| `NotoSansCJK-KR-Medium-subset.otf` | the same, for `korean` | Noto Sans CJK KR, Medium, a subset |

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

## CJK fonts

The maps' fonts and Overpass have no Chinese, Japanese or Korean characters,
which the subtitles (`port/linux/game/subtitles.c`) can need. `fonts.json`'s
`fallbacks` give each of those subtitle languages a font that draws the
characters the font of a tag lacks (`port/linux/src/text_hires.c`); for the
other languages there is none. Each is a subset of Noto Sans CJK (Adobe and
Google, Source Han Sans), weight Medium, of the language's own region's
forms, since a character is drawn differently in each: Traditional Chinese
(TC), Simplified Chinese (SC), Japanese (JP) and Korean (KR).

Noto Sans CJK is under the SIL Open Font License 1.1 (`NotoSansCJK-OFL.txt`),
copyright 2014-2021 Adobe, without a Reserved Font Name, and the subsets keep
its names and license text, as the license asks. They were made from
[notofonts/noto-cjk](https://github.com/notofonts/noto-cjk) commit
`f8d157532fbfaeda587e826d4cd5b21a49186f7c` (`Sans/SubsetOTF/<region>/NotoSans<region>-Medium.otf`,
version 2.004) with fontTools, keeping the characters of the first levels of
the region's legacy character set (Big5, GB 2312, JIS X 0208 and KS X 1001's
Hangul) with the kana, the CJK symbols and the full-width forms, without
hinting or layout tables:

    python3 tools/cjk_fonts.py <noto-cjk checkout>/Sans/SubsetOTF port/assets/fonts

| File | SHA-256 |
| --- | --- |
| `NotoSansCJK-TC-Medium-subset.otf` | `361ddec79fdfb41411934bfd669e0f6825b71cc5eeab6e45d0c59374743a40e7` |
| `NotoSansCJK-SC-Medium-subset.otf` | `e5faecf9ddddae30337f0c52469ace1e061432581873039a6d72df58dc33ea1c` |
| `NotoSansCJK-JP-Medium-subset.otf` | `2e2d78df7d7e044a14062c893a0e59732e03b3241af6917e744d4abcac9fcb01` |
| `NotoSansCJK-KR-Medium-subset.otf` | `f079b8855d977a66471b43c8b2691b05d416bc00ed5d119d6ed02e40bf1d39c1` |

The builds embed the four subsets, and the release
packages carry `NotoSansCJK-OFL.txt`, as `Overpass-OFL.txt`.
