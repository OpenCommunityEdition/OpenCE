# `effects.obj` Jonas public-API pair — 2026-08-28

## Result and frozen boundary

Starting from pushed base `c8c40b2ceaaee631f11abbed03447257c83f9a36`, this lane attempted exactly two ordinary-C bodies in `source/effects/effects.c`:

- `_effects_initialize`
- `_effects_information_get`

Both matched the January target exactly on the first and only code-producing compile. No residual remained, so no pruning, tuning, retry, comparator exception, or semantic adjudication was used. The explicitly excluded private-call bodies `_effects_update` and `_effect_stop` were not touched.

| owner | meaningful / padded bytes | relocations | normalized target/candidate SHA-256 | result |
|---|---:|---:|---|---|
| `_effects_initialize` | 80 / 80 | 9 | `efae2465adf619125cd126de1b205706c9b2ca9660accea5357a8a517bc57f7f` | exact |
| `_effects_information_get` | 118 / 128 | 8 | `8be527574267651b6417cfe08678f822db789753c64b64fa7b02607755e0fb2a` | exact |

The lane adds **198 meaningful bytes, 208 padded bytes, and 17 relocations**. `effects.obj` advances from 5/41 to **7/41 exact**, totaling **307 meaningful bytes, 352 padded bytes, and 31 relocations**. The object remains `NonMatching` against its 11,332 meaningful / 11,616 padded bytes and 549 relocations.

## Provenance and authenticated evidence

- January target object: `build/split/source/effects/effects.obj`, SHA-256 `b9a46dba146ee46c39600e5bf5b181e620e971c75644be2f2ff3d1475ff362b2`.
- Compiler: Microsoft Xbox XDK `xbox/bin/vc7/CL.Exe`, version `13.00.9254.1`, SHA-256 `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Stable pre-edit source: Git blob `bdb54a97a223726bfd274ccf9c85b81a32789a8c`, SHA-256 `1d70feee020074bcc4c6947327836e8a7a2cbbdb6704d02db7324e592150560a`.
- Resulting `source/effects/effects.c`: Git blob `4d4f076213521881a1cbd1573f0b81062edc10d8`.
- Preserved first candidate: `build/audit/effects_public_pair_first_candidate_20260828.obj`, SHA-256 `1fa54354241f65eb31348269b4ac3eae7385ed102397bfa8be3a293e74c175c3`, 3,320 bytes.
- Clean HCEA oracle: commit `c168af2e747d3095d9a29418ae401f3a39544863`.
  - `src/effects_initialize.c`: blob `0ad7eaf94a7db7ddcd4208323dbdc4b5782f6ff0`.
  - `src/effects_information_get.c`: blob `d1e17c70c2d25bb9e0fccaac808e7cfec102216d`.
- Stian full-history reference: commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`.
- Pastudan reference: commit `918af885935ec470a31256ecce9a977b12b01f80`.
- Stian/Pastudan source history attributes the January initialization guard to commit `8720b23a3`: allocate both public data arrays, then issue the immediate `"couldn't allocate effect globals"` error if either allocation failed. This filled the one statement omitted by the HCEA donor and agreed with the January target's string and relocation schedule.

The implementation uses only typed public owners and ordinary cdecl calls: `game_state_data_new`, `error`, `data_next_index`, and `datum_get`. Type evidence established `data_array.actual_count` at `+0x30`, the effect flags word at `+0x02`, stopped/invisible bits 3 and 4, and the three-short `effects_information` layout. Compile-time size checks retain the four-byte datum header and six-byte result structure. No raw-offset access, private ABI reconstruction, assembly, `volatile`, pragma, forced inline, synthetic anchor, or pointer/integer conversion was introduced.

Applicable campaign guidance and both earlier effects ledgers were read in full before implementation:

