# source/effects/weather_particle_systems (admit source/effects/weather_particle_systems)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\patches\\01_one_over_char_max_external.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\patches\\02_weather_particle_system_globals_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\candB.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_weather_particle_systems\\LEDGER.md"
]

## production_changes
PROPOSED, not applied. Integrator applies; no tracked file was edited.
(01, the assigned construct) C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weather_particle_systems\patches\01_one_over_char_max_external.patch changes weather_particle_systems.c:309 from `static real const one_over_char_max = 1.f/255.f;` to `real const one_over_char_max = 1.f/255.f;`. It needs no symbols.json change, because row 14570 is already external. The patch is identical to storage_only.patch and to round-1 prod_storage_only.c.
(02, optional, outside the assigned construct) patches\02_weather_particle_system_globals_static.patch adds `static` to `struct weather_particle_system_globals weather_particle_system_globals = { 0 };` and adds `"static": true` to symbols.json row 22981. The two changes must land together as one atomic pair, because the split must be regenerated.
Both patches pass git apply --check, alone and together. They apply in either order in an isolated autocrlf=true repo, and the result is byte-identical to candB.c and to emu/config/symbols.json. Credit: +0 code, +0 data, +0 functions, +0 objects.

## evidence
Governing rule: storage follows cachebeta publics (WORKER_BRIEF s3: "public = external, absent = file-static"; commits 2535f91a, 7d92f97c, cdc8ebd3, 6e3e2d35, 05255584). Canonical 6e3e2d35 already landed the identical construct in the external direction: hs.c `static real const _hs_type_real_default` became `real const ...`. That symbol is a cachebeta public in .rdata section 0013, the same section as one_over_char_max. The precedent form is a plain keyword edit with no disclosure comment. Each condition checked:
- C1: cachebeta_publics.txt:5686 lists `_one_over_char_max` as public at 0x25AAFC.
- C2: the January split has storage 2. HCEX.pdb lists it as `Global, const float` and as a PublicSymbol.
- C3: the only definer or referencer in build/split and in build/base (621 objects) is weather.
- C4: control.obj equals build/base. Candidate vs control: 61/61 sections are equal (section_infos_equal and raw bytes), and exactly one symbol differs (_one_over_char_max storage 3->2). gate --all rows are identical to production: 19 EXACT plus _weather_particle_system_render 1680 residual [sha], which is pre-existing. The objdiff 3.3.1 mini project gives the same result for prod and cand (4114/5779 code, 19/20 functions, 949/949 data).
- C5/C6: the name is authentic, the edit is in the owner .c, and no header extern is needed because nothing else uses the symbol. The sibling `real const _one_over_255` in ui_widget.c is precedent for that.
Admission battery results:
- object_audit: production FAIL(2), candidate FAIL(1, render only), 30 January symbols with 0 differing.
- Data: 10/10 sections EQUAL under coff_compare.
- pdb_storage: 2 disagreements before, 1 after.
- surplus_identity: unchanged, 13/14 IDENTICAL; `_real_local_random` differs, as before.
- provider_link: unchanged, FAIL(1) on `_real_local_random` in both orders. With --baseline it PASSes (no new surplus).
- /W3: 15 warnings, the same set as production.
- fake_match_scan: 0 leads.
The second disagreement: `_weather_particle_system_globals` is not a cachebeta public, and HCEX marks it 'File Static'. I emulated the split with csplit.exe into a config copy under the slug; only the weather object changed, by one storage bit. candB against the emulated split: 19 EXACT plus render, 30/30 symbols with 0 differing, pdb_storage 0, data 10/10, and objdiff rows identical to production.
Full record: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weather_particle_systems\MANIFEST.md and LEDGER.md.

## blockers
This fix does not make the object complete; it stays NonMatching.
(1) _weather_particle_system_render (1680) is still residual. It waits on the owner ruling for Lane D class I: the /Od-attested dead `short unused_count = 0;` with an invented name. Round-1 production.patch closes it on a YES.
(2) The `_real_local_random` surplus COMDAT differs from effects.obj's selected copy, and provider_link fails in both orders. The repair is in real_math.h, which is RESERVED for the Codex packet.
So the brief's 'every January function EXACT' check fails for the object: it is 19/20, unchanged from production. The storage precedent itself does not require it; zero-credit storage fixes have landed on incomplete objects before (cdc8ebd3).
Procedural note: I ran `python -m tools.parked_functions --help` before reading that script's source. I checked right away: git status was unchanged, and the script exits in parse_args before its only write.

## reopen_criteria
Render: apply the round-1 body patch after an owner YES on class I (it includes 01's hunk). Provider: once the Codex real_math reservation is released, re-measure the named-result real_local_random header body with a full-board sweep. Patch 02: the integrator decides whether to land it as the atomic source+symbols.json pair, with a full ninja so csplit regenerates the split. The emulated split shows only the weather object changing.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weather_particle_systems\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_weather_particle_systems\LEDGER.md
