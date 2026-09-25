# MANIFEST: source/effects/weather_particle_systems, zero-credit storage packet (r2w2)

Worker label: `r2w2_weather_particle_systems`. The unit was claimed before any candidate edit and released at the end.
Tree: HEAD `434f0151`, which is `05255584` plus one docs-only ledger commit. `source/effects/weather_particle_systems.c` is blob
`d2aae8ff` and `config/symbols.json` is blob `f9abb17d` at both commits. The worktree is clean. The only untracked path is research/fifty_objects_r2_20260924/.
No tracked file was edited. I did not run ninja, configure, or any git mutation in the worktree.

## 1. Packet

| file | content | status |
|---|---|---|
| `patches/01_one_over_char_max_external.patch` | the ASSIGNED construct. One token in source/effects/weather_particle_systems.c:309: `static real const one_over_char_max = 1.f/255.f;` becomes `real const one_over_char_max = 1.f/255.f;` | **proposed for landing** (precedent-covered) |
| `patches/02_weather_particle_system_globals_static.patch` | OPTIONAL. This is a second storage disagreement found while auditing, outside the assigned construct. It makes `weather_particle_system_globals` static in the .c and adds `"static": true` to symbols.json row 22981 | measured, integrator's choice (same precedent; section 6) |

`storage_only.patch` is byte-identical to 01. Both patches are LF with git headers. `git apply --check` is clean in the worktree for 01, for 02, and for 01+02. In an isolated `core.autocrlf=true` repo, applying the two in either order gives `candB.c` and `emu/config/symbols.json` byte for byte. Applying 01 alone gives `cand.c`, which is byte-identical to round-1 `prod_storage_only.c`.

## 2. Precedent (governing rule) and conditions

