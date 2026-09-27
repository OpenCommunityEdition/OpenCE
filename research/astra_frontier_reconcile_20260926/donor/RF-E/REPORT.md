# RF-E report: source/interface/ui_widget.c (saved by the lead)

Patch-only; nothing committed; no ninja; lane source not edited by the worker. Every experiment has a clock-stamped
card written before its compile. Cards, evidence, patches and logs here (`cards\`, `logs\`, `DSH_EVIDENCE.md`,
`PG_DB_EVIDENCE.md`); scratch copies, objects and tools under `scratch\rf\workers\RF-E\`. Baseline: copy of lane HEAD
b62f74c1 gates 96/6/0; keyed diff vs `build\base\...\ui_widget.obj` 0 changed.

**Result: `_display_scenario_help` EXACT candidate (596 meaningful / 608 padded). The other two stay NEGATIVE with a
narrower diagnosis and new witnesses.**

## 1. `_display_scenario_help`: EXACT candidate
- Patch `display_scenario_help.patch` (one line; `git apply --check` clean): the vassert expression `text_box` becomes
  `text_box && text_box->type == _ui_widget_type_text_box`; file string, line 2438 and message unchanged.
- Evidence: /Od fn 0x65fc70 (same function: its assert/message/error strings equal January's) tests
  `cmp [t],0; je assert; movsx; cmp 1; je ok` before the "expected text box widget in player help screen" message,
  i.e. `text_box && type == 1`. Its scenario-name lookup is a later rewrite (not used). `match_vassert` passes the
  message, not `#expr`, so January's code and .rdata cannot show the expression. The conjunct is redundant with the
  search loop's exit condition; precedent: the landed `_widget_instance_text_box_is_focused` keeps an /Od-attested
  redundant conjunct. Needs owner review.
- Mechanism (predicted before compiling; allocator oracle): `string_index` (priority -22) ties a constant-1 web (-22)
  and wins on position, pops first, takes EDI; forcing `string_index` to EBX gives exact bytes (card DSH-D0). The
  Aug-2001 debug build (earlier revision) also puts `string_index` in EBX while our compile of that shape gives EDI
  (DSH-X1), so the deciding fact is in the shared assert/text-box tail. With the /Od assert the constant-1 web rises
  to -20 and `string_index` falls to -24; the constant pops first and takes EDI (never emitted), leaving EBX for
  `string_index` as in January.
- Gate `--all` 97/5/0. Keyed diff vs build/base: 329 -> 329 sections, only `_display_scenario_help` changed (608 ->
  608, EXACT vs January); 0 added, 0 removed, 0 losses. /W3 17 before and after, identical. fake_match_scan 0 leads.
  Retail witness: still matches the Sept and Oct bodies (asserts compile out).
- Strip test: the assert expression alone is EXACT; stripping it returns the residual. The /Od loop header
  (`text_box && type != ...` with an empty body) is byte-inert alone, so it is stripped; loop header + assert is also
  EXACT and kept as `display_scenario_help_AB_alternative.patch` for the owner's style choice.

## 2. `_ui_check_for_pause_game`: NEGATIVE, narrowed to one allocator tie
- The best recorded form (H6) is one allocator decision away: the `local_player_count` web and the controller web tie
  at priority 47. The tie is broken by [W+0x44], traced (site 0x107277e8) to the position of the web's last
  definition: `count++` (0x44) beats `gamepad++` (0x2a), so the count web takes ESI. Forcing that decision gives exact
  bytes.
- New first-party witness: the Sept-2001 retail body at 0x4a3b70 has H6's shape and January's register choice.
  Compiling H6 in the retail configuration gives 624 B (Sept's extent) with only the same ESI/EDI swap; the retail
  oracle shows the same 47/47 tie; forcing it reproduces the Sept body verbatim.
- The tie is robust: lane source, recorded H1/H2/H4/H5/H6 and the 150K lane's q9 give identical priority terms.
  Two new tests inert: PG-D1 (handling inside the loop, q9) and PG-1 (explicit `(short)` casts on the two
  short-parameter calls, the Bungie idiom at `players.c:595`).
- Reopen: a first-party fact present in both January debug and Sept retail that shifts register pressure in one
  block of the controller's live range by one web; check with `tools/oracle` and `retail_oracle.py` before compiling.

## 3. `_draw_bitmap_in_rect`: NEGATIVE, mechanism located
- Scheduler traces: the plasma block's first scheduling region is capped at 81 records (79 emitted + 2 hidden temp
  records from the `*= 201.0f` statements). The y `fmul` waits on its hidden record (key 0x1c00), which loses the
  integer issue slot to the address stores (keys 0x2400-0x2c00) for 12 cycles; January's gating key must lie in
  (0x2400, 0x2c00]. January issues the `&map1_offset` store in the key-0x800 run of stores; ours carries 0x1400.
- New witness: the Oct-2001 retail body at 0xb0660 is Bungie's retail compile of January's source (January's debug
  body is verbatim in the Oct debug build). It differs from our retail compile in three ways (two parameter stores
  inside both branches instead of at the join; constant 1 not held in EBX; different vertex-loop registers), so retail
  confirms our source for this function differs from January's.
- Cards: DB-1 (duplicate trailing stores in both branches) worse; DB-2 (the three stores at their /Od positions)
  byte-inert; DB-3 (/Od parameter-store order) byte-inert; DB-4 (/Od four named offset temporaries) changes the
  schedule, does not reach January; DB-5 (named product inside the helper) breaks the exact
  `_compute_offset_coordinate`, rejected; DB-6 (DB-3 on DB-4) identical to DB-4; DB-X1 (diagnostic) refutes "one
  hidden record too many" as sufficient.
- Reopen: how C2 computes that hidden record's key, or a first-party fact fixing the three retail differences; check
  candidates against the Oct body with `retail/rw/octfind.py` first.

## Failed predictions
PG-D1 (expected a position lever; allocator saw identical input); DB-1/DB-2 retail (expected the Oct placement of the
two stores; the compiler sank them to the join); DB-3 (expected EXACT or improvement; byte-identical); DB-4 (EXACT
failed; partial schedule shift); DB-5 (expected the helper to stay exact; it lost exactness); DB-X1 (expected
January's y schedule; unchanged).

## Tools (scratch; read-only on the lane)
Oracle copy with `pblocks.py`, `popsdump.py`, `ilcap.py`, `postrace.py`, `retail_oracle.py`, `retail_pops.py`;
`schedsucc.py`; retail witness copy with `rcheck.py`, `octfind.py`; `sepdis.py`, `augdis.py`; `splice.py`, `hunks.py`,
`w3.py`.

## Lead verification (2026-09-26)
Independent /Od readout at 0x65fc70 (`scratch/rf/lead/lead_od_65fc70.txt`): search loop 0x65fdfc..0x65fe14
(`cmp [t],0; je; movsx [t+0xe]; cmp 1; je; jmp loop`), then the assert block 0x65fe16..0x65fe2c
(`cmp [t],0; je fail; movsx [t+0xe]; cmp 1; je ok`) before `push 1; push 0xaa0; push file; push 0x98b9a0;
call display_assert; int3; push -1; call system_exit`. The /Od assert line is 0xaa0 = 2720 (January 2438): a later
revision of the file, consistent with the rewritten scenario-name lookup; the assert expression is first-party but
from that later revision. Applied `display_scenario_help.patch` in the lane: gate `EXACT 608 _display_scenario_help`,
97 exact / 5 residual; keyed diff vs build/base 329 -> 329, 1 changed (EXACT vs January), 0 added, 0 removed. Full
clean-build checkpoint pending.
