# MANIFEST - source/bink/bink_playback (round 2, completeness-critic route critic_0_bink_playback)

- Base: 5d3ca708 (the task named 7b9de585; HEAD has moved on by ledger-only commits, and the unit's files are unchanged: source blob aa25ad5a, symbols.json blob 885b3b3d). Tip: nothing committed. The packet is in `patches/`.
- Route: close bink's owner NAMING blocker (`_code_001b5850`) with a first-party name and file-static storage. The empty stub is emitted only because it is used (static-COMDAT law), so the source needs one byte-inert call. This follows the R2-1 hardware_geometry MoveResourceMemory precedent.
- Outcome: **ADMISSION_FIX_PROPOSED**, zero credit. The object stays NonMatching. After this packet, the q3 owner ruling on `_bink_query_analog_controller_buttons` is its **only** blocker. A lab composition proves this: packet + q3 = 24/24 and OBJECT AUDIT PASS.

## Production changes

Apply in order. `git apply --check` is clean at 5d3ca708 against both the worktree and `--cached`, for each patch alone, for 01+02, and for 01+02+03. The sha256 values are in `patches/patches_sha256.txt`.

| # | file | patch | what |
|---|---|---|---|
| 01 | source/bink/bink_playback.c | `patches/01_bink_playback.c.patch` (aa25ad5a..94b281d9) | `void code_001b5850(void)` becomes `static void bink_decompress_audio_frame(void)` in both the prototype and the definition. The body is unchanged: `return;`. Adds one call `bink_decompress_audio_frame();` just before `bink_decompress_video_frame();` in `bink_playback_render`, with a 2-line disclosure comment. The header "symbols in this file" line for 001B5850 now uses the real name. |
| 02 | config/symbols.json | `patches/02_symbols.json.patch` (885b3b3d..dc1dc066) | Row 7388 (file_offset 1792080 = 0x1b5850), edited in place: `_code_001b5850` becomes `_bink_decompress_audio_frame` with `"static": true`. No other row changes, and the file is not re-serialised. |
| 03 (optional hygiene) | both files | `patches/03_optional_hygiene_bink_playback_update__internal.patch` (94b281d9..cc27b290; dc1dc066..65510792) | Renames `bink_playback_idle` to the first-party `bink_playback_update__internal` (prototype, definition and 2 calls). Changes the header listing line 001B5E30 and symbols.json row 7400 (0x1b5e30), which is already static. It requires 01+02 first. |

- **Landing:** 01 and 02 must land atomically, followed by a **csplit-only regeneration**.
  - Emulation (`mk_cfg.py`, `split_ctl`/`split_A`/`split_H`): split_ctl matches build/split on 833/833 objects. split_A changes only `bink_playback.obj`, and only symbol 18, which goes from (`_code_001b5850`, storage 2) to (`_bink_decompress_audio_frame`, storage 3).
  - With 03 (split_H), the only further change is symbol 42, `_bink_playback_idle` to `_bink_playback_update__internal`.
- Mixed states are not safe:
  - Production against split_A gives 22/1/1 (stub UNWRITTEN) and object_audit FAIL(3).
  - The candidate against build/split also gives 22/1/1 and FAIL(3).
- No header changes, so there are no consumers. No other config file names either symbol; `contribs.json` is keyed by offset and is unchanged. No park, rejection, semantic or ownership row refers to them. `config.json` status stays NonMatching.

## Source evidence per changed construct

1. **Name. First-party linker maps dated 2001-09-25; data only.**
   - `astra-inputs/earlier-map-archives/2001-09-25/cachebeta.map`: line 19268 heads "Static symbols". Line 19359 reads `0001:001b3090 _bink_decompress_audio_frame 005b3470 f bink_playback.obj`.
   - In that map it sits between `_bink_free@4` (001b2fd0) and `_bink_draw_frame` (001b30a0), so it is 0x10 bytes. January has the same spacing: `_bink_free@4` 0x1b5790, stub 0x1b5850 (0x10), `_bink_draw_frame` 0x1b5860.
   - The same directory's retail `cache.map` line 13674 is a Static symbol: `@bink_decompress_audio_frame@0 0054d870`.
   - The atlas has it twice:
     - 7eacac85 (Sept cachebeta xbe), line 5498: `_bink_decompress_audio_frame` at 0x1c4090.
     - 6455066470 (Sept retail exe), line 5992: at 0x54d870.
   - January's own atlas, 4cc87b45, has no record at 0x5b5850 because of the exact-tier size floor.
2. **Body and use** (`verify_bytes.py` / `verify_bytes.txt`). All three builds have `c3` followed by 15 `90` bytes. None has an E8/E9 call or an absolute reference: Sept xbe 0x1c4090, Sept retail 0x54d870, January cachebeta.exe 0x5b5850.
   - The function is never called out of line and never has its address taken, yet all three builds emit it. So it is a **used** file-static, inlined at every site.
   - Lab `lab_strip_call.c` shows the law: a static with no call is not emitted.
3. **Storage.** cachebeta.pdb publics have nothing at RVA 001B5850, and both Sept maps list it under Static symbols. So it is `static`.
   - Lab `lab_extern_call.c`: without `static` the bytes are identical, but object_audit reports a storage DIFF of 3/2.
4. **Not attested (disclosed in the source comment).** The call's position and site count are unknown.
   - Labs `lab_call_after.c` (after the video call) and `lab_two_sites.c` (an extra call in `bink_playback_start`) give non-debug sections identical to cand. Only `.debug$S` changes, by the length of the file path.
   - There is no /Od attestation. The /Od build `halo_cache_symbols.exe` is the PC binkw32 port: delay-import IAT at 0xf5a200 and one monolithic caller at 0x4d1a50..0x4d1efd. HCEX has only bink stubs.
   - Assert anchors carry literal line numbers, and render lies before the first anchor (jan 414), so anchor drift cannot place the call either.
   - The name (audio frame decoded next to the video frame) and the render function's decode block make this spot the natural one.
5. **Hygiene 03.** Sept `cachebeta.map` has the static `0001:001b3670 _bink_playback_update__internal`; the next record is `_bink_playback_render` at 001b3710, so it is 0xa0. January 0x1b5e30 is also 0xa0 and is followed by `_bink_playback_render` at 0x1b5ed0. Retail has `@bink_playback_update__internal@0`.
   - `verify_hygiene.py` compares the Sept body with the January section. The 52/52 mnemonic sequences are equal, and 115/160 bytes are equal; the rest are addresses.
   - Sept's callers are tail `jmp`s from `bink_playback_render` (0x1b3764) and `bink_playback_update` (0x1b3ba9). These are exactly January's two callers.

## Target vs candidate (cand.obj vs emulated split_A; `fn_table.txt`, `gate_cand_vs_splitA.txt`)

`gate.py --all --forbid-emitted-symbol _point_from_line3d` passes the guard. Result: **23 exact, 1 residual, 0 unwritten**. The residual is the parked q3 row, which is unchanged: 96 vs 80 bytes, 1/1 relocs, same candidate sha eec8d318 as production.

| function | size | relocs | normalized sha (16) | row |
|---|---:|---:|---|---|
| _bink_decompress_audio_frame (static) | 16 | 0 | 499f1f307c1cb989 | EXACT (was `_code_001b5850` external) |
| _bink_playback_render | 96 | 12 | 0670afcbb0884c06 | EXACT, bytes identical to production (the call inlines to nothing) |
| _bink_query_analog_controller_buttons | 80 vs 96 | 1/1 | c88d131b vs eec8d318 | residual, parked q3 (owner ruling) |
| the other 21 functions | = | = | = | EXACT, identical to production |

- **Whole object.** cand.obj matches production orig.obj on 77/77 sections, and no non-debug section differs by a single byte. The only change is symbol 43, name and storage (`symdiff_orig_cand.txt`).
- **Data.** All January .bss, .data and .rdata sections are `ok` in object_audit. objdiff 3.3.1 gives data 1701/1701.
- **Symbols.** object_audit cand vs split_A gives FAIL(1), and that one is the q3 .text row only. All 59 January symbols have 0 differences. Production vs build/split is the same FAIL(1).
- **PDB storage.** Production has 1 disagreement (`_code_001b5850` split 2, ours 2, PDB-public False). The candidate has **0**, and so does hyg.
- **Surplus.** surplus_identity finds 0 candidate-only code COMDATs. provider_link passes 6 rows (3 SDK tables, 3 `__real` literals) in both orders, and `--baseline=production` finds no new surplus.
- **objdiff 3.3.1 mini report** (sha1 3130e428, `objdiff_proj/report.json`):

  | pairing | code | functions | data |
  |---|---|---|---|
  | production vs build/split | 3733/3803 | 23/24 | 1701/1701 |
  | cand vs split_A | 3733/3803 | 23/24 | 1701/1701 |
  | production vs split_A | 3732/3803 | 22/24 | 1701/1701 |
  | cand vs build/split | 3732/3803 | 22/24 | 1701/1701 |
  | hyg vs split_H | 3733/3803 | 23/24 | 1701/1701 |

- **.text emission order.** The candidate order matches production, with the stub in the same slot. Production's order already differs from January's address order. That is pre-existing, is not an object_audit criterion, and this packet does not change it (`text_order.txt`).
- **Lab, not landable** (`lab_with_q3_NOT_LANDABLE.c` = cand + round-1 L3 q3 else-if arms): 24/0, OBJECT AUDIT PASS, pdb_storage 0. So after this packet, only the q3 ruling blocks admission.

## House-rule review

- The `code_` placeholder is replaced by the first-party name. The prototype sits in the file's own static prototype block, and the definition keeps its position immediately before `bink_draw_frame`, matching the Sept map's static order.
- `void` is on its own line, the body ends with an explicit `return;`, and no header is changed.
- **Strip test.** Removing the call un-emits a January function, so the call is load-bearing through existence and is not decoration. Removing `static` brings back a storage DIFF against the map and PDB.
- There is no filler declaration, cast, pragma, asm, volatile or dummy local. The declared-name count is unchanged (rename only), and the TU is otherwise byte-identical.
- The pre-existing `__asm { int 3 }` lead is the same in production and candidate, and this packet does not touch it.
- /Zs /W3 gives the same 14 warnings in production and candidate (C4018 x2, C4146, C4244 x11).

## Tests run

- `git apply --check`, worktree and `--cached`, singly and together; post-image blob hashes checked with `git hash-object --no-filters`.
- csplit emulation for ctl, A and H, plus splitdiff and symdiff.
- `gate.py` against build/split, split_A and split_H.
- `object_audit` (candidate, production and both mixed states), `pdb_storage`, `surplus_identity`, `provider_link` (both orders plus the production baseline).
- The objdiff 3.3.1 mini report, `fake_match_scan`, and the /Zs /W3 census.
- Byte and reference scans of the Sept xbe, the Sept retail exe and the January exe.

**Integrator:** apply 01+02 (and 03 if wanted), regenerate csplit only, then run a full ninja, the stable sweep, parks/admission, pytest and the diff check.

## Credit

- Code, functions and data are credit-neutral: 3733 B / 23 functions / 1701 data before and after.
- Admission is **none yet**. This packet removes one of the two remaining blockers: the owner naming item plus the PDB storage disagreement.
- Held: the q3 ruling on `_bink_query_analog_controller_buttons` (parked; owner decision; not part of this packet).
- Reopen (comment-only): only if a first-party January-dated source shows the call site(s) of `bink_decompress_audio_frame`.
