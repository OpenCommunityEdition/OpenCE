# RF-L report: `_widget_instance_render_recursive` (743 meaningful / 752 padded): EXACT candidate (saved by the lead)

Patch-only; nothing committed; no ninja; lane source/config/tools/build untouched; compiler hashes unchanged (C2
9dbf908b, C1 02843d31, CL 483e00c4). Patch `render_recursive_L1_exact.patch` (`git apply --check` clean on HEAD;
applied to a pristine HEAD copy it reproduces `scratch\rf\workers\RF-L\src\l1.c` byte for byte) = RF-E's RR2 (changes A
and B, kept) plus change C:
- C: the inline helper `widget_instance_get_cumulative_alpha_modifier` in the shape of /Od 0x668190:
  `real alpha_modifier = widget->alpha_modifier; widget = widget->parent; while (widget) { alpha_modifier *=
  widget->alpha_modifier; widget = widget->parent; } return alpha_modifier;`. Evidence: /Od has one local only
  ([ebp-4]); the parameter home [ebp+8] is loaded with `widget->parent` at +0x18..+0x1e and stepped at +0x39..+0x3f;
  the loop is a `while` (init falls straight into the test). The old separate `parent` local is attested nowhere.
  January: the standalone helper (32 B) and the four other inline sites (`render_text_box`, `render_spinner_list` x2,
  `render_column_list`) stay byte-identical; only render_recursive changes, to EXACT.
- Caveat carried from RF-E: in A the value -1 is attested, the `0xFFFFFFFF` spelling is not.

## Checks
| Check | Result | Log / card |
|---|---|---|
| gate --all | 98/4/0 (HEAD and RR2 97/5/0) | logs\L1_gate_all.txt |
| keyed diff vs build/base | 329 -> 329; only render_recursive changed (752 -> 752, EXACT vs January); 0 added, 0 removed, 0 losses | logs\L1_keyed_vs_build_base.txt |
| /W3 | 17 = 17, same warnings (lines shifted +3/+2) | logs\W3_lane_head.txt, W3_rr2.txt, W3_l1.txt |
| fake_match_scan | 0 (HEAD 0) | logs\L1_fake_scan.txt |
| strip test | every change carries bytes: C alone (S-1), A+C (S-2), B+C (S-3) each fix +0x78 but leave frame hunks; A+B (RR2) leaves only +0x78; A+B+C exact; in every variant only render_recursive changes | cards\L-S.txt |

Disclosure (house rules 21, 36): C's byte effect runs through C2's value-numbering ordinal (mod 4); chosen from /Od
evidence and predicted in the card before compiling (P(EXACT) 30%); not count filler.

## 1. Survey (SURVEY.txt; sort readouts logs\survey_sorts.txt); January = ours in every EXACT row
| Function (line) | Emitted | Order | Sorted ADD inputs (key) |
|---|---|---|---|
| ui_widget_delete (2209) | `mov esi,[ebx+0x58]; add esi,[ebp-4]` | pointer is the mov | [SR t_1c6 0x17180, load 0x14008] |
| event_handler_dispatch (3394) | `mov eax,[edi+0x2d8]; add eax,esi` | pointer is the mov | [SR 0x1e840, load 0x1c008] |
| load_children_recursive (3485) | `mov al,[ebx+ecx+0x30]` + `add ebx,ecx` | base = pointer | [SR 0x19680, load 0x14008]; fold is a backend rematerialisation (ctor 0x10758346) |
| widget_instance_initialize (3631) | `mov eax,[edi+0x58]; add eax,ebx` | pointer is the mov | [load 0x1c008, SR 0x16d80] |
| render_text_box (4918) | `mov edi,[esi+0x64]; add edi,[ebp-4]` | pointer is the mov | [load 0x18008, SR 0x17980] |
| render_spinner_list (5111) | `mov edi,[esi+0x64]; add edi,[ebp-0x14]` | pointer is the mov | [load 0x1c008, SR 0x19e40] |
| process_one_event 5797/5814 | whole pointer strength-reduced | no base/index | - |
| process_one_event 6084 | `mov eax,[ecx+0x58]; add eax,esi` | pointer is the mov | - |
| render_recursive (5238), RR2 | ours `[eax+ecx]`, January `[ecx+eax]` | ours base = pointer; January base = offset | RR2 [load 0x1c008, SR 0x19300]; L-1 [SR 0x19500, load 0x14008] |

Same-TU controls: `text[length] = 0` in text_box/spinner folds as `[2*length + pointer]` (base = offset). Board census of
strict-exact functions (boardscan.py): 44 base=offset vs 17 base=pointer: context decides. In multi-use loops the
pointer is always the `mov` regardless of sort order (the load must go to a register).

## 2. Decoded rule (DECODE.txt)
The operand is built before the backend marker by C2's address builder 0x1071c576 (ADD case 0x1071c71a). The common
path 0x1071ca52 first turns a memory input into a same-slot temp (0x1071e15d); 0x1071ca76 sets base = sorted input 1,
index = input 2; the memory-entry constructor 0x107157db (called at 0x1071cc02) writes base to +0x34, index to +0x38;
the backend only re-points the index to the reload temp. Swap exception (0x1071c9ff): only when input 1 is a memory
load and input 2 a temp, and the next reader of the ADD result (0x1070221e) is an assign (0x261) or compare (0x285);
ours reads it with ARG 0x25e, so no swap. Input order: the last sort before lowering (0x107105ab; earlier 0x1070d41d
sorts are overridden since keys are recomputed), descending, comparator 0x10710ed9. Key of the strength-reduced IV
temp = `(id & 0x3ff) << 6`; key of the load `[t_base+0]` = `8 + ((id(t_base) & 3) << 14)` (the E1 worker found the
same `& 3 << 14` law independently). Symbol ids are preset per 32-record slab from counter [0x1088b648] (reset per
function at 0x10752857), so id mod 4 = the value-numbering ordinal mod 4; the VN table insert 0x10702896 creates one
temp per distinct expression. In RR2 `definition + 0x4c` is VN temp 0x183 (ordinal 35, &3 = 3): load key 0xc008 beats
IV key 0x9300. January needs that ordinal not 3 (mod 4), or the swap. Validation: forced swap (L-D1) and forced load
key at the last sort (L-D2b) each give January's section exactly with no other function changed; unforced controls
RR2 (base = pointer) and text_box `text[length]` (IV-style key 0x152c0 > load 0x14008, base = offset) match the rule;
L-1 observed ordinal 29 (predicted 29 or 33), &3 = 1, load key 0x14008 < IV key 0x19500, so base = IV.

