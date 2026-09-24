# files_windows / _file_get_size worker ledger (lane claude/fifty-objects-20260925)

## Start (fresh run; slug dir did not exist)
- Baseline gate (production source): 26 exact / 1 residual (_file_get_size 224 [sha]) / 0 unwritten.
- Read: park entry (unclassified), small_family_wave_20260904, opus5_150k_w2_20260914.
- Recorded negatives (DO NOT REPEAT): initializer form (fixes statement order, leaves ESI/EBX file/info swap);
  result/else single exit; if(!result) single exit; partial January section order; full January definition order.
  Remaining per opus5: January file in ESI, info in EBX; ours file in EBX, info in ESI.

## P1 c1_init.c (initializer form; reproduce opus5 shape 1) -- EXPECTED/CONFIRMED
- 26/1; only diff: file in EBX (ours) vs ESI (Jan), info ESI (ours) vs EBX (Jan); prologue ours `push ebx; mov ebx,[ebp+8]; push esi; push edi`
  vs Jan `push ebx; push esi; mov esi,[ebp+8]; push edi`. Plus "size" string reloc naming (symbol vs defined-noncode; csplit naming, not a byte issue).

## /Od readout (halo_cache_symbols.exe fn 0x8aef00, files_windows_pc.c line 0x270) -> od_get_size.txt
- Frame (RTC-padded, decl order): info [ebp-8], full_path [ebp-0x10c] 256, result [ebp-0x111] (char), data [ebp-0x13c] 36.
- Statement order: info=get_info(file) initializer; full_path="" ; result=FALSE ; assert(size); get_full_path(info->location, info->path, full_path);
  if (GetFileAttributesExA(full_path, GetFileExInfoStandard, &data)) { *size = data.nFileSizeLow; result=TRUE; } if (!result) file_error(...); return result;
- Jan edi: rep stos clobbers edi/ecx, so file/info (live across memset) can only take ebx/esi; size (loaded after stos) takes edi. Residual = colouring order of {file, info}.

## P2 p2.fn: /Od-attested decl order (info, full_path, result=FALSE, attribute_data) + if(!result) single exit
- Hypothesis: result declared BETWEEN full_path and data (opus5 v3-v6 declared it after data) shifts IL ordinal tie-break.
- Result: 26/1, identical swap (file EBX, info ESI). NEGATIVE. (NB: Bash heredocs collapse "\\\\" -> use Write/Edit for .fn files.)

## Emission-order readout (secorder.py)
- Jan: is_valid compare_dates find_files_start add_name add_extension remove_name path_split get_full_path read_only file_error create delete exists rename open close get_pos set_pos get_eof set_eof read write read_from write_to get_last_mod get_size find_files_next
- Ours: definition order, with static file_error pulled just before its first caller file_open. => in Jan the first caller of file_error (or its definition point) is right before file_create.
- Hypothesis H-inl: our error path is the INLINED file_error(file,...) whose parameter copy adds a ref to `file` web (priority tie -> file wins ebx). January may use the explicit
  sibling error block (as exact file_create/file_delete/file_close/get_last_mod do): error_info = get_info(file); error(...); SetLastError(0).

## P3 p3.fn: explicit sibling error block (error_info local, no inlined file_error param copy) -- NEGATIVE (identical swap). H-inl refuted.

## KEY FINDING: January DEFINITION order recovered (deferral model, verified on ours)
- Model (verified on our own object): C2 compiles non-deferred functions in definition order; a function calling a not-yet-defined TU function is DEFERRED and
  compiled at TU end in definition order. Ours: open/get_size/exists/read/write/read_from/write_to deferred (call static file_error defined last) -> emitted last. Matches.
- Applying to January emission: non-deferred = is_valid compare find_start add_name add_ext remove_name split get_full_path read_only file_error;
  deferred = create delete exists rename open close get_pos set_pos get_eof set_eof read write read_from write_to get_last_mod get_size find_files_next.
