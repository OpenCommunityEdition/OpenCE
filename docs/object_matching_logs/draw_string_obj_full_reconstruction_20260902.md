# `draw_string.obj` full reconstruction — 2026-09-02

## Result

This wave starts from canonical commit `f9ad12e6b` in the isolated branch
`agent/draw-string-full-20260902`. It reconstructs every previously unwritten
function in `source/text/draw_string.c`, assigns the anonymous January
functions and BSS aggregate semantic names, moves public declarations to the
owning header, and retains only ordinary typed C.

The strict per-section gate moves from 13 exact, zero residual, and 15
unwritten functions to 18 exact, ten residual, and zero unwritten functions.
Strict exact coverage in this object moves from 1,152 to 2,176 padded bytes,
for five functions and 1,024 padded bytes gained. A stable-section snapshot of
all 8,245 target functions reports those same five gains and zero regressions.

The conservative campaign progress display moves from 733,604 to 734,328 code
bytes and from 5,360 to 5,364 functions: +724 displayed code bytes and +4
displayed functions. The difference from the strict five-function gain is the
270-byte `_parse_unicode_string`: the stable COFF comparator and semantic
report prove it exact after its anonymous private-symbol rename, while raw
objdiff still displays 21.511627 percent and therefore does not add it to the
ordinary progress total.

