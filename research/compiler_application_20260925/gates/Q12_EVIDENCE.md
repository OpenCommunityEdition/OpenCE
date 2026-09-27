# Q12 data entries: independent verification and evidence pins (2026-09-26)

Owner ruling item 8 (Q12): the four single-section entries through the EXISTING verifier, conditional on fresh
independent identity and complete section/relocation/ownership checks; evidence hashes pinned. King evaluated
separately; its code and whole-object holds stay intact. Data gains reported separately from code gains.

Independent checker: tools/data_identity.py (own COFF reader; does not import tools/coff_compare). Per section:
size, characteristics, payload with relocation fields zeroed, every relocation (offset, type, target, addend),
owner symbols (name, offset, storage class), and each ??_C@ literal target's content against January's
linker-selected definition (scan of all build/split objects; exactly one distinct content required).

| unit | owner | size | relocs | literals checked | January UNDEF literals | normalized sha256 (verifier pin) |
|---|---|---:|---:|---:|---|---|
| source/bitmaps/bitmap_group | _global_bitmap_reference | 1424 | 152 | 116 | ??_C@_00CNPNBAHC@?$AA@ (January copy: source/ai/action_obey.obj); ??_C@_07DLHCIBDH@default?$AA@ (January copy: source/ai/actor_looking.obj) | df5c6d69f4e13ea6df9398e2b5cc5c4455410b077b5ba6763930c4dd748505cb |
| source/ai/ai_communication | _global_communication_priority_names | 276 | 69 | 69 | ??_C@_04CGFJFPFD@none?$AA@ (January copy: source/ai/actions.obj); ??_C@_04IMJFEJN@flee?$AA@ (January copy: source/ai/actions.obj); ??_C@_05BAOFOMKD@shout?$AA@ (January copy: source/ai/ai.obj); ??_C@_05MGHOCOML@cover?$AA@ (January copy: source/ai/actor_firing_position.obj); ??_C@_06GJOIPFFF@damage?$AA@ (January copy: source/ai/actor_looking.obj); ??_C@_07PEIBNLKE@berserk?$AA@ (January copy: source/ai/action_obey.obj) | d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03 |
| source/ai/ai_debug | _global_ai_debug_firing_position_color_count | 56 | 11 | 11 | ??_C@_04CGFJFPFD@none?$AA@ (January copy: source/ai/actions.obj) | 3ba0a54a0875e71a01972b13dcf574a4d16d0f1ea3531ef51d98311b5ac7eee5 |
| source/game/game_engine_king | _king_engine | 136 | 23 | 1 | ??_C@_04PJOEONHN@king?$AA@ (January copy: source/game/game_engine.obj) | 4009f5b6117e9f00cb001a23b9a56b9dc51a694206c442ce715e2bed2d5465a7 |

Object pins (sha256 of the objects checked; build/base rebuilt at lane commit bd68541c + the entries):
- source/bitmaps/bitmap_group: January build/split 9c138a38b3f364d94dcfd0e2a389594e0cf118588d71ff16ea631a66a40c4e7c; ours build/base 754b9213d97412eedbf81d21e939c91f187ba9c5276e31289c2644c5b333f5b0
- source/ai/ai_communication: January build/split 7c63a9cdfdfce1f3faf3f1316010be76c191175f60462142384599bae9140535; ours build/base 088f3beeea4e296379b5114869022164b7c21d6d91b1eb97cfb6d06421675a15
- source/ai/ai_debug: January build/split c8a62d4199a73333df020be794bffe55caa9d130c256e934b494632e319d48ad; ours build/base ac0f934474709023b144e4f366722b4187b96cb8d8efcd7b0f7af36f39e3622d
- source/game/game_engine_king: January build/split 1977994ee6b936cf6a92a3e23906d0d66a339fd29c3457c149363cafc517f996; ours build/base 31cbfed400a983816dc813cf2c9e97236d4d161af3165f7753aa33d0205d8e17

Every check PASSED for all four sections (RESULT PASS x4; raw log gates/Q12_identity.txt).

