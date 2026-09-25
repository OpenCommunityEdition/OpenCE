# review_critic3_hs LEDGER: adversarial review of critic_3 patch 13 (source/hs/hs)

Base/tip 5d3ca708, worktree clean at start and end (only the pre-existing untracked research/). No tracked edits,
no ninja/configure/git mutation. Claim: source/hs/hs claimed as review_critic3_hs (released at the end).
Patch reviewed: scratch/w/critic_3/patches/13_hs.patch sha1 e2837739 (copy: 13_hs.patch.orig).

## R0 the claim under review
- 3 static no-op evaluators at January 0xb14a0/0xb14c0/0xb14e0 (0x20 each): `radiosity_{start,save,debug_point}_evaluate`
  (invented from the hs command strings) -> `radiosity_hack_{start,save,find_point}_evaluate` in hs.c (listing comment,
  prototypes, 3 definition initialisers, 3 HS_EVALUATE_NO_OP lines) + symbols.json rows 2908-2910 (name only; "static": true kept).

## R1 first-party name evidence (independent re-derivation; xbe_defs.py)
- Three first-party maps agree: Aug-15-2001 cachebeta.map (0001:000a3590/b0/d0), Sep-25-2001 cachebeta.map
  (0001:000afb20/40/60) and Sep-25-2001 retail cache.map (@radiosity_hack_*_evaluate@12, fastcall) all name them
  `radiosity_hack_start/_save/_find_point_evaluate`, in this order, between _debug_pvs_evaluate and the next evaluator.
- DATA pairing (not only order), parsed myself from the XBE images (section 0 VA 0x11000):
  Sept .rdata struct for "radiosity_start" -> evaluate 0xc0b20 = 0001:afb20 = _radiosity_hack_start_evaluate;
  "radiosity_save" -> 0xc0b40 = _radiosity_hack_save_evaluate; "radiosity_debug_point" -> 0xc0b60 =
  _radiosity_hack_find_point_evaluate; parse slot -> _hs_macro_function_parse (decode sanity). Aug xbe: same mapping
  (0xb4590/b0/d0; parse -> _hs_macro_function_parse). help strings match January's.
- January image (cachebeta.exe, PE parse): "radiosity_start" def 0x65fdb0 -> evaluate 0x4b14a0 (file 0xb14a0);
  "radiosity_save" -> 0x4b14c0; "radiosity_debug_point" -> 0x4b14e0. Each address has exactly ONE absolute reference in
  the whole image (its definition slot); no E8 callers. Split + defrel.py: _hs_radiosity_*_definition +0x0c relocate to
  the three symbols (production and split agree).
- Body identity: January/Sept/Aug bodies are the same 20-byte `push ebp; mov ebp,esp; mov eax,[ebp+0c]; push 0; push eax;
  call _hs_return; add esp,8; pop ebp; ret` (call targets resolved: Sept 0001:b9960 _hs_return, January 0xbb320 _hs_return).
  Identity alone cannot pair no-op evaluators (all identical) - the struct data does.
- Coverage: 444 of the 447 Sept hs.obj function names are ALREADY the January split names; the only 3 Sept names absent
  are exactly these. After the patch 447/447; the sole January-only function is _scripted_foley_predict_evaluate.
- Contradiction search: cachebeta.pdb publics contain no *radiosity*_evaluate (file statics; consistent); January exe
  strings: only command/global names (radiosity_start/save/debug_point/lines/normals/quality/step_count), no
  "find_point"; symbol atlas: only the 3 Sept retail map rows (no other lineage name); HCEX.pdb: neither old nor new names
  (DIA2Dump -sym, sanity-checked with create_weapon). Current names have no provenance at all.

## R2 reference consistency
- `git grep` (all tracked files, excl. scratch/research): old names occur ONLY in symbols.json 2908-2910 and the 9 hs.c
  sites the patch edits; nothing in parked.json, semantic_*.json, symbol_ownership, relocs, rejections, docs, tools/tests.
- Prototype block stays alphabetical (profile_sections_deactivate < radiosity_hack_find_point < _save < _start < random_range).
- Listing comment: the 3 `_code_000b14a0/c0/e0` lines renamed; other `_code_` lines are pre-existing initial-commit
  staleness (precedent for naming listing lines exists, e.g. f7afb704).
- No other scratch patch has a symbols.json hunk near 2890-2925. critic_2's hs_compile packet (submitted.patch ==
  hs_compile_storage.patch) + patch 13 apply in BOTH orders (GNU patch on LF copies) with identical results.

## R3 measurements (all slug-local)
- git apply --check 13_hs.patch at 5d3ca708: OK (LF patch, index LF, worktree autocrlf).
- csplit: split_ctl (config copy) == build/split 833/833; split_P (patched symbols.json) differs in hs.obj only;
  objcmp split_ctl vs split_P: 1359 sections / 3698 symbols IDENTICAL with the 3-name map (negative control without the
  map: DIFFER 4).
- gate (all January functions): production vs build/split 447/1; candidate vs split_P (with_split.py) 447/1; rows SAME
  modulo the rename (residual _real_random_range_evaluate [sha] pre-existing). Negative control: candidate vs unpatched
  split = 444/1/3 unwritten (wrapper effective).
- objcmp production obj vs candidate obj: 1410 sections / 3753 symbols IDENTICAL modulo rename (bytes, relocations,
  symbol order/storage; .file/.debug$S aux = compile path only; build/base vs gate recompile also IDENTICAL).
- With critic_2's hs_library_internal_compile.h shadowed ahead of source/hs (cl_shadow.py; #error probe proves the shadow
  is taken): prod vs prod+c2hdr IDENTICAL; prod+c2hdr vs cand+c2hdr IDENTICAL modulo rename.
- object_audit: prod vs build/split FAIL(1) and cand vs split_P FAIL(1), same text modulo rename/padding; the 1 = the
  pre-existing _real_random_range_evaluate DIFF; "1830 January symbols, 0 differ" both.
- pdb_storage: 0 disagreements both. surplus_identity: 0 candidate-only code COMDATs both. provider_link (both orders
  internally): PASS both, identical text (2 pre-existing literal surplus rows).
- objdiff 3.3.1 (sha1 3130e428) mini project: prod, cand and build/base units identical measures (code 25248/25329,
  447/448 fns, data 18/54798 = pre-existing '$'-literal defect); renamed functions 100% on the cand side.

## R4 house rules / held classes
- Names only; no new declarations, scopes, casts, parens, macros or flags -> strip test and /Od declaration order n/a.
- Rule 7 (authentic names): first-party names replace invented ones. `hack` is part of the authentic Bungie name.
- Not a held class (not a January-bug item, not owner-queue, not COMDAT, not decals/physics).

## R5 verdict
APPROVE as submitted, zero credit (byte-inert). No amendment needed. Integrator: land with csplit regen; independent
of critic_2's hs_compile packet (either order).
