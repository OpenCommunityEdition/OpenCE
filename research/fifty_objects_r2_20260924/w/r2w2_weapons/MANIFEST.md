# MANIFEST - source/items/weapons, round 2 wave 2 (r2w2_weapons)

- **Base / tip:** worktree HEAD 434f0151 (05255584 plus the R2 ledger doc). Nothing is committed; the packet is in `patches/`.
- **Kind:** zero-credit ADMISSION fix. The storage packet was pre-cleared as COVERED_BY_PRECEDENT ("storage follows cachebeta publics", 05255584).
- **Claim:** `source/items/weapons`, label `r2w2_weapons`.
- **Object status after landing:** still NonMatching, at 78/79. `_weapon_place` is the only remaining failure: it is the owner-held class-F long-return item, and the descriptive `_data_00307140` name is also owner-held. Neither is touched here.

## Production changes (land together, then regenerate the split)

| file | patch | change |
|---|---|---|
| source/items/weapons.c | `patches/01_weapons.c.patch` (index 72e3597..c0baa42, 2 hunks, 2+/2-) | Adds `static ` to the `weapon_magazine_finish_reload` prototype (l.449) and definition (l.1382). Nothing else changes. |
| config/symbols.json | `patches/02_symbols.json.patch` (index f9abb17..17bd4c8, 2 hunks, 11+/11-) | Adds `, "static": true` in place to lines 4013-4018, 4023, 4030-4031, 4047 and 4054. Row order and all other text are unchanged, and the file is not re-serialised. |

- `git apply --check -v --whitespace=error` at 434f0151 is clean, both singly and together.
- Applying the patches to a mirror of the HEAD blobs reproduces `cand.c` and `sym_s.json` byte-for-byte.
- `final/weapons.c` and `final/symbols.json` are the CRLF working-tree post-images.
- The r1 `symbols_static.diff` was a plain line diff. Rows 4013-4054 still match HEAD. The line diff of the rebuilt file (`symbols_s.linediff`) equals the r1 diff once CR is normalised.
- **Joint landing is required.** Measured with object_audit:

  | state | result |
  |---|---|
  | production vs current split | FAIL(11) |
  | source patch only | FAIL(12): `finish_reload` becomes split 2 / ours 3 |
  | symbols.json only | FAIL(2): `finish_reload` becomes split 3 / ours 2 |
  | both patches + regenerated split | FAIL(1): `_weapon_place` .text only |

- **No other tracked file changes.** No header is touched, no config.json status changes, and there are no parks, rejections or semantic entries.
- **Split regeneration:** ninja regenerates `build/split` from symbols.json. The csplit emulation changes only `source/items/weapons.obj`.

## Precedent (governing ruling and its conditions)

