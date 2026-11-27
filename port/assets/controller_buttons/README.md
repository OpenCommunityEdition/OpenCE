# Controller button artwork

These PNGs were supplied by the project owner for this feature from the desktop
folders `playstation button style`, `xbox button style` and
`switch controller button style`. The retained `source/` files are unchanged;
`source-sha256.json` records their SHA-256 hashes. This records the supplied
provenance, not an independent identification of their original artist.

The existing Xbox Classic, Gems and Dark Gems assets remain in their original
folders and are not regenerated here. This set adds Xbox Modern, four PlayStation
styles and three Switch styles. Face slots follow SDL's physical south, east,
west and north positions. Switch therefore uses B, A, Y and X in that order.

Gamepads > Button Appearance can also use this set's existing `xbox_control_4`
(RB) and `xbox_control_5` (LB) for Xbox Classic, Gems and Dark Gems. Bumper Style
defaults to Original Xbox and does not regenerate or modify any artwork.

Regenerate with `python tools/controller_button_assets.py` from the repository
root. The script requires Pillow, NumPy and SciPy. It writes `buttons.json`, the
62 generated PNGs and `port/linux/game/controller_glyph_assets.h`. The generator
uses the repository's Overpass-900 font (see `../fonts/README.md`) to replace only
the supplied shoulder/trigger lettering: L1/R1/L2/R2 for PlayStation and
L/R/ZL/ZR for Switch. The project owner explicitly approved these deterministic
corrections. Source silhouettes, face artwork and original source files remain
unchanged. No commercial map fonts are used.

The short ASCII labels explicitly use Pillow's Basic text layout, matching
the committed artwork. Pillow otherwise selects Raqm automatically when it
is installed, which changes some label metrics and pixels on Linux.

The generator preserves each silhouette's aspect ratio within the established
HUD cell, and records half-alpha bounds for the renderer. Transparent padding
does not affect prompt centering. Thumbsticks retain the supplied three frames;
D-pad, Start and Back retain the existing renderer's fallback.

`tools/test_controller_buttons.py` regenerates and compares every output and
checks the production renderer's bounds, advances, split-screen scale, opacity,
allocation failure and cleanup. Asset generation does not require retail maps.

RGBA resizing explicitly disables Pillow's preliminary reduction pass so the
generated pixels match the committed artwork on both Pillow 12.3 and 13.
[Pillow 13 began applying `reducing_gap` to RGBA thumbnails](https://pillow.readthedocs.io/en/stable/releasenotes/13.0.0.html#resizing-images-with-alpha);
the supplied Xbox gem/control generator uses the same explicit setting.
