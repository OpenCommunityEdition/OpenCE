# `ui_widget.obj` evidence closeout (2026-09-19)

## Scope and baseline

- Unit: `source/interface/ui_widget.c`; January target:
  `build/split/source/interface/ui_widget.obj`.
- Compiler: repository VC7/XDK toolchain and unit flags from `build.ninja`.
- Baseline strict gate: **92 exact, 10 residual, 0 unwritten** out of 102
  listed functions. Final strict gate: **94 exact, 8 residual, 0 unwritten**.
  The gain is **924 meaningful / 944 padded code bytes** in two functions;
  no matching credit is claimed for a fuzzy body.
- The complete candidate object was also checked with the
  `_point_from_line3d` emitted-owner guard; this unit does not emit that COMDAT.

## Primary evidence searched

1. January `cachebeta.exe` / `cachebeta.pdb` and the existing IDA database.
   The PDB contains public names and RVAs for this unit, but no source locals,
   local scopes, line records, or function type records that resolve the ten
   residuals.
2. January target code, relocations, data owners, COMDAT order, and debug string
   line anchors.
3. Every locally preserved `ui_widget.c` worktree version under
   `C:\halo-worktrees`: 114 files reduced to 23 unique SHA-256 contents and
   recompiled against the current January split object. This found an old
   scratch-only exact render-loop pair that a prior review had rejected as an
   unauthenticated duplicate-assignment spelling. It was not admitted until
   the independent primary evidence below resolved that objection.
4. All preserved UI scratch candidates in the 100K and 150K Opus lanes were
   recompiled.  Their maximum is 91 exact because they predate the currently
   accepted exact function; none closes a residual.
5. The Bungie-built HCEA prototype's `HCEX.pdb` supplies original CodeView
   line records for `d:\projects\code\hcex\sources\interface\ui_widget.c`
   (source MD5 `ABA933F724B948D0BD139A9A1441C8E2`). In
   `render_ui_widgets_postgame`, the four eligibility arms occupy distinct
   source lines and code ranges (970/972, 976/978/980/982,
   987/990/992/994/996/998/1001). The corresponding main-loop ranges are
   likewise distinct at lines 1065/1068, 1070/1072, 1074/1076 and 1079.
   This is direct debug evidence that the repeated assignments are real
   branch-owned source statements, not fake byte shaping.
6. The HCEA executable's independently reconstructed control flow confirms
   the same `should_render` local and branch partition. This later build is a
   semantic witness only; admission rests on the PDB line records plus the
   January code and relocation identity.

## Exact render-loop pair

`render_ui_widgets` and `render_ui_widgets_postgame` now retain the source
branch topology proved by the CodeView records: initialize a local
`should_render` flag and assign it independently in the render-regardless,
error-dialog, unowned-first-slot, and owned-by-player arms. The main routine
uses a natural virtual-keyboard `if`/`else` and ends with the required explicit
`return;`.

The rebuilt bodies are strict exact:

| Function | Meaningful | Padded |
|---|---:|---:|
| `_render_ui_widgets` | 549 | 560 |
| `_render_ui_widgets_postgame` | 375 | 384 |

Both target relocation streams and normalized hashes match. The focused gate
preserves every prior exact sibling and passes the forbidden
`_point_from_line3d` emitted-owner guard. `tools/fake_match_scan.py` reports
zero review leads and `git diff --check` is clean. The before/after candidate
objects have the same 103 code owners. All 226 non-code, non-debug sections
are semantically identical, including owner sets, normalized bytes and
relocations; no data, BSS, COMMON, literal or other COMDAT owner moved.

## New retained fuzzy improvement

`widget_instance_text_box_is_focused` now uses the HCEA-corroborated nested
parent-chain `do/while` graph.  It has exact target size and relocation count,
and 26 of 29 instructions agree.  The only remaining difference is the legal
ordering of the boolean arms for the two authentic enum constants
`_ui_widget_type_spinner_list` and `_ui_widget_type_column_list`.

Two additional source-plausible spellings were measured and rejected:

- a `switch` with the two enum cases grows the comparison into signed range
  checks (`movsx`, `jl`, `jg`);
- a short-circuit ternary introduces an early return and `sete`.

Neither is closer than the retained ordinary boolean predicate.  This remains
fuzzy and receives zero exact credit.

## Full residual disposition

| Function | January / candidate padded bytes | Evidence result |
|---|---:|---|
| `_display_error` | 896 / 896 | Best authenticated structural candidate reproduces the switch/default/cross-jump graph.  The remaining difference is one constant-zero register web; no new source/type record exists. |
| `_display_scenario_help` | 608 / 608 | Instruction and relocation streams agree apart from EBX/EDI assignment of `string_index`; no local record exists in the stripped PDB. |
| `_draw_bitmap_in_rect` | 1168 / 1168 | Exact symbolic calls/relocations and frame; residual is VC7 scheduling of `map1_offset.y` versus the parameter-block address store. |
| `_ui_check_for_pause_game` | 672 / 640 production | The preserved source-authentic p1 graph reaches 672/50 (exact size and relocation count); only an ESI/EDI allocation tie remains.  It was not landed because it is still nonexact. |
| `_ui_widget_launch_widget` | 304 / 304 | Moving the definition to January's SCC/source position repairs the load sequence.  The remaining `NONE` parent-tag constant is pinned differently by VC7. |
| `_widget_instance_process_one_event_recursive` | 2016 / 1984 | January keeps distinct tab-to-previous tails; candidate jump threading removes one call relocation.  Preserved pe1-pe4 shapes do not close it. |
| `_widget_instance_render_recursive` | 752 / 752 | Same calls and relocations; frame-slot allocator keeps the `clip` phi in a register instead of the parameter home. |
| `_widget_instance_text_box_is_focused` | 64 / 64 | Retained best fuzzy graph above; three branch-target instructions remain different. |

## Admission result

Two exact functions were admitted for **924 meaningful / 944 padded bytes**,
with no regression. The text-focus production change is retained only as the
cleanest and most source-corroborated fuzzy body; it must not be presented as
exact. Reopening any remaining UI row requires new primary source/type/line
evidence or a measured VC7 rule that explains the named backend choice;
another spelling sweep is not evidence.