- `docs/exact_match_acceleration_playbook.md`
- `docs/matching_methodology.md`
- `docs/object_matching_logs/trees_obj_codex_checkpoint.md`
- `docs/object_matching_logs/effects_obj_jonas_lifecycle_wave.md`
- `docs/object_matching_logs/effects_obj_jonas_random_wrappers_20260826.md`

No `CLAUDE.md` or `AGENTS.md` exists in this checkout, its ancestors, or the
clean HCEA donor checkout. The canonical external campaign copies were still
read in full before emission and govern this lane:

- `work/halo/CLAUDE.md`, SHA-256
  `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
- `work/halo/AGENTS.md`, SHA-256
  `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`.

The earlier ledgers' Claude-worktree census did not find either selected body
under active Claude effects work.

## Strict ownership and relocation proof

`_effects_initialize` reproduced this nine-relocation schedule with zero addends:

- `+0x0B DIR32` `"effect"`; `+0x10 REL32` `_game_state_data_new`
- `+0x1C DIR32` `"effect location"`; `+0x21 DIR32` `_effect_data`
- `+0x26 REL32` `_game_state_data_new`; `+0x2C DIR32` `_effect_data`
- `+0x36 DIR32` `_effect_location_data`
- `+0x41 DIR32` `"couldn't allocate effect globals"`; `+0x48 REL32` `_error`

`_effects_information_get` reproduced this eight-relocation schedule with zero addends:

- `+0x04 DIR32` `_effect_data`; `+0x16 DIR32` `_effect_location_data`
- `+0x2A DIR32` `_effect_data`; `+0x32 REL32` `_data_next_index`
- `+0x42 DIR32` `_effect_data`; `+0x49 REL32` `_datum_get`
- `+0x5D DIR32` `_effect_data`; `+0x64 REL32` `_data_next_index`

The candidate contains seven selection-1 external `.text` COMDAT owners. Its only newly owned non-code sections are natural string COMDATs of 33, 16, and 7 bytes; these receive zero matched-data credit under campaign convention. `_effect_data` and `_effect_location_data` remain undefined public externals with value zero. The candidate introduces no `.data`, `.bss`, COMMON, or runtime-storage claim.

## One-shot discipline and regression evidence

An XDK `/Zs` parse-only check preceded the production build and emitted no object. Ninja's dry run showed exactly one production edge, `CL build\\base\\source\\effects\\effects.obj`. That edge was executed once, and its output was immediately preserved under the fingerprint above. There was no second code-producing compile.

The clean pre-edit regression manifest is `build/audit/effects_public_pair_preedit_manifest_20260828.json`, SHA-256 `442b1a4e71976b3de7c2362b39e83579dfbc88fce5688a116e46d3486f2cd77a`; its self-check was green before editing. The first-candidate no-build comparison classified both selected owners `NEWLY_EXACT` and reported `changed_nonexact: []`. Adding the two earlier COMDAT owners naturally shifted object-local section/debug/symbol inventory, so no exception was used to suppress generic inventory differences. Independent strict owner comparison proved all five inherited exact owners unchanged:

- `_effects_initialize_for_new_map`
- `_effects_dispose_from_old_map`
- `_effects_dispose`
- `_effects_disconnect_from_structure_bsp`
- `_local_random_direction3d`

The regenerated selected-object report contains 41 functions and exactly these seven ordinary 100% owners. Semantic audit evaluated the selected result with zero unit errors; admission audit remained 0 candidates / 0 contradicted / 0 revoked, parked audit remained 13 active / 0 stale / 0 invalid, and the tool suite completed 205 tests successfully. The integration coordinator separately reports the current combined full build, audits, tests, and board green with this source applied; cumulative campaign metrics belong to that integration checkpoint.

## Handoff state

This isolated worktree intentionally remains uncommitted and unpushed for coordinator integration. Its only tracked changes are:

- `source/effects/effects.c`
- `docs/object_matching_logs/effects_obj_jonas_public_pair_20260828.md`

Units, Vehicles, every Claude-active path, and all other object sources were preserved read-only.