Ownership: every data-section owner has the same name/offset/storage class in January and ours; none is among
pdb_storage's disagreements (pre-existing function-level rows only: bitmap_group 2, ai_communication 3, ai_debug 1,
king listed in gates/Q12_ownership.txt). object_audit: each .data row 'ok' (size, flags, alignment equal); the
incomplete units' FAIL counts are their residual functions, unchanged.

Existing verifier (tools/semantic_progress.apply_semantic_data_matches, fail-closed) with the three entries:
bitmap_group +1,424 (7,868/7,868), ai_communication +276 (11,892/11,892), ai_debug +56 (5,874/5,874); board data
2,587,011 -> 2,588,767 (+1,756); code, functions, objects, parks unchanged; no other unit moved.

Patch pins: three entries semantic_data_matches_B2_three_entries_without_king.patch sha256 cdb0b2ab6bc601869d063d5df74f3dc863eab9ef6d258e0ac0352cbb22945fdd;
king semantic_data_matches_B2_king_only.patch sha256 b0ab7ea1e29d6d8bf74d2abf4e5e28a9171cebd66dfb4cf05c0714322a9af06b.

King (separate packet): the existing verifier credits +136 (game_engine_king 876/876 data); board data 2,588,767 ->
2,588,903. Side effect (disclosed, as B2 predicted): tools/audit_object_admission.py now lists game_engine_king as a
review candidate (10 -> 11; decision 'audit-coff-ownership-before-admission'). This is a listing, not an admission:
config.json keeps game_engine_king NonMatching, owner queue #13 (_find_next_hill's uninitialised no-candidate return
plus the cachebeta storage fix) stays held, and no code or object credit moves.

## Addendum 2026-09-26: completed Q12 evidence (owner request; lane otherwise stopped)
All files are under `gates/Q12/` unless named otherwise. The objects checked are the current build: the four units'
build/base objects equal a fresh compile at HEAD dba492ac in every code and data section and symbol
(`freshness.txt`). `.debug$S` differs only by the S_OBJNAME output path (`tools/debugs_objname.py`).

**Corrections.**
- The logs committed with 387bab37 were FILTERED:
  - `Q12_identity.txt` was `tail -n +7` per unit, which dropped the size, characteristics, payload,
    relocation-count, relocations and owner-symbol lines;
  - `Q12_ownership.txt` was tail/grep excerpts.
  Both files now hold the complete, unfiltered output, identical to `identity_full.txt` and the `ownership_*.txt`
  logs.
- The earlier literal step of `tools/data_identity.py` compared only the NUL-terminated prefix. The
  complete-section comparison below supersedes it.
- The "Object pins" above hold only for the build instance at 387bab37. Current pins are in `PINS_objects.tsv`.

1. **Identity, complete and unfiltered** (`identity_full.txt`, 243 lines): 221 check lines, every one PASS, and
   RESULT PASS with exit code 0 for all four sections. This covers size, flags, payload with relocation fields
   zeroed, relocation count, every relocation (offset/type/target/addend), owner symbols and every literal target.
2. **COMDAT selection and complete contents** (`literals.json` sha256 7977dcf72c2198f70cbad6e46f9d6a4986adf594abf8133398a41cdfc07cce5e, `literals.txt`): 197 literal rows (bitmap_group 116,
   ai_communication 69, ai_debug 11, king 1).
   - Every definer, whether January's split definer or any current definer in our build, is a COMDAT `.rdata`
     section with selection 2 (IMAGE_COMDAT_SELECT_ANY), taken from the actual section-definition aux record.
   - Each section holds exactly one literal symbol and no relocations; the aux Length equals the section size.
   - Size, alignment and the COMPLETE section bytes are identical across all definers on both sides. The full bytes
     of every definer are recorded in hex.
   - Informational differences, disclosed and not used by the entries' measurements:
     - aux CheckSum: csplit writes 0, ours a CRC;
     - aux Number: csplit writes the section's own index, ours 0.
     Per the PE/COFF specification, Number applies only to ASSOCIATIVE selection and CheckSum is not consulted for
     ANY. The Q12 entries measure the `.data` section (normalized bytes plus resolved relocations) only.
