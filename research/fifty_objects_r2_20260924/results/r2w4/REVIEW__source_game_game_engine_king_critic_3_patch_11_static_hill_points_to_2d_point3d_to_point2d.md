# REVIEW source/game/game_engine_king (critic_3 patch 11: static hill_points_to_2d -> point3d_to_point2d)

approve: True

amended_patch: None

## checks
All work is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_king\ (LEDGER.md R1-R5, MANIFEST.md). No tracked file was edited. `git status` is clean apart from the pre-existing research/ dir. The claim was taken and then released as review_critic_3_king.

1. Patch integrity. `git apply --check` of patches/11 passes at HEAD 5d3ca708. 7b9de585 is an ancestor of HEAD and the difference between them is one docs ledger. I built the candidate by exact text replacement (4 .c occurrences, 1 symbols.json row). It is identical to an LF application of the patch. Only identifiers change: the listing comment, the static prototype, the definition, the single call in find_hill, and symbols.json line 2303. "static": true is kept.

2. Name evidence, checked independently:
   - Three raw maps name this slot. The Aug-2001 and Sept-2001 cachebeta.map both list `_point3d_to_point2d` in their "Static symbols" sections: size 0x20, game_engine_king.obj, between _king_engine_dispose and _find_hill. The Sept-2001 retail cache.map lists `@point3d_to_point2d@12`, which means 3 dword args and matches the 3 parameters.
   - No other object in any map defines this name, so there is no collision.
   - Raw bytes: January's split body (32 B, 0 relocations) is byte-identical to Sept cachebeta.xbe (.text+0x9ebb0), Aug 1749betaP.xbe (.text+0x94d20) and Sept retail cache.exe (0x475e70). These were parsed as data and never executed.
   - The current name was invented in the Fable King wave (9d2372ea/f68c6e25). HCEX.pdb has neither name, the Jan atlas has none at this address, and neither name is a cachebeta.pdb public, so the function is static on both sides. cachebeta.exe contains no string with either token. Nothing contradicts the Sept name.

3. Emulated split. I ran csplit (build/tools/csplit.exe) on config copies in my slug. The control split equals build/split for all 833 objects. The candidate split differs only in game_engine_king.obj. My own objcmp (bytes, relocations and symbols of every non-.debug section, name-mapped) reports IDENTICAL.

4. Gate:
   - Production against build/split: 29/29 EXACT.
   - Candidate against the emulated split: 29/29 EXACT, rows identical to production modulo the rename.
   - Control, candidate against the stale split: 28 exact plus `_hill_points_to_2d` UNWRITTEN. So csplit regen is required when this lands.
   - Compiled prod.obj against cand.obj: objcmp IDENTICAL name-mapped (74 sections, 197 symbols; only .debug$S differs).

5. Section-4 battery:
   - object_audit: PASS for production, and PASS for the candidate against the emulated split.
   - pdb_storage: 23 disagreements, identical for production and candidate. These belong to the held king storage packet and predate this patch.
   - surplus_identity: output identical (10 COMDATs, 0 not identical).
   - provider_link: PASS in both orders, identical.
   - Data sections are covered by the full-object objcmp.
   - objdiff 3.3.1 (sha1 3130e428): production and candidate both measure 4865/4865 code, 29/29 functions, 740/876 data. The helper is 100% under both names.

6. Held and house-rule review. No strip-test construct, new scope, declaration, cast or header change, so there is nothing to check for /Od declaration order. The authentic name satisfies house rule 7. King's held January-bug class is not touched. I grepped tracked tools, tests and config: nothing except symbols.json references the old name (the docs logs are history).

7. Held-packet compatibility. The owner packet's 01, 01A, if_declined, 03, 04 and 05 all still apply after patch 11. 02_king_symbols.patch does not (see issues).

8. /Od later build (evidence only). It locates the same loop as a real call, 0x5bdce0, from 0x5bcc80. The caller's RTC locals are named hill_points_3d and hill_points_2d. No symbol names are recoverable.

Not run: ninja, pytest, board-wide sweep (integrator).

## issues
No blocking issue. The patch is approved unchanged: it is byte-inert, earns zero credit and uses an authentic first-party name.

Non-blocking notes for the integrator:

(1) Evidence correction, which strengthens the case. The manifest's "0.929 against Sept" is an artefact of how capstone printed branch targets: it writes the absolute address, which includes the load address, so 13 of 14 instructions compared equal. The true relation is raw-byte identity (32/32) with Aug-2001, Sept-2001 cachebeta and Sept retail cache.exe. MANIFEST/LEDGER L2 could list king among the IDENTICAL rows.

(2) Integration hazard. The held round-1 owner packet research/fifty_objects_20260925/w/owner_queue/ub_king_glow/king/patches/02_king_symbols.patch uses symbols.json line 2303 (`_hill_points_to_2d`) as context. After patch 11 its hunk 1 FAILS. I made a copy that differs only in that one context line: scratch/w/review_critic_3_king/held_king_02_symbols_rebased_after_p11.patch. Its dry-run applies after 11. Whichever lands second needs this rebase.

(3) csplit regen is mandatory after landing. Against the stale split the helper row reads UNWRITTEN, and objdiff would pair the helper by its old name.

(4) Out of scope, pre-existing and not caused by this patch. The later /Od build calls the helper with arguments in the order (count, points3d, points2d). Our source uses (points, points2d, point_count). January is exact 29/29 with the current order, and the out-of-line body uses ecx=points3d, edx=points2d, esi=count in the Jan, Sept and retail builds alike. No action is needed; it is recorded in LEDGER R2.