| Function newly strict exact | Code bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_text_bounds_draw_character` | 107 | 112 | 10 | `8497c0497bf3d1c1f59bb22c46ddb237ab9338a5f37d0b97114eba3c10e92c6e` |
| `_parse_string_new` | 247 | 256 | 17 | `e257eb07009cc417d63fadc3f316a2a9a7ab537df9f77409c22e2f2ac7e8d60e` |
| `_parse_unicode_string` | 270 | 272 | 7 | `933bc5d77f06ac0cdd924f9e0b97eb09f817bf5250929087f0329c80ac8f8219` |
| `_draw_string_compute_bounds` | 185 | 192 | 14 | `1546e737cb0c9365d2894beae56d2890e6af2028be8d00e26b8e0bab67264628` |
| `_draw_unicode_string_compute_bounds` | 185 | 192 | 14 | `1546e737cb0c9365d2894beae56d2890e6af2028be8d00e26b8e0bab67264628` |

The ten non-exact implementations are recorded in `config/parked.json` as
measured `unclassified` fuzzy residuals. The production fail-closed classifier
returns `UNKNOWN`, class `unclassified`, confidence `NONE` for every one; no
specific compiler recipe is inferred.

| Honest fuzzy implementation | Target / candidate padded bytes | Target / candidate relocations | Objdiff percent |
| --- | ---: | ---: | ---: |
| `_bitmap_draw_character` | 944 / 912 | 25 / 25 | 70.833885 |
| `_bitmap_draw_string` | 304 / 240 | 9 / 9 | 51.18261 |
| `_draw_string` | 912 / 976 | 46 / 45 | 48.085617 |
| `_draw_string_partial` | 528 / 544 | 9 / 9 | 63.23656 |
| `_draw_string_set_tab_stops` | 112 / 112 | 8 / 7 | 94.0 |
| `_draw_unicode_string` | 912 / 960 | 46 / 45 | 53.439026 |
| `_draw_unicode_string_partial` | 528 / 544 | 9 / 9 | 62.967567 |
| `_parse_string` | 624 / 624 | 30 / 29 | 27.615385 |
| `_styled_font_get` | 112 / 112 | 6 / 6 | 86.6129 |
| `_text_pick_draw_character` | 224 / 224 | 11 / 11 | 80.10256 |

The complete target/candidate normalized hashes and the source-level reopening
criteria for each residual live beside the measurements in
`config/parked.json`; `python -m tools.parked_functions` validates all ten.

## Provenance and semantic reconstruction

The primary independent source oracle is the HCEA corpus at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. The exact donor file blobs and
payload hashes, the January split-object hash, and the frozen one-shot compiler
input are recorded in
`docs/object_matching_logs/draw_string_obj_jonas_hcea_one_shot_20260828.md`.
This wave re-audited that complete donor set under
`work/research-cache-halocea`, the frozen
`scratch/_gate_28180.c` one-shot, January function/relocation topology, the
current headers, and the current tree's object-matching documentation. No
current `CLAUDE.md` or `AGENTS.md` exists in this checkout.

The stable January-to-semantic map is now applied both in source and in
`config/symbols.json`:

| January name | Recovered private name |
| --- | --- |
| `_code_0018ac10` | `_text_bounds_draw_character` |
| `_code_0018ac80` | `_text_pick_draw_character` |
| `_code_0018b160` | `_bitmap_draw_character` |
| `_code_0018b510` | `_styled_font_get` |
| `_code_0018b580` | `_parse_string_new` |
| `_code_0018b680` | `_parse_string` |
| `_code_0018b8f0` | `_parse_unicode_string` |
| `_code_0018ba00` | `_draw_string_partial` |
| `_code_0018bc10` | `_draw_unicode_string_partial` |
| `_bss_004c1908` | `_draw_string_globals` |

The aggregate at `_draw_string_globals` remains one 0xF8-byte storage owner.
Its software-bitmap, text-pick, text-bounds, and draw-state fields are typed at
their proven offsets; the unused two-byte gap and trailing 0x8C-byte
multitexture parameter region remain non-owning fields in the same aggregate.
No storage was split, duplicated, or moved.

The public formatting enums, callback type, and declarations for the narrow,
Unicode, bounds, pick, and bitmap entry points now live in
`source/text/draw_string.h`. Private helper definitions and prototypes remain
in `draw_string.c`. `bitmap_format_get_bits_per_pixel` is obtained from its
owner `bitmaps/bitmaps.h`, and `tolower` from the CRT `ctype.h`; there are no
consumer-local external prototypes.

## Source-authentic implementation decisions

The reconstructed parser and layout code follows HCEA's semantic control flow:
style, color, and justification escapes; localized word-boundary patterns;
narrow and Unicode parsing; font selection; glyph measurement; horizontal and
vertical wrapping; tab columns; indentation; clipping; highlighting; callback
rendering; bounds accumulation; and text picking. Correct enum constants are
used in switches. Draw flags use `TEST_FLAG`, and pixel/color values use the
project's `pixel32` and `real_argb_color` types.

The software bitmap callback implements the five January-supported formats,
uses `MIN`/`MAX` where appropriate, preserves the target assertion provenance,
and reaches the target's 25-relocation count through natural direct aggregate
field access. Bounded variants tested destination pixel width, row and column
types, cached versus direct bitmap access, blend-expression organization, and
the R5G6B5 packing operands. The retained form is the closest source-authentic
ordinary C result.

`_bitmap_draw_string` deliberately does not reproduce the rejected donor's
absolute-VA-zero/null-bounds behavior. When no bounds are supplied, the
retained safe C constructs the full bitmap rectangle. Its format validation,
clip intersection, callback installation, and draw call are otherwise the
recovered behavior. Exactness here remains blocked until authoritative source
explains a safe expression that naturally produces the January code.

For the parser, partial renderers, and top-level layout loops, bounded trials
covered result widths, branch order, escape-loop `continue` forms, local
lifetimes, line-top expressions, font/style lookup forms, and tab-count
handling. Once authentic variants stopped improving the gate, the functions
were parked. No fake store, volatile or register forcing, pragma, optimizer
barrier, raw address, representation pun, undefined behavior, forced inline,
nonsensical branch, gameplay assembly, or comparator exception is retained.

All functions use the campaign's multi-line parameter format, void parameter
lists name `void`, and void functions end with explicit `return;`. No
`point_from_line3d` COMDAT is emitted by `draw_string.obj`.

## Shared-header blast and verification

The direct includers of `text/draw_string.h` were gated before and after the
header change. Their verdict counts are unchanged; the final counts are:

| Translation unit | Exact | Residual | Unwritten |
| --- | ---: | ---: | ---: |
| `source/ai/ai_debug` | 56 | 4 | 0 |
| `source/interface/hud` | 19 | 3 | 0 |
| `source/cutscene/recorded_animations` | 15 | 1 | 0 |
| `source/cutscene/cinematics` | 16 | 1 | 0 |
| `source/interface/interface` | 6 | 0 | 12 |
| `source/game/game_engine` | 169 | 11 | 0 |
| `source/physics/collision_usage` | 12 | 2 | 0 |

The required Units sentinel remains 189 exact, zero residual, zero unwritten.
The following final checks pass:

```text
python tools/campaign/gate.py source/text/draw_string --all
  exact 18, residual 10, unwritten 0

ninja all_source
  success

ninja progress
  734328 / 2198102 displayed code bytes
  5364 / 11060 displayed functions
  141 validated parked compiler ties

python -m tools.parked_functions
  active 141, stale 0, invalid 0

python -m tools.campaign.stable_verdicts diff <clean-f9ad12e6b> <candidate>
  gained 5, 1024 padded bytes; regressions 0

python -m pytest -q --basetemp scratch/pytest-temp
  261 passed

git diff --check
  clean
```

The first unconstrained pytest invocation could not access the host-global
`AppData/Local/Temp/pytest-of-isabe` directory under the workspace sandbox;
rerunning the identical suite with a workspace-local `--basetemp` passed all
261 tests. This was an environment setup error, not a test failure.

Only `source/text/draw_string.c`, `source/text/draw_string.h`, the semantic
symbol map, the ten measured parked records, and this ledger are tracked by
this wave. The neighboring bitmap subsystem source files, build policy,
compiler flags, comparator, and object status labels are untouched.
