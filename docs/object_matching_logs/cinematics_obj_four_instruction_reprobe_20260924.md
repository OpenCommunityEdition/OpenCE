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

The separate `_fast_ftol` inline-COMDAT ownership issue remains as documented
in `cinematics_obj_jonas_authenticated_fast_ftol_reopen_20260831.md`. The later
folded-header-helper exception does not itself close the object: its caller
must first become strict exact, and the selected-provider link must pass.

Keep the current, coherent fuzzy body at zero credit. Reopen on a new
first-party January source/local record, or a same-compiler donor plus a
minimal source form that explains the shift-first OR accumulator without
manual assembly, barriers, invented lifetimes, or a policy-only match.