- => January definition order: is_valid, compare, create, delete, exists, rename, open(308), close, get_pos, set_pos, get_eof, set_eof, read(423), write(451),
  read_from, write_to, get_last_mod, get_size(524), find_files_start(548), find_files_next, add_name(672), add_ext(696), remove_name, split, get_full_path(788), read_only, file_error.
- Assert-anchor spans agree (write 451 -> get_size 524 = 73 lines for rest-of-write+read_from+write_to+get_last_mod; get_size 524 -> find_start 548 = 24 lines; find_start->add_name 124 = find_files_next body).
- close/get_pos/set_pos/get_eof/set_eof are in the DEFERRED group but call no later public fn => in January they CALL file_error() (ours: explicit inline blocks; bytes identical after inlining).
- opus5 fw_v5 used the EMISSION order as definition order (different; inert). This order + file_error sibling calls is untested.

## P4 p4.c = c1_init + reord.py January definition order (explicit sibling error blocks kept)
- 26/1 exact kept. Emission: close/get_pos/set_pos/get_eof/set_eof NOT deferred (emitted before find_start) -> confirms they must call file_error in Jan. get_size swap unchanged.
## P5 p5.c = p4 + siblings.spec (create/delete/get_last_mod/close/get_pos/set_pos/get_eof/set_eof call file_error(file, "<name>"))
- 26/1 exact kept (all 8 siblings still EXACT via auto-inline). EMISSION ORDER NOW IDENTICAL TO JANUARY (27/27). Deferral model fully validated.
- get_size swap UNCHANGED (file EBX / info ESI). TU order + sibling spelling are not the lever for this tie (but p5 is a genuine structure recovery; keep as base).

## L1 (lab) TU name-count oracle on janorder.c: N=0..40 `extern int` names inserted just before file_get_size -> ALL residual (same row).
- > one period (K=22) swept: NOT a name-count tie. Do not repeat.

## Mini-lab (lab/mini.c: janorder TU with only file_get_size + file_error; reproduces identical residual)
- mA (lab: no error path): file not enregistered, info -> ESI (not EBX!). mB (lab: no assert): file -> ESI, info -> EDX. mC (lab: no *size store): same swap.
- => the FIRST-coloured callee-saved web takes ESI here (EBX is second choice). So in OURS info is coloured first (info prio > file prio);
  in JANUARY file is coloured first (file prio >= info). Need: more/heavier refs to `file` or fewer/lighter refs to `info` in January's source.
- mD (early-return error), mE (if(!Get) error/result=FALSE else success/result=TRUE): same swap.
- mF: `result = GetFileAttributesExA(...); if (result) {*size=...;} else {file_error(...);} return result;` -> ALLOCATION FLIPS TO JANUARY'S (file ESI, info EBX)!
  Remaining: result kept in BL (`mov bl,al; test bl,bl` / `mov al,bl`) vs Jan `test eax,eax` + constant returns (mov al,1 / xor al,al). So Jan is not mF literally,
  but a result web / IL shape around the call result is the lever.

## Allocator tracer (Lane A dbg32c/prio.py copied to lab/dbg, gencfg retargeted; runs stock CL under debugger, nothing patched on disk)
- m0 colouring (get_size): info W1fd8 pri17 -> ESI (cost tie ebx/esi -> esi wins by class order), file W1f48 pri13 -> EBX, error_info W1f90 pri12 -> ESI, size W1de0 pri11 -> EDI.
- mF: result W1f90 pri24 -> EBX first; info pri20 gets ESI cost +200 (file became a single-register neighbour: ebx excluded by result) -> info EBX; file ESI.
- => Jan needs EITHER file coloured before info (file prio > info prio) OR a conflicting web that forces file to single-register esi before info is coloured.
- Tracer mJ (=P2 /Od single-exit form) and mK (result/else): info pri19 -> ESI, file pri17 -> EBX (both ebx cost 400: a byte-register single-reg neighbour, i.e. the boolean result web).
  File priority rises 13->17 in single-exit forms but stays below info. Tie-break (pos) would favour file (pos 0x12 > info 0xe) if priorities were equal.