3. **Current providers of the 10 literals January's units leave UNDEF.** Each has exactly one January definer (the
   linker-selected copy). That provider's current build/base object defines the same literal as COMDAT ANY with
   identical complete bytes. The count of current definers in our whole build is shown for information:

| unit | literal | January provider (split) | bytes | current definers in our build | link unit-first / provider-first |
|---|---|---|---:|---:|---|
| bitmap_group | `??_C@_07DLHCIBDH@default?$AA@` | source/ai/actor_looking.obj | 8 | 5 | PASS / PASS |
| bitmap_group | `??_C@_00CNPNBAHC@?$AA@` | source/ai/action_obey.obj | 1 | 44 | PASS / PASS |
| ai_communication | `??_C@_04CGFJFPFD@none?$AA@` | source/ai/actions.obj | 5 | 13 | PASS / PASS |
| ai_communication | `??_C@_05BAOFOMKD@shout?$AA@` | source/ai/ai.obj | 6 | 3 | PASS / PASS |
| ai_communication | `??_C@_06GJOIPFFF@damage?$AA@` | source/ai/actor_looking.obj | 7 | 5 | PASS / PASS |
| ai_communication | `??_C@_05MGHOCOML@cover?$AA@` | source/ai/actor_firing_position.obj | 6 | 3 | PASS / PASS |
| ai_communication | `??_C@_04IMJFEJN@flee?$AA@` | source/ai/actions.obj | 5 | 6 | PASS / PASS |
| ai_communication | `??_C@_07PEIBNLKE@berserk?$AA@` | source/ai/action_obey.obj | 8 | 5 | PASS / PASS |
| ai_debug | `??_C@_04CGFJFPFD@none?$AA@` | source/ai/actions.obj | 5 | 13 | PASS / PASS |
| game_engine_king | `??_C@_04PJOEONHN@king?$AA@` | source/game/game_engine.obj | 5 | 2 | PASS / PASS |

4. **Provider-link receipts** (`receipts/`):
   - 20 receipts, one per literal and order, with the command line, object hashes and the complete Link.Exe
     output. Each has 0 LNK2005/LNK1169 lines. Exit code 1120 is LNK1120 for unresolved externals, expected in this
     /NODEFAULTLIB probe.
   - 4 negative controls (`receipts/CONTROL_*`, each unit linked with a byte copy of itself): every one reports
     LNK2005 for the unit's non-COMDAT `.data` owner, so the probe detects real duplicates. None reports a
     duplicate literal.
   - The existing `scratch/tools/provider_link.py` over every surplus external definition (`provider_link_*.txt`,
     unfiltered): bitmap_group 4, ai_communication 42, ai_debug 89, king 23 PASS rows, 0 FAIL, all 10 literals
     included.
   - This is bounded duplicate/coalescing evidence between current objects, not a whole-program link.
5. **Ownership, unfiltered** (`ownership_*.txt`, `owner_vs_pdb.txt`): no data-section owner (31, 3, 4 and 1) is
   among its unit's pdb_storage disagreements, which are function-level or held rows: bitmap_group 2,
   ai_communication 3, ai_debug 1, king 23 (king's whole-object storage fix, owner queue #13, stays held).
   object_audit: FAIL(1) / FAIL(4) / FAIL(2) are the residual functions of the incomplete units; king PASS.
6. **Pins:**
   - `PINS.sha256` (sha256 f5bca56c96b72231d7074654a0d6a5728b3dbb65b49b5a857c9396453f9da795): 74 files that persist in the repository (archived objects, logs, receipts, tools,
     configs, Link.Exe); `sha256sum -c` from the worktree root verifies all 74.
   - `PINS_objects.tsv` (sha256 13459b70970179baf60b23c2e5022bb2ed90dc39799e0cca4ab8f90868ae66f2): all 126 checked objects with their raw and TimeDateStamp-masked sha256. Raw
     build hashes hold for this build instance; the masked hash survives a deterministic rebuild.
   - `objects/`: byte copies of the 20 core objects (the four units and the ten receipt providers, January and ours).

Unchanged by this addendum: the four entries, all credit figures, every hold (including king's code and whole-object
holds and the inherited near_player initializer), the scorer and objdiff 3.3.1. No source change and no push.