## 3. Source facts that could flip it
| Fact | Effect | Status |
|---|---|---|
| helper param-reuse `while` shape (/Od 0x668190) | loop's widget+0x24/+0x30 VN entries reuse the first accesses' entries: -6 temps | attested; compiled; EXACT |
| named local or test on `input->function` (swap route) | would flip | not attested (/Od pushes the load directly); not compiled |
| >= 181 more VN temps (IV key > 768) | would flip | implausible, not attested |
| dead `[ebp-0xc] = widget->parent` local (/Od 2020) | predicted no net change | held form (dead store); not compiled |
| `render_children = TRUE` before `alpha_modifier` (/Od frame order) | constant assigns create no VN temps; predicted inert | not compiled |
| HCEX `short function` parameter | no IL change; predicted inert | not compiled |
| `input_index + (T *)address` | VN visits by subtree level after the sort; inert by inference | not attested; not compiled |

## 4. Cards (cards\): 7 card-driven compiles of 30; traced runs of unchanged sources in TRACES.txt (all stock-equal)
L-D1 forced swap (diagnostic): EXACT. L-D2 forced key at 0x1070d41d: failed (undone by the 0x107105ab re-sort). L-D2b
forced key at 0x107105ab: EXACT. L-1 candidate C on RR2: EXACT. L-1V traced run: ordinal 29 as predicted. L-S strip
tests: all held. (L-1's outcome text had its inline-site list corrected in place; disclosed in the card.)

## 5. Failed predictions and corrections
L-D2 failed; the multi-use `mov` was first read as sorted input 1 (wrong); `load_children`'s fold first assumed to be the
same builder (it is a backend rematerialisation); C first leaned inert (card: 45% inert). Inconclusive controls:
ai_handle_spatial_effect (keys 0 at builder time); ai_conversation_status and recorded_animations_update (no
memory-entry build seen). The census classifier mislabels some rows. RF-E's "decided later in address formation" is
refined: the deciding sort is the post-strength-reduction re-sort with VN temp keys (already in RF-E's
rr2_sorts.txt:1784).

## 6. Reopen criterion (only if C is rejected)
Another first-party fact making the VN ordinal of `definition + 0x4c` 0-2 (mod 4) in the prologue, or an attested
assign/compare reading `input->function`; check with sortfinal.py / vnjoin.py before compiling.

Files: SURVEY.txt, DECODE.txt, EVIDENCE.txt, TRACES.txt, cards\, logs\, the patch; tools and scratch sources in
`scratch\rf\workers\RF-L\` (`tools\rfl.py` is the stock-equal debugger harness).

## Lead verification and landing form (2026-09-26)
Independent /Od readouts (`research/remaining_frontier_20260926/lead/od_66a020_full.txt` and the commands below):
- 0x668190 (helper): one local [ebp-4] = widget->alpha_modifier; `mov edx,[ecx+0x30]; mov [ebp+8],edx` (param reuse);
  test at +0x21, body multiplies and steps [ebp+8], `jmp` back to the test: exactly C.
- 0x66a020 +0x220..+0x284 (clip): `cmp [ebp-0x3c],0; je`; copy 8 bytes to [ebp-0x48]; `lea ecx,[ebp-0x48]; mov
  [ebp-0x3c],ecx`; then +2 += offset.x, +6 += offset.x, +0 += offset.y, +4 += offset.y through the pointer: exactly B
  (x0, x1, y0, y1).
- +0x31d..+0x332 (color): `push alpha; push -1; call 0x40a6dc` (-> 0x6627d0 = modulate_pixel32_by_real_alpha); the
  result is stored to the local [ebp-0x24] BEFORE the 0x8c-byte parameter block init, and the draw call pushes
  [ebp-0x24]. So the /Od build has a NAMED color local, not a nested argument. The landed form uses `pixel32 color;`
  assigned right after the flash block; it is byte-identical to the nested L1 (VC7 forwards the single-use local;
  keyed diff L1 vs named form 0/0/0). The name `color` is descriptive (house rule 15; RTC names aggregates only).
- RTC descriptor 0x66a65c (5 vars): bounds [ebp-0x34] 8, local_clip [ebp-0x48] 8, multitexture_params [ebp-0xf0]
  140, widgets_were_deleted [ebp-0x101] 1, null_event [ebp-0x114] 8. The landed form renames our `clipped` ->
  `local_clip` and `parameters` -> `multitexture_params` (house rule 15; byte-inert: keyed diff 0/0/0; the second name
  also matches draw_bitmap_in_rect's parameter at ui_widget.c:1655). widgets_were_deleted and null_event belong to
  2020-only code absent from January's exact function.
- Lane gate 98/4/0, `EXACT 752 _widget_instance_render_recursive`; keyed diff vs build/base 329 -> 329, 1 changed
  (EXACT vs January), 0 added, 0 removed; diff --check clean. Full clean-build checkpoint R2 pending.