- Chooser order empirically: ESI before EBX on equal cost (VC esi/edi/ebx convention).
- Variants v01-v07 (lab/gen.py; all natural single-exit spellings: result=TRUE init + else FALSE; result assigned both arms; `!= 0`; if(!Get) arms; `?TRUE:FALSE`):
  constant-result forms (v01,v02,v04,v05,v07) keep info first (info 17-19 > file 15-17) -> same swap.
  call-valued result forms (v03 `!= 0`, v06 `?:`, mF) flip allocation (result pri24 -> EBX first, file single-reg ESI, info EBX) but keep result in BL
  (setne bl / mov al,bl) instead of Jan's test eax,eax + constant returns. None exact.
- v08/v09 (`else { result = FALSE; file_error(...); }`, sibling file_set_eof style): file NOT enregistered, `xor ebx,ebx` zero pin appears -> further from Jan. w01 (BOOL success + `?TRUE:FALSE` return), w02: same swap.
- Lab q1 (file_error twice): file pri 13 -> 35, file coloured first -> Jan allocation (lab only, not legit). q3 (explicit block) priorities identical to m0 (17/13/12/11).
- Lane A laws recalled: block weight tracks block pressure (inline params raise it); single-exit return split; "where an initialiser executes".
- r1/r2 (lab: no const casts / non-const param): same swap -> casts inert.

## KEY EVIDENCE: file_error parameter ORDER
- /Od PC build file_error (0x8ae8f0): file at [ebp+0xc], function_name at [ebp+8] => first-party signature file_error(function_name, file); callers push file then name.
- January _file_error (exact) uses a private register ABI: file arrives in EAX (`push esi; push eax; call get_info`), function_name at [ebp+8].
  => the (file, name) vs (name, file) order is INVISIBLE in _file_error's own bytes (either order puts name at [ebp+8] with file in eax).
- Hypothesis H-ord: January's file_error is (function_name, file) like the /Od build; inlined-parameter IL order changes block pressure/priorities (Lane A: inline params raise block pressure).
## P6 p6.c = janorder + file_error(function_name, file) everywhere (13 sites + def/proto): 26/1 kept (order invisible, confirmed); get_size swap unchanged.
- P6 + body sweep (m0 mD mJ mK v01-v05 v07 w02 v08 with (name,file) order): identical outcomes to (file,name) order -> param order inert for get_size.
- a2 (?: expression assert), a3 (do-while wrapped), a4 (if/else), a5 (halt noreturn-ish): same swap -> assert CFG form inert (lab).

## Priority accumulation decoded (lab/dbg/prio2.py: bps 0x1072a527/0x1072a54c = count x block-weight adds, + chooser 0x1072da96)
- m0: block weights entry=5, assert-fail=1, post-assert=2, error=3. info = -1 + 2*5 + 4*2 = 17; file = -3 + 2*5 + 2*3 = 13; size = -3 + 2*5 + 2*2 = 11; error_info = 4*3 = 12.
- For Jan colouring (file first) need file > info: 2*z_err - 4*y_post > 2 (same ref counts).
- C2 decode (0x1072aac5..0x1072ab3a): block weight w(B) = number of webs REFERENCED in B (x freq); a web live-through B without a reference is PENALISED by w(B).
  m0: w0=5, w_assertfail=1, w_post=2 {info, zero-const}, w_success=2, w_err=3 {file, error_info, zero}. info-file = 5*w_post - 2*w_err = 4.
  => January needs MORE webs referenced in the error block (w_err>=5 with w_post=2) or fewer in post-assert.
- /Od file_error (0x8ae8f0) stores GetLastError() into a LOCAL ([ebp-0xc]) after info ([ebp-8]) before building the message -> hypothesis H-ec:
  January file_error has `error_code = GetLastError()` local; when inlined it adds a web to the error block (raises w_err, file priority).

