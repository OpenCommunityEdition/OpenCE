# source/rasterizer/rasterizer_geometry

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_uncompress_int32_to_real_vector3d"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_geometry\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_geometry\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_geometry\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_geometry\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_geometry\\probes\\t7.txt"
]

## production_changes
1) Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_geometry\production.patch to source/rasterizer/rasterizer_geometry.c. The patch keeps the file's CRLF line endings. `git apply --check` exits 0, and `patch --binary -p1` reproduces cand.c byte for byte. The only change is the body of uncompress_int32_to_real_vector3d: it adds `real value;` after `real_vector3d v;` and makes each int-to-real conversion its own statement (`value = (real)(long)(compressed<<21);` then `v.i = (value * (1.0f/1048576.0f) + 1.0f) * (1.0f/2047.0f);`; the same for j with <<21, and for k with <<22, 1/2097152 and 1/1023). The `compressed >>= 11` shifts, the constants and the by-value return are unchanged. No header, prototype, symbols.json or ABI change. 2) Retire the park in config/parked.json for unit source/rasterizer/rasterizer_geometry, function _uncompress_int32_to_real_vector3d, class instruction-scheduling (the entry near line 516). Run `python -B -m tools.campaign.unpark --write` after the full ninja, or remove the entry by hand. 3) Optional, following the lane's convention: flip config/config.json `source/rasterizer/rasterizer_geometry.c` (index 96) from NonMatching to Matching. This is a status edit and earns no credit. 4) Optional closeout ledger in docs/object_matching_logs, copied from scratch/w/rasterizer_geometry/LEDGER.md. If the owner prefers the arithmetic spelling from the later /Od build, use probes/t7.txt instead: the same staged temp plus `&0x7ff` masks and divisions. It is also EXACT.

## evidence
Baseline: gate reported 18 EXACT and 1 residual [sha]. January and ours both have 49 instructions; the only difference was that January emits `fadd __real@3f800000` before `shl eax,0x16`. Lab mechanism (probes/labptr.c, x1/x4/x5): VC7 places exactly 3 integer filler instructions between dependent x87 ops. January's fmul(i) group holds one integer node that emits no bytes, and adding a visible integer op in that position reproduces January's order. What moves the i-component fadd one slot earlier is staging the int-to-real conversion as its own statement stored to a real (t4). Every natural staging of that conversion is EXACT: a reused temp, three temps, in-place v.i staging, a temp initialised at its declaration, the temp plus /Od masks and divisions, and a temp for i only (t4-t9). More than 15 unstaged spellings stay residual, including every recorded negative plus the /Od masks and divisions alone. Also inert: double intermediates (they would emit qword constants, which January's relocations refute), casts, multiply or split shifts, copy variables, struct, array and pointer variants, 1-33 dummy declarations (oracle only), 1-8 unused locals, 1-8 live copies, and the /G5 /G6 /G7 /GB /Op /Ot /Os /Oi /QIfist /arch:SSE flags. Whole-object audit on cand.obj: gate 19/19 EXACT (gate_cand.txt). object_audit.py PASS: all 44 January symbols match, every January-owned section is ok, and the candidate-only surplus is identical to the production base (literal COMDATs plus the owner-admitted _fast_ftol). surplus_identity: _fast_ftol is IDENTICAL to January's selected copy in actor_combat.obj. provider_link: every surplus symbol PASS in both link orders. pdb_storage: 0 disagreements. build/report.json: data 864/864 (100%); code 3919/4054, and this closes the missing 135 meaningful bytes. No admission rejections or semantic entries exist for the unit. fake_match_scan: 0 leads. CL /Zs /W3: no new warnings. The change touches only one function body in one translation unit, with no header or ABI change. Callers in other translation units call the extern function, so there is no blast radius.

## blockers
None mechanical. Owner-review disclosure: the later first-party /Od build (fn 0x82ea80, 0x18-byte frame = v, RTC guard and GS cookie) has no scalar local. It computes cvtsi2ss straight into divss and writes `& 0x7ff` masks and divisions. So the staged `value` temp is not attested by /Od; it is inferred from January's own bytes. Every natural staging matches and no unstaged spelling does, and the file already stages values through a reused real (compress_real_to_int16: `z = (real)floor(z * 32767.5f)`). `value` is assigned and read three times, so it is not a filler or dummy local. The strip test removes it back to the parked residual, which is why it is disclosed for owner judgement.

## reopen_criteria
If the owner rejects the staged conversion temp, keep the park. Reopen only on first-party evidence of January's statement shape for this function, such as an earlier /Od build or source fragment. Do not respell with casts, parentheses or double constants; all are measured inert (see LEDGER.md P1-P14).

## task notes
Resumed fresh: scratch/w/rasterizer_geometry/ did not exist, so no earlier probes were lost. Every probe was appended to LEDGER.md as it ran. The mechanism finding may generalise to other x87/integer interleave residuals. VC7 fills exactly 3 integer slots between dependent x87 ops, and a code-invisible node (such as a conversion staged into a named real) shifts the integer stream by one slot. The research harness is scratch/w/rasterizer_geometry/probe.py, mkvar.py and lst.py. production.patch is CRLF-preserving; apply it with `git apply` or `patch --binary`.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_geometry\LEDGER.md
