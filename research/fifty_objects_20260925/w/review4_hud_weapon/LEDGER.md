# review4_hud_weapon - adversarial review of P5 (hud_weapon.c shared stack frame), wave 4 - LEDGER

Tree 16542e46, git status clean at start. Worker slug scratch/w/hud_stack_check/ (P5 depends on P1; packet P1..P7).

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log (held table: hud_unit row = this packet's reopen path; damage A / hs
  real_random_range = P1 class "new header COMDAT from a non-exact caller"), worker LEDGER (all sections), sibling
  reviews review4_hud_draw (approve=false as submitted; amended set P1+P2+(P3|01RS)+P5+P7, P6 excluded) and
  review4_hud_nav_points (P4 approved inside the packet). parked.json: no hud_weapon entry. rejections: no hud_weapon entry.

## V1 patch application (git apply --check on 16542e46)
- P5 alone CLEAN; P1+P5 CLEAN; {P1,P2,P3,P5,P7} CLEAN; {P1,P2,01RS,P4,P5,P6,P7} CLEAN.
- My own replay (git show HEAD: blobs + patch -p1 P5,P1,P2) == worker cand hud_weapon.c / hud_draw.h / hud_draw.c (EOL-normalised).

## V2 gate (own TU copies in gtu/, include redirected; own raw comparator rawcmp.py: bytes, relocs w/ $L normalised
   by value, flags, COMDAT selection, full symbol table)
- prod / ctrl (verbatim header copy) / cand (P5+P1 header) / sub (prod TU + P1 header): all 13 EXACT / 3 residual
  (_crosshairs_draw relocs 77!=76; _hud_update_weapon_local_player size 1456!=1472; _render_weapon_hud size 2624!=2656).
- rawcmp prod vs ctrl: 0 (harness inert). prod vs cand: ONLY +_check_stack_buffer section + external symbol.
  prod vs sub (P1 without P5): DIFF _render_weapon_hud [bytes] -> worker's A5 reproduced: P5 is required once P1 lands.
- Inline callers in hud_weapon: _render_grenade_hud EXACT, _hud_update_weapon EXACT; _hud_update_weapon_local_player,
  _crosshairs_draw, _render_weapon_hud residual (unparked).

## V3 residual callers inline identically to January (sentinel_tail.py / tail_dis.py, read-only)
- Exact callers _render_grenade_hud / _hud_update_weapon: sentinel tail (anchor mov eax,7Fh; mov ecx,62626262h .. 2nd
  system_exit) bytes SAME, reloc names SAME.
- Residual callers _hud_update_weapon_local_player / _crosshairs_draw / _render_weapon_hud: same inlined shape as
  January (loop, or esi,-1 merge, return-eip cmp, two assert pairs with the site's line); differences are only loop
  alignment filler (lea ecx,[ecx] / mov edi,edi, placement shifts) and pre-existing frame offsets (crosshairs
  stack_buffer -0x318 vs -0x314, return_eip slot -0x6c/-0x68, -0x48/-0x4c). No site calls the helper out of line.

## V4 law (i) labs (labs/mklabs.py; LAB ONLY)
- L1 exact callers only use the inline (3 residual sites keep a lab-local goto copy): gate 13/3, rawcmp vs cand 0
  differences -> the COMDAT is still emitted, identical, and owned by the strictly exact callers
  _render_grenade_hud + _hud_update_weapon. (Also shows the goto copy and the inline give identical residual bytes.)
- L2 residual callers only: rawcmp vs cand 0 differences.
- L0 all five sites goto copy + P1 header + STACK_BUFFER_* names (no TU enum): identical to cand minus the COMDAT
  -> A5's drift comes from the TU's own duplicate HUD_WEAPON_STACK_BUFFER_* enum names (declaration count), which P5
  removes as genuine duplicates of the header constants.

## V5 STRIP TEST of the macro block scope
- ST1 (no braces in macro; `short corrupt_index;` declared after stack_buffer in each of the 5 functions) and ST2
  (declared first): gate 13/3, rawcmp vs cand 0 differences. The block scope is byte-inert: the macro buys nothing,
  its form rests on the F2 evidence (both asserts of every January site carry one line literal).

## V6 admission tools on cand.obj
- surplus (slug copy surplus_cand.py): _square_root, _power (pre-existing) + _check_stack_buffer vs January hud_draw: IDENTICAL.
- repo provider_link.py (production providers): _check_stack_buffer FAIL (LNK2005 both orders vs production NODUP
  hud_draw) - expected control; every other surplus PASS.
- slug plink.py: cand hud_weapon + my P1+P2 hud_draw.obj (gtu/hud_draw_cand.c; 22/0/1 same as prod; rawcmp vs prod:
  only _check_stack_buffer selection 1->2): PASS both orders. Control vs production hud_draw: FAIL both orders.
  => P5 is NOT independently landable: it needs P1 (compile) AND P2 (link).
- object_audit prod vs cand: text differs ONLY by `+.text sym:_check_stack_buffer size 48`; 23/23 January symbols ok;
  FAIL(3) = the 3 pre-existing residuals. All January data/rdata/bss sections ok (unchanged).
- pdb_storage: 0 disagreements; _check_stack_buffer PUBLIC in cachebeta (000C08F0) -> external non-static __inline is right.

## V7 /Od and warnings
- odbuild str "corrupt stack at" / "corrupt return address": no hits; odbuild file "interface\hud_weapon.c": 8 fns;
  rtc of 0x6363e0 / 0x6379d0 / 0x638150: no stack_buffer descriptor. The later build has no sentinel at all ->
  no /Od declaration order exists for the macro block (law (d) not applicable; strip test shows block byte-inert).
- /W3 /Zs (slug warn.py copy, tmp in slug): ctrl vs cand warning multisets IDENTICAL (1x C4146, 13x C4244).

## V8 consumer list + own shadow sweep (sweep.py; ctrl = verbatim header, min = P1 header + P2,P3,P5,P7)
- grep -rlE over .c/.h/.cpp/.inl (spaces-safe): 23 consumers, all `#include "interface/hud_draw.h"`, no header
  includes it; list == worker's consumers.txt.
- ctrl: 23/23 rawcmp 0 differences vs build/base (harness inert; build/base current).
- min: 20 objects 0 differences (models with P7: 0); hud_draw only _check_stack_buffer selection 1->2; hud_unit and
  hud_weapon only +ANY _check_stack_buffer. Gate summaries identical everywhere. sw_min/hud_weapon.obj == out/cand.obj.
- 3-object packet link (hud_weapon + hud_draw + hud_unit from sw_min): PASS forward and reverse.
- Only build/base definer of _check_stack_buffer/_get_return_eip today: hud_draw (so no other NODUP conflict).

## V9 pins
- Reviewed patch hashes (patches/SHA256SUMS.txt): P5 55cb5670..., P1 cd30e78e..., P2 798cc72d..., P3 7e0f2a10...,
  P7 cb4f34e3... = identical to review4_hud_draw's pinned copies. Tree git status clean after all probes.

## VERDICT
- P5 content: APPROVE, unchanged, inside the atomic packet {P1, P2, P3|01RS, P5, P7} (+P4 optional, P6 per the hud_draw review).
- Dependency statement corrected: P5 needs P1 (compile: macro + STACK_BUFFER_*) AND P2 (link: LNK2005 vs production NODUP
  hud_draw, both orders); P1 in turn needs P3/01RS (hud_unit redefinition error) and P7 (models count flip).
- Zero credit: hud_weapon stays 13/3; NOT an admission. Pre-existing hud_weapon admission blockers untouched (see REVIEW.md).