**Governing precedent: "storage follows cachebeta publics".** Canonical landed it in 05255584 (R2 batch 1b, integrated by the owner's lane):

> "12 typecast converters are static per cachebeta publics (provider link FAIL(12) -> PASS)."

In the same commit, 13 hardware_geometry XDK wrapper rows "go static". 7979cf8f made three D3DINLINE wrappers static with the note "none is a cachebeta public".

The rule the precedent applies is WORKER_BRIEF_R2 s3: "cachebeta.pdb publics (January storage truth: public = external, absent = file-static)". The same method appears in `docs/object_matching_logs/claude_data_ownership_closeout_20260924.md` (Storage truth, point 3) and in `claude_object_closure_20260924.md`, where action_flee and action_guard got symbols.json `"static": true` per cachebeta publics with no cross-TU references.

### Required form

The approved hs_runtime packet (review_r2_hs_runtime_storage_12_converters, APPROVE) has this form:

- a bare `static` storage keyword on the existing prototype and definition in the owning .c, next to the other static prototypes;
- no comment, rename or new declaration;
- symbols.json rows flagged `"static": true` in place, in the existing row format and order (memory law: never re-serialise symbols.json).

This packet uses exactly that form.

### Conditions and evidence

| # | condition (as the precedent was applied and reviewed) | evidence here | holds |
|---|---|---|---|
| P1 | Each symbol is absent from the cachebeta.pdb publics. | `scratch/tools/cachebeta_publics.txt`: none of the 11 appears by name, and no public sits at any of their RVAs (file_offset = RVA: 0xEAB30, 0xEAB80, 0xEABD0, 0xEAC20, 0xEAC60, 0xEAC90, 0xEAEF0, 0xEB210, 0xEB230, 0xEC300, 0xEC730). The controls `_weapon_place`, `_weapon_new`, `_weapon_overcharged` and `_weapon_set_total_rounds` are present, which confirms the method. | yes |
| P2 | There is no cross-TU consumer, so the change is link-safe. | `xref_scan.py` checked 1,454 objects (build/split + build/base): 0 references or definitions of the 11 outside weapons.obj. Only `weapons.c` names them (callers of `finish_reload` at l.1989 and l.2951); no header declares any of them. | yes |
| P3 | The change is byte-inert and claims no credit (strip test). | Production vs `head.obj`: 0 diffs. `head.obj` vs `cand.obj`: 154/154 sections equal; the only symbol diff is `_weapon_magazine_finish_reload` storage 2 -> 3. Gate rows are identical. All 79 function fingerprints are unchanged in split_ctl vs split_s. objdiff 3.3.1 is identical (13277/17595, 75/79, 2052/2052). | yes |
| P4 | The precedent's exact form is used: `static` on the owning-.c prototype and definition, in-place symbols.json flags, names unchanged, no new declaration. | Patch 01 is two `static ` prefixes. Patch 02 appends `, "static": true` to 11 rows. The names are authentic (HCEX and the Sept-2001 map use the same names). The declaration count is unchanged. | yes |
| P5 | Split regeneration is confined to the unit, and after it the audit symbols agree. | csplit (build/tools/csplit.exe v0.0.2): split_ctl equals build/split for all 833 objects. split_s vs split_ctl: 1 object differs (weapons.obj) by exactly 11 bytes, the 11 storage classes. object_audit cand vs split_s: 97 January symbols, 0 differ. | yes |
| P6 | Surplus and provider link are unchanged or better. | surplus_identity: 17 COMDATs, 0 not identical, the same as production. provider_link: 31 rows PASS in both orders, the same as production; `--baseline` shows no new surplus. | yes |
| P7 | The change is not a held class and carries no header, COMDAT or P1 change. | Only the storage keyword and flags change. The class-F long return and `_weapon_update_section` naming stay held and untouched. | yes |

### Independent first-party corroboration (beyond the precedent's minimum)

- **Sept-2001 cachebeta.map** (`.../astra-inputs/earlier-map-archives/2001-09-25/cachebeta.map`, sha256 b63d3090...):
  - All 11 appear under the `Static symbols` header (l.19268), at l.20331-20358 (`f weapons.obj`). The controls sit in the public section (l.2299-2316).
  - The split-external names that the map lists as static are exactly these 11, so the packet is complete for functions.
- **HCEX.pdb** (DIA2Dump, `hcex_*.txt`):
  - `static function: true` for 7 of the 11: magazine_state_change_ok, get_effect_object_index, get_owner_object_index, effect_new, set_state, magazine_finish_reload (`static void weapon_magazine_finish_reload(long, short)`) and reset.
  - trigger_get, magazine_get, busy and state_interruptable have no HCEX record.
  - The controls weapon_place and weapon_new are public (SymTag 0xA).

## Target vs candidate per function

- `gate.py --all` against the emulated split_s: `== exact 78 residual 1 unwritten 0`. The candidate gives the same result against build/split (`gate_cand_vs_split_s.txt`, `gate_cand_vs_split.txt`).
- The only residual is `_weapon_place` (208 padded, `[sha]`, the class-F owner item). The other 78 rows are EXACT, and each row's padded size, normalized sha and relocation count equal January's (see the gate files).
- The 11 functions whose storage changes are all EXACT before and after, with unchanged fingerprints:

  | function | padded size |
  |---|---:|
  | `_weapon_trigger_get` | 80 |
  | `_weapon_magazine_get` | 80 |
  | `_weapon_busy` | 80 |
  | `_weapon_magazine_state_change_ok` | 64 |
  | `_weapon_get_effect_object_index` | 48 |
  | `_weapon_get_owner_object_index` | 64 |
  | `_weapon_effect_new` | 240 |
  | `_weapon_state_interruptable` | 32 |
  | `_weapon_set_state` | 432 (sha 667efc08, 21 relocs; the semantic_matches pin still holds) |
  | `_weapon_magazine_finish_reload` | 224 |
  | `_weapon_reset` | 400 |

## Data, symbols, surplus

- **Data** (coff_compare via object_audit): all 18 January data sections are ok. That is 17 `.rdata` literals/reals plus `.data` `_data_00307140` (1,536 B). objdiff data is 2052/2052.
- **Symbols:** object_audit cand vs split_s gives FAIL(1): the `_weapon_place` .text only; 97 January symbols, 0 differ.
- **pdb_storage:** production has 12 disagreements; the candidate vs split_s has 1. The remaining one is `_data_00307140`: external in both split and ours but not a PDB public. It is the owner-held naming item B and out of scope here.
- **Surplus:** 17 code COMDATs, all identical to January's selected copies. There are also 14 literals/reals and `.drectve`, all pre-existing. provider_link is PASS in both orders.

## House-rule review

- The storage keyword only; the names are authentic, not placeholders.
- The static prototype stays in the owning .c, next to the other file-static prototypes (rule 9 does not apply to statics).
- There are no casts, parentheses, locals, pragmas or comments.
- STRIP TEST: removing `static` restores production bytes exactly. The keyword buys no byte; it is a storage-truth fix with three sources:
  - cachebeta publics absence;
  - the Sept-2001 map "Static symbols";
  - HCEX `static function`.
- CL /Zs /W3 output is identical (20 lines); no storage warning is introduced. fake_match_scan finds 0 leads.

## Tests run

- `git apply --check` (singly and together, `--whitespace=error`);
- mirror post-image equality;
- csplit control and static emulation (833/833; 1 object differs);
- `gate.py --all` against build/split and split_s, for head and cand;
- object_audit in 4 pairings;
- pdb_storage;
- surplus_identity;
- provider_link, both orders and `--baseline`;
- xref scan (1,454 objects);
- the semantic_matches pin check;
- objdiff 3.3.1 mini report;
- /W3 census;
- fake_match_scan.

**Integrator:** apply both patches, run a full ninja (it regenerates the split), then run the stable sweep, parks/admission, pytest and `git diff --check`.

## Credit

- Code, functions, data and objects are all 0; nothing is credited.
- This removes 10 object_audit symbol rows (production FAIL(11) -> FAIL(1)) and 11 pdb_storage disagreements (12 -> 1).
- **Held, with reopen criteria:**
  - `_weapon_place`: owner ruling on the class-F authentic long return (the r1 packet p2 with its BUG comment).
  - `_data_00307140` -> `blurred_permutation_names` + `weapon_update_section`: the naming owner item B, which HCEX only partly authenticates.
  - On a yes to both, the object completes with the r1 p1+p2+p3 packets on top of this one.
