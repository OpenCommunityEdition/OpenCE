# Lane B worker ledger: source/game/game

## Baseline (verbatim gate.py --all)

```
EXACT        64  _game_all_quiet
EXACT        16  _game_difficulty_level_get
EXACT        32  _game_difficulty_level_get_ignore_easy
EXACT        32  _game_difficulty_level_set
EXACT       192  _game_dispose
EXACT       192  _game_dispose_from_old_map
EXACT       160  _game_frame
EXACT        80  _game_initial_pulse
EXACT       272  _game_initialize
EXACT       352  _game_initialize_for_new_map
EXACT        16  _game_is_cooperative
EXACT       208  _game_load
EXACT        32  _game_map_loading_in_progress
EXACT        48  _game_options_new
EXACT        32  _game_options_verify
EXACT        16  _game_players_are_double_speed
EXACT       496  _game_precache_new_map
EXACT       320  _game_safe_to_save
EXACT        32  _game_safe_to_speak
EXACT        48  _game_set_game_engine_index
EXACT        48  _game_set_game_variant
residual     96  _game_set_game_variant_from_name  [size 48!=96, relocs 2!=4, sha]
EXACT        32  _game_set_players_are_double_speed
EXACT       272  _game_tick
EXACT       128  _game_unload
EXACT       176  _remove_quitting_players_from_game
EXACT        16  _set_random_seed
== exact 26  residual 1  unwritten 0  (of 27 listed)
```

## January decode of _game_set_game_variant_from_name (see aln_base_full.txt)

