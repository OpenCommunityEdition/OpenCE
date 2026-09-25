# REVIEW source/cseries/profile (critic_3 patch 05_profile: 11 static renames to Sept-2001 timesection vocabulary)

approve: True

amended_patch: None

## checks
All checks ran at HEAD 5d3ca708 (the task named 7b9de585; the commits between them change one docs ledger only). No tracked files were edited, and git status is clean apart from the untracked research/ directory that was already there. I claimed and then released source/cseries/profile under the label review_critic_3_profile. All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_profile\ (LEDGER.md, MANIFEST.md, rawcmp.txt, callcheck.txt, splitcmp.txt, objcmp_*.txt, gate_*.txt, battery/, objdiff/).

1) Pairing, checked without critic_3's tools:
- The raw Sept-2001 cachebeta.map has exactly 11 profile.obj entries under Static symbols, one name per address, so no identical-code-folding aliases.
- Merged by address with the public entries, their order and sizes equal January's split for all 11.
- rawcmp.py compared January split bytes with Sept xbe bytes at the Sept map address, masking only January's relocation fields: 0 differing bytes for all 11 pairs (sizes 0x10 to 0x610).
- callcheck.py found 6 January calls into the renamed functions. Decoding the same call in the Sept xbe lands on the proposed Sept name every time: 6 OK, 0 wrong. The other renamed functions are inlined at every January call site.
- January cachebeta.exe literals: the assert text 'parent_timesection->self_msec >= child_timesection->elapsed_msec', the hs global 'profile_dump_frames' and 'd:\framedump.txt'. None contradicts any proposed name.
- HCEX.pdb has no profile module (profile_initialize is absent), so it neither supports nor contradicts the names.
- None of the new names already exists in source/, config/symbols.json, build/split or build/base.

2) The patch only renames:
- Reverse-mapping the candidate reproduces production exactly, except the 11 lines of the symbol-listing comment, where `_code_0007e7e0`-style placeholders also go away.
- The patched symbols.json differs from the original only on the 11 name lines. Row order is unchanged.

3) Emulated split, using my own csplit runs:
- The control split matches build/split on all 833 objects.
- The patched config changes only source/cseries/profile.obj. Its 128 sections and 298 symbols are identical to the control apart from the 11 names (bytes, relocation records, storage class 3, section, value).

4) gate.py:
- Production against build/split: 43 exact, 1 residual, 0 unwritten.
- Candidate against the patched split: 43 exact, 1 residual, 0 unwritten.
- Rows are identical once the old names are mapped to the new ones. The only residual is the parked _compare_profile_sections (304 vs 288), unchanged.

5) Object compare, production vs candidate: 148 sections and 340 symbols identical apart from the names. Only .debug$S differs, because it contains the names.

6) Audit battery, production vs candidate:
- object_audit: FAIL(1) on both sides, from the same pre-existing _compare_profile_sections DIFF. Output is identical apart from the names.
- pdb_storage: 0 and 0 disagreements.
- surplus_identity: 0 surplus COMDATs.
- provider_link: PASS in both orders (the 3 `__real` constants, unchanged).

7) objdiff 3.3.1 (sha1 3130e428), my own mini report: 8112/9316 code, 42/44 functions and 1131932/1131932 data, the same on both sides. Zero credit, as claimed.

8) git apply --check:
- Patch 05 alone applies at HEAD.
- The series 01 and 03-21 also applies together in one git apply call.

9) Dependencies: the old names appear nowhere in config/, tools/ or tests (only in docs history). The only profile entry in parked.json is _compare_profile_sections, which is not renamed.

10) House rules and held classes:
- The change is identifiers and comments only.
- Strip test: nothing to test, because no casts, parentheses, locals or scopes are added.
- /Od declaration order: nothing to check, because no scopes are added.
- It follows the R2-1 precedent, which landed Sept-map first-party renames in xbox_texture_cache.
- The held January-bug item compare_profile_sections is untouched.

Not run (integrator's job): full ninja, pytest and the board-wide sweep.

## issues
No blocking issues. No amendment needed.

- The landing must regenerate the split in the same commit (ninja/csplit). Otherwise build/split keeps the old names and the 11 renamed functions show as regressions.
- The candidate object itself is unchanged apart from names; I confirmed that with my own emulated split.
- Non-blocking follow-up, outside this patch: struct and parameter names still use the 'profile_timer' vocabulary while the functions now use 'timesection'. critic_3 disclosed this as a follow-up.
- The 01+02 pair fails when passed to one multi-patch git apply --check call (symbols.json:602). critic_3 describes 02 as applying after 01 has been applied. That belongs to the ai_debug review, not this one. Patch 05 applies alone and within the 01,03-21 series.
