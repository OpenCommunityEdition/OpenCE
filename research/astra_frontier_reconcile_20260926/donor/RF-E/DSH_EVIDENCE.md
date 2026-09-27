# RF-E evidence: _display_scenario_help EXACT candidate (written 2026-09-26 14:56:13 -0700)

Unit source/interface/ui_widget.c, lane claude/remaining-frontier-20260926 @ b62f74c1 (base canonical 8cda1f91).
Meaningful 596 B / padded 608 B (row was [sha]: string_index in EDI, January EBX).

## Patch (display_scenario_help.patch, git apply --check clean)
One line: the vassert expression `text_box` becomes `text_box && text_box->type == _ui_widget_type_text_box`
(file string, line 2438 and the message "expected text box widget in player help screen" unchanged).

## Evidence for the change (first-party)
- /Od halo_cache_symbols.exe fn 0x65fc70 (read with od_dis.py): the same function (its assert/message/error strings
  at 0x98b98c "string_index>=0", 0x98b9a0 "expected text box widget in player help screen", 0x98b9d8, 0x98ba04,
  0x98ba50 equal January's). After the text-box search its assert block is
  `cmp [text_box],0; je assert; movsx eax,[text_box+0xe]; cmp eax,1; je ok; push 1; push 0xaa0; ...display_assert;
  push -1; system_exit` = `vassert(text_box && text_box->type == _ui_widget_type_text_box, ...)`.
  Its scenario-name lookup is a later rewrite (table) - that part is NOT used.
- January's own bytes are unchanged by the change (the second test is removed after register allocation on the
  found path), so the expression is invisible in January's code and .rdata (vassert does not stringify).
- The conjunct is redundant with the search loop's exit condition; precedent: the landed
  _widget_instance_text_box_is_focused keeps an /Od-attested redundant conjunct (`&& ancestor->focused_child ==
  parent`). Disclose for review.

## Mechanism (allocator oracle, tools/oracle copy of the Lane A / fifty-objects instrument)
- Lane source: string_index web (id 7) pri -22 pos 4 ties the const-1 web (id 2: TRUE of both asserts + the
  type compare) at -22 pos 0; string_index pops first, allowed {edi, ebx} (esi taken) -> EDI.
- Forced string_index->EBX: STRICT EXACT (const-1 then takes EDI, never emitted) (card DSH-D0).
- Aug-15-2001 DEBUG build (1749betaP.xbe + cachebeta.map, earlier revision without the local-player call) also puts
  string_index in EBX; our compile of that shape still picks EDI (card DSH-X1) -> the discriminating fact is in the
  shared assert/text-box part, not the local-player part.
- With the /Od assert: const-1 gains a 1x2 term (-20), string_index one more live-through block (-24); const-1 pops
  first -> EDI; string_index -> EBX = January.

## Verification
- gate --all: 97 exact / 5 residual / 0 unwritten (lane head 96/6/0); logs/DSH1b_gate_all.txt.
- keyed diff vs build/base/source/interface/ui_widget.obj: 329 -> 329 sections, CHANGED only _display_scenario_help
  608 -> 608 [EXACT vs January], 0 added, 0 removed (logs/DSH1b_keyed_vs_build_base.txt). Zero exact losses; no
  data/literal/COMDAT section moved.
- /W3 /Zs: 17 vs 17, identical warning multiset (logs/W3_lane_head.txt, logs/W3_dsh1b.txt).
- fake_match_scan: 0 review leads (logs/DSH1b_fake_scan.txt; lane head also 0).
- Retail witness (calibrated retail config): unchanged SEP MATCH + OCT verbatim (asserts compiled out).
- Hashes: dsh1b.c c07ab139..., dsh1b.obj 642e6172..., display_scenario_help.patch c49936fb....

## Strip test
- B (assert expression) alone: EXACT. Strip B -> lane source (residual [sha]).
- A (/Od loop header `text_box && text_box->type != ...` with empty body) alone: byte-identical to the lane source
  (inert). A+B: EXACT and byte-identical to B alone. A is therefore stripped from the candidate; the A+B form is kept
  as display_scenario_help_AB_alternative.patch (also /Od-faithful) for the owner's style choice.
