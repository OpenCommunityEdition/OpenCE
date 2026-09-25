# r2w2 weather_particle_systems - storage-only admission ledger

Label: r2w2_weather_particle_systems. Claimed 2026-09-24 (claim.py rc=0).
Tree: 434f0151 (HEAD; 05255584 + R2 ledger commit), clean except untracked research/fifty_objects_r2_20260924/.

## Task
Precedent-covered zero-credit storage fix: drop `static` from `one_over_char_max`
(cachebeta public 0x25AAFC). Governing rule: storage follows cachebeta publics.

## Log
- Read WORKER_BRIEF_R2 s0-9, REVIEW (round 1 check 10), TRIAGE (r2w1), precedent_audit ledger.
- Production source: source/effects/weather_particle_systems.c:309 `static real const one_over_char_max = 1.f/255.f;`
- cachebeta_publics.txt:5686 `PublicSymbol: [0025AAFC][0013:00017EDC] _one_over_char_max`
- Only definer/user in source/: weather_particle_systems.c (grep). symbols.json:14570 row has no "static" flag.
- Candidate cand.c = production + one-token edit (drop `static` at l.309). control.c = production copy.
- gate --all: control and cand rows IDENTICAL: 19 EXACT + render residual [sha] (pre-existing class-I item). gate_control.txt/gate_cand.txt.
- objcmp.py: build/base obj == control.obj (61 sections, 144 symbols, 0 diffs). control vs cand: 0 section diffs
  (section_infos_equal + raw bytes), exactly 1 symbol diff: _one_over_char_max storage 3 -> 2.
- object_audit: base FAIL(2) [render + _one_over_char_max storage 2/3]; cand FAIL(1) [render only]; 30 January symbols, 0 differ; surplus list unchanged.
- datacmp.py split vs cand: 10/10 data sections EQUAL (8 .rdata incl. one_over_char_max 4 B, .data _weather, .bss globals 628 B).
- pdb_storage (production): 2 disagreements: _one_over_char_max (split 2, ours 3, public) and _weather_particle_system_globals (split 2, ours 2, NOT public).
  Candidate variant: 1 disagreement (_weather_particle_system_globals only).
- Cross-object refs in build/split: neither _one_over_char_max nor _weather_particle_system_globals is referenced by any other January object; each has exactly one definer (weather).
- HCEX.pdb: one_over_char_max is a PublicSymbol (export is DATA) -> corroborates external. weather_particle_system_globals is "File Static", type struct <unnamed-tag> -> corroborates the cachebeta absence (file-static).
  => _weather_particle_system_globals is a SECOND storage disagreement under the same rule, outside the assigned construct; measured separately below as optional sub-packet B.
- /W3 syntax census (w3.py, production cflags): control 15 = cand 15 warnings, identical set.
- surplus_identity (base) and candidate variant: identical 14 COMDATs, 13 IDENTICAL, _real_local_random DIFFERENT (pre-existing, effects.obj NODUP).
- provider_link cand: FAIL(1) only _real_local_random, both orders (identical to production). --baseline=build/base: "no surplus external definitions: PASS".
- build/base scan (621 objects): _one_over_char_max defined only in weather (now external) and referenced nowhere else -> no LNK2005 exposure.
- storage_only.patch built (LF, index d2aae8ff..40d657c0; HEAD blob d2aae8ff matches). git apply --check OK in worktree; isolated autocrlf=true repo apply reproduces cand.c byte-for-byte. Also equals round-1 prod_storage_only.c.
- PROCEDURAL NOTE: ran 'python -B -m tools.parked_functions --help' before reading its source (brief s7 violation). Checked immediately: git status unchanged (only untracked research/); source read afterwards: argparse parse_args at l.280 exits on --help before the only write (l.293, --output). No effect.
- Precedent search: canonical 6e3e2d35 (hs.c) already landed the IDENTICAL construct in the external direction:
  `static real const _hs_type_real_default= 0.0f;` -> `real const ...` (cachebeta public 0x25D7D4, .rdata section 0013). Form = plain keyword edit, no comment.
- objdiff 3.3.1 mini project (od/): prod == candA == prod_vs_emu == candB_vs_emu: 4114/5779 code, 19/20 fns, 949/949 data; all rows identical. Zero credit.
- Sub-packet B (optional, outside assigned construct): HCEX "File Static" + cachebeta absence for _weather_particle_system_globals.
  emu split (config copy, symbols.json row 22981 += "static": true; csplit exit 0; diff -rq: only weather obj; 1 symbol storage 2->3, 30/30 sections identical).
  candB vs emu: 19 EXACT + render [sha]; audit 30/30 symbols 0 differ; pdb_storage 0; data 10/10. candA vs emu shows the expected 3/2 DIFF => 02 is atomic (source+symbols.json).
- Patches: patches/01 (assigned), patches/02 (optional). --check clean singly and together; isolated autocrlf repo both orders == candB.c + emu symbols.json.
- RESULT: ADMISSION_FIX_PROPOSED (zero credit). Object remains NonMatching: render class-I owner ruling + _real_local_random provider (Codex real_math reservation).
