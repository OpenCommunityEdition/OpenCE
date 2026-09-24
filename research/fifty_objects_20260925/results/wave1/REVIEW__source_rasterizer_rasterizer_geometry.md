# review source/rasterizer/rasterizer_geometry
approve=False

## checks
Everything below was re-run independently. Evidence is in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_rasterizer_geometry\ (REVIEW.md, LEDGER.md, gate_cand.txt, object_audit*.txt, provider_link.txt, w3_*.txt, probes/).
- Source diff: the worker's cand.c, final.c and my copy are identical. Against production, the only change is the body of uncompress_int32_to_real_vector3d. Both files are CRLF.
- Gate: `gate.py --source cand.c --all` gives 19/19 EXACT, 0 residual, 0 unwritten. The production copy leaves _uncompress_int32_to_real_vector3d residual [sha]. alndiff on production shows 49/49 instructions, and the only difference is January's fadd placed before `shl eax,0x16`. alndiff on the candidate shows only relocation-name rows.
- object_audit.py on cand.obj: PASS. All 44 January symbols match and every January-owned section is ok. The base audit fails only on this one function. The candidate-only surplus list is identical to the production base.
- Surplus and linkage: surplus_identity finds _fast_ftol IDENTICAL to the copy in actor_combat.obj. provider_link on cand.obj passes every surplus symbol, and the selected-provider link passes. pdb_storage finds 0 disagreements.
- Data and scoring: build/report.json shows data 864/864 (100%) and code 3919/4054, with the missing 135 bytes in this function. No rejection or semantic entries exist for the unit.
- Lint: fake_match_scan finds 0 leads. CL /Zs /W3 gives the same warning set for production and candidate.
- Patch: production.patch passes `git apply --check`, and `patch --binary -p1` reproduces cand.c byte for byte.
- /Od readout: re-derived with odbuild (fn 0x82ea80, called from 0x82d080). It is identical to the worker's od_uncompress.txt. The frame is 0x18 (RTC guard + v, size 12 + guard + GS cookie) and has no scalar user local.
- STRIP TEST:
  - s1, the exact /Od spelling (masks, divisions, no temp): residual.
  - s2, the same spelling plus the staged `real value` temp (the worker's t7): EXACT.
  - Production, with no temp: residual.
- House rules on the diff: real type, parameter layout and return are unchanged. No header, prototype or ABI change. The name is descriptive and there is no fake-scan smell.

## issues
The byte and object-audit claims all reproduce. The candidate still fails the source-credibility gate as proposed, so this needs an owner ruling, not a landing.
1) The strip test fires. The added `real value;` staging temp changes no semantics, and it alone buys the match. The plain production spelling and the exact first-party /Od spelling are both residual. Every staged form is EXACT, so the lever is robust, but that is byte-derived evidence, not evidence about January's source.
2) First-party evidence contradicts the temp. The later /Od+/RTC build of the function (0x82ea80) has frame 0x18 = guards + v + cookie, with no scalar user local. /Od homes every named local, so that source had no staging temp. Its one store per component also rules out the in-place staging variant. Nothing shows the function was revised between January and that build: its masks and divisions fold to January's bytes.
3) The worker's "file style" support does not hold. compress_real_to_int16's `z = (real)floor(...)` reassigns a parameter, which January's bytes attest (fstp [ebp+8]); it is not a new temp. The siblings uncompress_int8/16_to_real are direct expressions with no staging. The `value` temp leaves no trace in January's bytes.
4) The 2026-09-20 owner ruling says to escalate, not rule unilaterally, when the strip test fires. It also requires an inferred construct to be disclosed as inferred, with a comment in the .c that reproduces the measurement. production.patch adds a bare temp with no such comment.
The one precedent for a scheduling temp is the units FP-intermediate reused temp. It predates the /Od-readout rule and did not face contrary /Od evidence.
If the owner admits it:
- Apply production.patch, or t7.txt, whose arithmetic is the /Od one and is also EXACT.
- Add a comment above the staged statements covering three points: the staging is inferred from January's scheduling bytes; the later /Od revision has no scalar local; plain and /Od spellings are residual while staged ones are EXACT.
- After the full ninja, retire the parked.json entry for _uncompress_int32_to_real_vector3d (instruction-scheduling) with `python -B -m tools.campaign.unpark --write`.
- Optionally flip config.json index 96 to Matching.
Blast radius: none found. The function is extern, all 19 functions in the TU stay EXACT, and nothing shared changes.
Other routes to approval: a temp-free spelling that gates EXACT, or first-party evidence of a January-era staged conversion.