- frame 0xd0 = TWO 0x68 aggregates: [ebp-0xd0] return buffer handed to the callee, [ebp-0x68] the local copy.
- `lea edx,[ebp-0x68]` / `test edx,edx` : a SURVIVING null test of a frame address (setter parameter).
- copy `*eax -> [ebp-0x68]` is scheduled between the test and the `jne`.
- null arm: push 0x68 / push edx (known zero) / push offset _game_variant_global / call _csmemset, own epilogue (ret #1).
- install arm: `lea esi,[ebp-0x68]` (setter deref copy-propagated to the local) -> _game_variant_global, ret #2.
- ours (48 B): call, then rep movsd straight from *eax into _game_variant_global. No local, no test, no memset arm.

## Attempt 1-2 (diagnostic only, NOT admissible as-is: needs a header change)

Hypothesis: the callee is really a struct-by-value return (its exact body is the classic hidden-pointer
shape: temporary + result + `*variant = result; return variant;`), a dimension no prior ledger varied.
try_1.c: `struct game_variant variant= game_engine_get_variant_by_name(name);` (prototype shimmed by macro)
try_2.c: declared then assigned.
Gate (both): `residual     96  _game_set_game_variant_from_name  [size 48!=96, relocs 2!=4, sha]`
Verdict: by-value return alone folds to the same 48-byte fixed point. Rejected.

## Overturn census (new evidence): the surviving frame-address test is UNIQUE in January

`scratch/lane_b/w/game/scan_testlea.py` disassembles every code section of every object under build/split
(Halo + libs) and looks for `lea r,[ebp-N]|[esp+N]` followed within 12 instructions by `test r,r` on the
same unclobbered register. Result: exactly 1 hit in the whole image:
`game.obj _game_set_game_variant_from_name @0x1b lea edx,[ebp-0x68]`.
So there is NO exact-function donor anywhere in source/ for this mechanism.

## Lab probes 3-13 (scratch/lane_b/w/game/lab*.txt, compiled inside a copy of game.c with game.obj's own cflags)

All of the following fold to the same 48-byte body (no test, no second aggregate, no memset arm):
- lab_a  out-buffer local passed straight to the callee then `game_set_game_variant(&variant)`
- lab_b  named pointer local for the call result, copy, setter
- lab_c  `&game_engine_get_variant_by_name(name)` (by-value) -> C2102, not valid VC7 C
- lab_d  static helper taking `struct game_variant` BY VALUE, called with `*call(...)`
- lab_e  `struct game_variant variants[2]` (setter gets ADDR+0x68): frame 0xd0 but still folded (52 B real)
- lab_h1 by-value return fed to a by-value-parameter helper
- lab_h3 comma-expression argument `(variant= f(name), &variant)`
- lab_h4 pointer local initialised to `&variant`, assigned through, passed on
- lab_m1 INTRINSIC memcpy result (dest pointer) passed to the setter (diagnostic) -> folded too
- lab_f  `#pragma optimize("g", off)` (diagnostic) -> no inlining at all, calls _game_set_game_variant; variant at -0xd0
Measured law: VC7 C2 folds `ADDR(frame object) == 0` after copy propagation in every reachable spelling,
including opaque-looking producers (intrinsic result, by-value parameter temp, ADDR+offset).

## Diagnostic flag sweep (never admissible; mechanism hunt only) on the baseline source

/O1 52, /Os 52, /Ob1 64 (setter not inlined), /Ox /Oa /Ow /Op /Oi /G3 /G5 /GB /Zi /Z7 /Gy /GF /Gs /GX /GR /Ze /Oy /Og all 48.
No flag produces January's 96/4 shape; the difference is not a build-flag artefact.

## Cross-build evidence (new)

- October 2001 (2276betaP.xbe, file+0x978e0): byte-for-byte the SAME shape - frame 0xd0, `lea edx,[ebp-0x68]`,
  `test edx,edx`, copy, `jne`, memset arm with `push edx`, install arm `lea esi,[ebp-0x68]`. The construct is
  stable across two source snapshots three months apart => source-determined, not a one-off scheduling accident.
- HCEA (HCEX_release.exe, game_set_game_variant_from_name @0x83688628, PPC): ONE local; callee is
  `(name, &variant)`; then `addi r11,r1,0x50 ; cmplwi cr6,r11,0 ; bne` - the same never-true frame-address guard
  around `memset(&game_variant_global,0,0x98)` survives in a 2010-era MSVC PPC compiler. So the 360 compiler does
  not fold it in the plain lab_a form; VC7 x86 DOES (lab_a = 48 B). HCEA therefore corroborates the inlined
  setter + dead arm but says nothing about which VC7 spelling blocks the fold.

## Lab probes 14-22

- lab_i1 static by-value-returning wrapper inlined, lab_i2 static pointer-returning wrapper inlined -> 48 (folded)
- lab_v1 `volatile` local (diagnostic, inadmissible): frame 0xd0, both copies retained, layout -0xd0 buffer /
  -0x68 variant exactly like January, but the null test is STILL folded (no memset arm, 63 B real).
- lab_k1 value of an aggregate assignment decayed through an array member -> 48 (folded)
- setter spellings S1 `== NULL`, S2 early return, S6 pointer-local: setter stays EXACT, from_name still 48/2.
  S3/S4/S5 (`if (variant)` first) break the setter itself ([sha]) and still fold from_name.

## Lab probes 23-30

- lab_w1 by-value call result, array-member decay straight into the setter -> nested shape (test eax, ONE aggregate, 68 B real)
- lab_w2 same through an inlined static by-value wrapper -> 48 (folded)
- lab_c1 local of a different same-size struct type (pointer type mismatch at the call) -> 48
- lab_c2 / lab_c3 address laundered through unsigned long / a long local (diagnostic, inadmissible) -> 48
- lab_o1 old-style (unprototyped) inlinee definition (diagnostic) -> 48
- /Ob1 diagnostic (setter NOT inlined, 64 B): `call ; lea edx,[ebp-0x68] ; mov esi,eax ; mov ecx,0x1a ;
  lea edi,[ebp-0x68] ; push edx ; rep movsd ; call _game_set_game_variant`, frame 0xd0, buffer -0xd0 / variant -0x68.
  This is January's prologue, frame, aggregate layout and `lea edx` argument materialisation EXACTLY; January then
  has the setter BODY where this has `push edx ; call`. January therefore looks like the non-inlined function with
  the setter expanded AFTER the global optimiser ran (argument still an opaque register value, local copy still
  address-escaped), whereas every /O2 spelling we can write expands it BEFORE copy propagation and folds.

## Final state

candidate.c is byte-identical to source/game/game.c (no admissible change found).
gate --all: `== exact 26  residual 1  unwritten 0  (of 27 listed)`; guard _point_from_line3d PASS (no FORBIDDEN row);
unitcheck: NO CHANGE gains=0 regressions=0; newsyms: OWNERSHIP PASS, 0 newly defined; fake_match_scan: 0 leads.

## Specific blocker (byte ledger, January 92 real bytes vs ours 44)

| January bytes | decision | ours |
|---|---|---|
| +3 `sub esp,0xd0` (6 B vs 3 B) and +3 `lea ecx,[ebp-0xd0]` disp32 | second 0x68 aggregate survives | folded away |
| +3 `lea edx,[ebp-0x68]`, +2 `test edx,edx`, +2 `jne` | setter formal kept as an opaque register value | folded (ADDR==0) |
| +3 `lea edi,[ebp-0x68]`, +2 `rep movsd` | intermediate copy *eax -> variant retained | forwarded to the global copy |
| +2+1+5+5+3 memset arm, +2+1+1+1 = 6 second epilogue | null arm of the inlined setter | removed with the fold |
| +5 `mov ecx,0x1a`, +3 `lea esi,[ebp-0x68]` (vs our 2-byte `mov esi,eax` kept in the hoisted copy) | install copy reads the local | reads *eax directly |
Total +48 real bytes, all downstream of ONE decision: C2 did not fold `&variant == NULL` after inlining.

## W3 pass (gate.py --cflag=/W3 --cflag=/WX, diagnostic)

Only one warning located in game.c itself: line 715 C4013 `rasterizer_initialize_for_new_map` undefined (implicit int)
inside the already-EXACT `_game_initialize_for_new_map` (void, no arguments, so no double-promotion effect). Not in
this packet's function; fixing it needs a prototype in the rasterizer owner header (header packet, optional hygiene).
Nothing implicit is involved in `_game_set_game_variant_from_name`.
