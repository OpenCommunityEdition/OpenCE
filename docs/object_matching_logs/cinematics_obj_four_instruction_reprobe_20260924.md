# `cinematics.obj` four-instruction reprobe (2026-09-24)

Base: canonical `4d1ebf179023f46efb77929fb599e514601152fb`.

The 17 January functions are represented in source. A fresh VC7 gate still
reports 16 exact and one residual. `_cinematic_render` remains 1,280 padded
bytes with all 57 relocation targets and addresses matching January. The only
normalized code difference is the already documented sequence at +0x45d:

```
January: shl eax,24; and ecx,0xffffff; or eax,ecx; push eax
Current: and ecx,0xffffff; shl eax,24; or ecx,eax; push ecx
```

This pass tested additional coherent source forms: a separately declared
alpha/RGB pair, a separately accumulated packed color, assignment back into
the alpha local, two compound assignments, an unsigned alpha local, and
reversed mask-operand order. Every candidate compiled to the same four
instructions as the current source. All trials were reverted; there is no
source or configuration change and no new matching credit.

A January split-object census of the exact `shift 24; mask 0xffffff; or`
idiom found this shift-first schedule only in `_cinematic_render`. Three other
January donors (`_hud_draw_meter`, `_modulate_pixel32_by_real_alpha`,
`_build_sprite`) emit mask-first schedules. That weakens a generic
source-idiom explanation; none of those donors supplies a natural spelling
that changes this translation unit's result.

## Independent cross-build and compiler checks

The October 12, 2001 prototype's `2276betaP.xbe` (SHA-256
`a3402b021833dd2a3c368786f239480d68a6394bdf540569fab18340a83ab827`)
contains the same x87 conversion and exact January shift-first sequence at
raw offset `0x83581`. The match includes the preceding stores and loads, not
just the four generic bitwise instructions. Its NB10 record names
`c:\halo\objects\halobetacache\cachebeta.pdb`, but the matching October PDB
is not present locally. This proves the sequence predates January by months;
it is not a one-build compiler or splitter accident.

January's PDB compiland record for `cinematics.obj` reports C, VC7 frontend
and backend 13.0.9254 QFE 0, no debug information, no LTCG and no `/GS`.
The available HCEX PDB records only the two `rectangle2d bounds` aggregates
and one `real_argb_color text_color` local for its later version of this
function; it does not reveal the scalar alpha's original lifetime or source
expression. Its `rasterizer_text_set_shadow_color` parameter is `unsigned
long`, consistent with the current `pixel32` declaration.

A bounded whole-TU flag sweep retained all 16 exact siblings but did not
change the residual under `/QIfist`, `/Ox`, `/Ot`, `/Ob1`, or `/Ob2`. `/Ow`
regressed five siblings and changed the render size/relocations; `/Op`
regressed one and changed the render size/relocations; `/O1` and `/Os`
regressed all 17. A separate local VC7 13.00.9210 compiler also produced
16/17 and the same four instructions from this source. Compiler mode or this
older VC7 build does not supply a clean resolution.

The separate `_fast_ftol` inline-COMDAT ownership issue remains as documented
in `cinematics_obj_jonas_authenticated_fast_ftol_reopen_20260831.md`. The later
folded-header-helper exception does not itself close the object: its caller
must first become strict exact, and the selected-provider link must pass.

Keep the current, coherent fuzzy body at zero credit. Reopen on a new
first-party January source/local record, or a same-compiler donor plus a
minimal source form that explains the shift-first OR accumulator without
manual assembly, barriers, invented lifetimes, or a policy-only match.