## BREAKTHROUGH (mini TU lab/mini7.c = p7 file_error with error_code local, (name,file) order)
- m0e (two returns): still residual (file 15 < info 17; w_err now 4).
- mJe (/Od single-exit: result=FALSE decl between full_path and data; if(Get){*size;result=TRUE;} if(!result) file_error; return result) -> EXACT 224.
- mKe (result/else) and v01e (result=TRUE init, else FALSE) -> EXACT too. w_err=5 in mJe -> file (20) coloured before info.

## P8 p8.c = p7 (janorder + siblings call file_error + (name,file) order + error_code local) + get_size /Od single-exit (p8.fn)
- FULL TU GATE: 27 exact / 0 residual / 0 unwritten. _file_get_size EXACT 224. (p8.gate.txt)
- Next: ablations to find the minimal, evidence-backed production diff.

## ABLATIONS (full TU gates)
- a1.c = PRODUCTION source + file_error `unsigned long error_code = GetLastError();` local + get_size /Od single-exit form -> 27/27 EXACT. MINIMAL DIFF.
- a3.c = janorder (reorder+sibling file_error calls) + same two changes -> 27/27 EXACT (reorder not needed).
- a4.c = production + get_size single-exit only (no error_code local) -> 26/1 (residual). => error_code local is load-bearing (via inlined IL web, not name count: file_error is defined AFTER get_size; name-count sweep negative).
- Corroboration (lift, not byte proof): Stian files.c file_error also holds XGetLastError() in a local `error` before DEBUG_LOG. First-party /Od (PC) file_error stores GetLastError() into a named local slot [ebp-0xc].
- a5.c = production + error_code local only (two-return get_size kept) -> 26/1 residual. BOTH changes are required together.

## FINAL CANDIDATE = a1.c (copied to files_windows.c; production.patch = unified diff vs source/tag_files/files_windows.c, `git apply --check` OK)
- gate --all --forbid-emitted-symbol _point_from_line3d: 27 exact / 0 residual / 0 unwritten.
- object_audit: PASS (55 January symbols, 0 differ; all January-owned sections ok). Production base audit was FAIL(1) (get_size only).
- pdb_storage: 0 disagreements. surplus_identity: 0 candidate-only code COMDATs. provider_link: SELECTED-PROVIDER LINK PASS (6 pre-existing surplus literals/D3D tables, same as production).
- All 34 non-.text sections byte+reloc identical to production build/base obj (report.json data already 1348/1348 = 100%).
- fake_match_scan: 0 leads. Diff touches only file_get_size body and static file_error (TU-private; no header change, no blast radius outside this object).
- objdiff-cli 3.3.1 (frozen scorer) on scratch project odproj/ (target=build/split obj, base=a1.obj): code 4062/4062 100%, data 1348/1348 100%, 27/27 functions.

## VERDICT: OBJECT_COMPLETE_CANDIDATE (files_windows.obj 27/27 + data 100%)
Integrator: apply scratch/w/files_windows/production.patch; retire config/parked.json park {source/tag_files/files_windows, _file_get_size};
full ninja + stable sweep (TU-private change only; no header/config edits).
Disclosure for owner: the load-bearing file_error `error_code` local is attested by the first-party /Od build (named local slot [ebp-0xc] holding
GetLastError() in file_error) and corroborated by the Stian lift; in the PC build it is also used by a later FormatMessage call. STRIP TEST: without the
local the object stays 26/1 (get_size residual) - it is a real, used local (not a dummy/decoration), mechanism = inlined-helper web raising the error-block
weight (C2 decode), not the name-count oracle (file_error is defined after get_size; N=0..40 sweep negative).
Optional, byte-inert, evidence-backed recoveries NOT needed for completion (kept in scratch): January definition order + sibling file_error calls (p5.c/a3.c,
emission order then identical to January), (function_name, file) parameter order per /Od (p6.c).
