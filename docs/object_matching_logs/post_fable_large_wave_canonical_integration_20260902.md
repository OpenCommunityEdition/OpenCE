# Post-Fable large-wave canonical integration (2026-09-02)

## Outcome

Starting from the fully reconciled Fable backlog at `5cd391421`, this wave
integrates four independently gated lanes:

| Lane | Result |
| --- | --- |
| `actors.obj` | Four complete typed functions moved from unwritten to honest residuals; no strict credit claimed |
| `ui_widget.obj` | One strict-exact function, 32 padded bytes, plus the complete `draw_bitmap_in_rect` residual |
| `hs_compile.obj` | Nine strict-exact functions, 1,584 padded bytes, plus the natural compiler/tokenizer/parser frontier |
| `sound_manager.obj` | Twelve strict-exact functions, 2,320 padded bytes, plus the natural refresh/audibility frontier |

The rename-stable whole-tree sweep records **22 new strict-exact functions and
3,936 padded bytes with zero regressions**. The campaign report, which also
credits verified semantic-exact sections, moves from 712,393 to **716,729 Halo
code bytes** and from 5,150 to **5,174 Halo functions**. The all-library view is
729,799 / 2,198,102 code bytes and 5,346 / 11,060 functions.

## Admission standard

The Actors functions remain ordinary typed C where register allocation,
instruction scheduling, or x87 stack scheduling prevents an honest strict
match. They are explicitly parked rather than altered with fake dependencies,
volatile or register steering, optimizer pragmas, invented branches, false
ownership, or gameplay assembly.

The HS symbol names come from target symbol-atlas and later-source evidence;
private parser/tokenizer functions and the compiler global no longer use
address-derived names. An initially exact boolean parser was rejected because
its C spelling read an uninitialized local on the error path; the retained
HCEA/later-PC spelling is a defined, honest residual. The target- and
HCEA-proven reversed `hs_compile` output call is retained with a documented
`BUG (original)` correction. The Sound Manager names and layouts are
corroborated by the HCEA source/PDB corpus, while January disassembly,
relocations, and exact gates remain the admission authority. Correctly private
Sound refresh helpers are not forced into production merely to obtain function
credit.

All new source follows the owner-header, typed-access, project-type, explicit
return, and parameter-formatting rules. The protected January inline schedule
is unchanged and no `point_from_line3d` COMDAT is introduced.

## Verification

- Fresh `configure.py`, split regeneration, full `ninja`, semantic report, and
  progress gate: pass with zero unit errors.
- Stable whole-tree diff from `5cd391421`: 22 gained / 3,936 padded bytes;
  zero regressions across 8,245 target functions.
- `source/units/units`: 189 exact / 0 residual / 0 unwritten.
- House-rule fake-match scan over all changed source/header files: zero review
  leads.
- Tool tests: 261 passed.
- Park manifest: 126 active / 0 stale / 0 invalid.
- `git diff --check`: pass.