**Governing rule: storage follows cachebeta publics.** The worker brief (WORKER_BRIEF_R2 s3) says:
"cachebeta.pdb publics (January storage truth: public = external, absent = file-static)". The canonical commits
apply the same rule. 2535f91a: "cachebeta.pdb publics are the independent truth". 7d92f97c, cdc8ebd3 ("collision_bsp_usage_times is
file-static (cachebeta publics)"), 6e3e2d35 ("839 January-static storage flags"), and 05255584 ("12 typecast converters are
static per cachebeta publics"). The **exact same construct in the external direction** has already landed in canonical 6e3e2d35 (source/hs/hs.c).
There, `static real const _hs_type_real_default= 0.0f;` became `real const _hs_type_real_default= 0.0f;` because
`__hs_type_real_default` is a cachebeta public in .rdata (0x25D7D4, section 0013). `_one_over_char_max` is public at 0x25AAFC, also in section 0013.

| condition (as applied by the precedent commits) | evidence | holds |
|---|---|---|
| C1. The symbol's cachebeta public membership decides storage | `scratch/tools/cachebeta_publics.txt:5686` `PublicSymbol: [0025AAFC][0013:00017EDC] _one_over_char_max`. That makes it external | YES |
| C2. Independent corroboration / January split agrees | January split storage = 2 (external). symbols.json:14570 has no `static` flag. HCEX.pdb has `Global, Type: const float, one_over_char_max` plus a PublicSymbol "export is DATA" (`hcex_oocm.txt`) | YES |
| C3. No duplicate-definition exposure / ownership | build/split: the ONLY definer is weather_particle_systems.obj, and no other January object references it. build/base (621 objects): the only definer is weather, with no other reference. It is January-owned, so it is not surplus | YES |
| C4. Byte- and credit-neutral, zero regressions | control.obj == build/base obj (61/61 sections, 144/144 symbols). cand vs control: 61/61 sections equal (section_infos_equal AND raw bytes) with exactly 1 symbol-table difference (`_one_over_char_max` storage 3 -> 2). gate rows are identical (19 EXACT + the pre-existing render residual). objdiff 3.3.1 mini project: prod == cand (4114/5779, 19/20, 949/949) | YES |
| C5. Authentic name, no invented construct, owner-file placement | The name is the cachebeta public name and the HCEX global name. The edit is a storage keyword in the owning .c | YES |
| C6. Required form | Precedent commits use a plain storage-keyword edit with no disclosure comment (6e3e2d35 hs.c, cdc8ebd3 collision_bsp.c, 2c7f73b9). No symbols.json change is needed because the row is already external. No header declaration is needed because there is no consumer. The canonical sibling `real const _one_over_255` (ui_widget.c:1467, cachebeta public) also has no header extern | YES |

Every precedent condition holds. The precedent covers zero-credit storage fixes on objects that stay incomplete (cdc8ebd3: "objects stay
incomplete on residual functions and held owner items"). It does NOT claim whole-object admission, and this packet does not either.

## 3. Functions (target vs candidate; padded size / reloc count / normalized sha8)

See `fn_table.txt`. The 20 January functions are 19 EXACT plus `_weather_particle_system_render` 1680 residual [sha] (target 823416f9, candidate dcf592bb,
47/47 relocs). This is **identical to production** (gate_control.txt == gate_cand.txt). The render residual is the pre-existing Lane D class-I
owner item and is not touched here.

## 4. Data (coff_compare, `datacmp_cand.txt`)

10/10 January data sections are EQUAL (section_infos_equal) against build/split: 8 .rdata (including `_one_over_char_max` at 4 B and 7 string literals),
.data `_weather` (1 B), .bss `_weather_particle_system_globals` (628 B). `tinfo --data`: control == cand. report.json data is already 949/949.

## 5. Symbols / storage / surplus / links

- object_audit: production FAIL(2) (render + `_one_over_char_max` storage 2/3). cand FAIL(1) (render only), **30 January symbols, 0 differ**.
  The surplus list is unchanged.
- pdb_storage: production has 2 disagreements. cand (same logic, `pdb_storage_cand.txt`) has 1 left, `_weather_particle_system_globals`
  (split 2, ours 2, NOT public; see section 6).
- surplus_identity: production and cand are identical. There are 14 candidate-only COMDATs: 13 IDENTICAL, and `_real_local_random` is DIFFERENT (pre-existing).
- provider_link on cand: FAIL(1), only `_real_local_random` against effects.obj in both orders. This is identical to production (the NODUP/header-body
  conflict inside the Codex real_math reservation). `--baseline=build/base`: "no surplus external definitions: PASS".
- /W3 (CL /Zs, production cflags): control 15 warnings = cand 15 = candB 15, the identical set. fake_match_scan: 0 leads on control, cand, and candB.

## 6. Optional sub-packet 02 (a second disagreement under the same rule; NOT part of the assigned construct)

`_weather_particle_system_globals` (0x43D590, .bss 628 B) is **absent** from the cachebeta publics. The neighbours at 0x43D588/0x43D589 are public, so the
region is covered. HCEX.pdb records `weather_particle_system_globals` as **"File Static"** (`hcex_wpsg.txt`). No other January object
references it, and no other base object defines or references it. The January split currently says external only because
symbols.json lacks the flag. Ledger docs/object_matching_logs/weather_particle_systems_obj_delete_reconciliation_20260907.md left
this linkage open ("Absence of external references does not establish original linkage"), and the PDB/HCEX evidence settles it.
Form: the same as cdc8ebd3 (`static struct collision_bsp_usage_times ... = { 0 };` plus symbols.json row 23117 `"static": true`).

Emulated split: I copied config/ to `emu/config`, edited ONLY row 22981 in place (CRLF kept, JSON parses), and ran `build/tools/csplit.exe -i cachebeta.exe
-p emu/config -o emu/split` (exit 0). `diff -rq build/split emu/split` shows ONLY weather_particle_systems.obj, and there it is a single symbol-table difference
(storage 2 -> 3). All 30 sections are identical.

candB (01+02) against the emulated split (`emu_audit_candB.txt`): 19 EXACT + render [sha] (unchanged), object audit 30/30 symbols with 0 differ
(FAIL(1) = render only), **pdb_storage 0 disagreements**, data 10/10 EQUAL. candB vs cand: 61/61 sections identical, 1 symbol storage
difference. objdiff 3.3.1: candB vs emulated target is 4114/5779, 19/20, 949/949, and every row is identical to production. 01 alone against the emulated split would show
the expected symbol DIFF (storage 3/2), so 02 must land as the source+symbols.json pair (atomic). The surplus and provider link are unchanged (sections are identical).

## 7. Credit

Code +0, data +0, functions +0, objects +0. This is a zero-credit storage correction. Admission audit effect: 01 takes object_audit symbols from 29/30 to 30/30
and pdb_storage from 2 to 1. 01+02 (with the regenerated split) takes pdb_storage to 0.

## 8. Held / not complete (reopen criteria)

The object stays **NonMatching and not admissible**:
1. `_weather_particle_system_render` (1680) needs the owner ruling on Lane D class I (the /Od-attested dead `short unused_count = 0;` at /Od slot [ebp-0x24]).
   Reopen on an owner YES by applying research/fifty_objects_20260925/w/weather_particle_systems/production.patch. Its storage hunk equals 01, so apply 01 first and
   then the body-only remainder, or apply that patch in place of 01.
2. `_real_local_random` surplus: this conflicts with effects.obj's selected copy (FAIL in both orders). The fix sits in real_math.h, which is RESERVED for Codex.
   Reopen when the reservation is released, with a full-board sweep.

## 9. Tests run (all outputs are in this slug)

gate --all (control, cand, candB); objcmp.py (base vs control, control vs cand, cand vs candB, split vs emu split); object_audit (base, control,
cand); emu_audit.py (cand and candB vs emu split, cand vs build/split); pdb_storage (production + candidate logic); datacmp.py; tinfo --data
(split/base/control/cand); surplus_identity (production + candidate logic); provider_link (cand, base, cand --baseline); w3.py /W3;
fake_match_scan; HCEX DIA2Dump (-sym one_over_char_max, weather_particle_system_globals); objdiff-cli 3.3.1 mini project (od/); git apply
--check plus isolated both-order apply.
