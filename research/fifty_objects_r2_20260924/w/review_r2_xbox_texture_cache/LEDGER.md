# review_r2_xbox_texture_cache LEDGER (adversarial review of r2w1 OBJECT_COMPLETE_CANDIDATE)

Worktree HEAD b7c1b7d9 (source/config/tools == f6d00a8c), git status clean at start.
Read: WORKER_BRIEF_R2 0-9, worker MANIFEST/LEDGER, patches 01-04, round-1 REVIEW (approve=False, blockers a-f),
docs/campaign_house_rules.md, 20260904 steal_memory owner-reconciliation ledger (bitmap schema veto).

## P0 git apply --check at current tree
- combined exit 0; 01 exit 0; 04 exit 0; 02 and 03 standalone FAIL (they depend on 01 context; sequential by design).
- Sequential git apply (non-repo mode, LF HEAD blobs) 01->02->03->04 == combined (diff -r clean); all 7 source files
  == worker sF tree after LF normalisation; .c == cand_P3.c; seq symbols.json == patches/symbols.json byte-exact.
  symbols.json diff vs working copy: exactly rows 7239, 7242-7245, 7250 (6 rows) as claimed.
## P1 first-party names (atlas)
- 7eacac85 (Sept-2001 map tier): 21 xbox_texture_cache.obj entries (worker said 22; it is 21 + 4 unlisted XDK wrappers = 25),
  ALL 21 land on the patched symbols.json names at constant delta 0xE900 (scripted check). Includes
  _texture_cache_name_block_proc 0x1ae660, _compare 0x1ae7b0, _texture_cache_initialize_hardware_format 0x1ae920.
- 4cc87b45 (January exact tier): 12 entries, all match incl. _compare 0x5ae7b0. -> names authentic.
## P2 HCEX bitmap_data (re-run DIA2Dump -type bitmap_data -> hcex_bitmap_data.txt)
- 0x14 short mipmap_count, 0x16 short mipmap_pad, 0x18 long pixels_offset, 0x1C long pixels_size, 0x20 long tag_index,
  0x24 long cache_block_index, 0x28 void* hardware_format, 0x2C void* base_address: patched bitmap_group.h == HCEX exactly.
## P3 shadow sweep (own rsweep.py; tctl = copy of worktree source+libs, tcand = tctl + seq overlay, CRLF)
- 612 build.ninja TUs each. ctl vs build/base: 612/612 identical (harness reproduces production).
- cand vs ctl: 611 identical; ONLY xbox_texture_cache differs (3 .text sections + .debug$F with relocs to renamed statics,
  symtab name changes). No park can move (all other objects byte-identical incl. bink_playback, bitmaps, bitmap_group,
  render_debug, decals, hud_weapon, units).
- Consumers from /showIncludes: texture_cache.h 14 (ctl) -> 15 (+render_debug); bitmap_group.h 45 -> 46 (+xbox_texture_cache);
  new xbox_texture_cache.h 1 (xbox_texture_cache.c). Lists == worker's consumers_*.txt.
## P4 emulated split (own): emu/config = config + seq symbols.json + seq config.json; csplit rc 0;
  diff -rq build/split emu/split -> ONLY source/cache/xbox_texture_cache.obj differs.
## P5 strict compare
- cand_review.obj (compiled from tcand) vs emu split: 25/25 EXACT, storage equal on all 25 (XDK wrappers 3/3 now).
  vs production split: 19 exact / 3 residual (reloc names) / 3 unwritten (renamed), as expected.
- gate.py run literally in a gate root (gr/: copies of build.ninja, tools, tcand/source, xbox/include, emu split obj):
  --source source/cache/xbox_texture_cache.c --all --forbid-emitted-symbol _point_from_line3d -> guard passed,
  exact 25 residual 0 unwritten 0 (gate_final_emu.txt).
## P6 whole-object audit on emulated split (gate root gr/: unmodified copies of scratch/tools scripts; build/split=emu split,
   build/base=production objs + cand_review.obj)
- object_audit: PASS; all 42 January-owned sections ok (15 strings, _bitmap_d3d_format_tables 144, .bss 5656, 25 .text); 46/46 symbols.
  Surplus: .drectve, 10 pooled literals, 3 XDK tables, 4 __real, _add_vectors3d/_dot_product3d (unchanged from production).
- XDK table surplus precedent: 64 Matching objects carry the same 3 SDK tables candidate-only (census over build/base) -> not a blocker.
- pdb_storage (emu): 0 disagreements (production: 3, the XDK wrappers, PDB-public False). Mappers + debug_render ARE PDB-public.
- surplus_identity (emu): 2 COMDATs IDENTICAL (dot_product3d vs action_charge, add_vectors3d vs actor_combat).
- provider_link (real root, cand_review.obj): SELECTED-PROVIDER LINK PASS, 19 rows, both orders. git status still clean.
- report.json today: code 3457/3457, data 6344/6344 for the unit.
- Name census (split/base/cand): no other object defines/references any renamed or storage-flipped name; only render_debug
  references _texture_cache_debug_render (UNDEF).
## P7 /W3 warnings (CL /Zs /W3, ctl vs cand): identical sets except render_debug loses C4013 texture_cache_debug_render (fixed).
  bink_playback, bitmaps, xbox_texture_cache: same warnings (only pre-existing).
## P8 whole-TU admission review
- TU-local bitmap type/format/flag enums: same state as Matching bitmaps.c, bitmap_extract.c, bitmap_utilities.c (each has TU-local
  copies) -> accepted precedent, not blocking (but the 20260904 ledger's veto is superseded only by that precedent; owner note).
- HCEX texture_datum (PC analogue of the datum): identifier, request_index, available, postprocessed @5, bitmap @8, texture @0xC;
  NO pad member. Candidate struct xbox_texture_cache_texture keeps `byte reserved006[2];` (pre-existing): an explicit pad member
  in a TU-private struct = brief sec.9 admission blocker; round-1 precedent (player_profile, game_engine) removed such pads.
## P9 focused-header justification re-measured (lab tB, deleted after): texture_cache.h + debug_render + BOTH mapper protos,
   14 consumers vs ctl: decals.c .text 400 (decals_delete_permanent_from_cluster) DIFF and hud_weapon.c .text 2624 DIFF
   (plus xbox_texture_cache renames) -> worker's regression claim reproduced; debug_render-only form (tcand) is identical.
   Precedent for a focused owner header: math/real_math_planes.h (commit 6c7ab19d), same comment pattern.
## P10 pre-existing constructs, strip tests (lab, gate root, vs emu split)
- `struct bitmap_group *bitmap_group` named local inlined away: bitmap_new EXACT either way (not load-bearing).
- render_inverse_transform_screen_point point->vector casts removed: helper + debug_render EXACT (type-only, byte-inert);
  construct is canonical Wave 12 (336fecdb, A30 class, add_vectors3d COMDAT identical + provider link PASS).
## P11 AMENDMENT A (required): drop `byte reserved006[2];` from TU-private struct xbox_texture_cache_texture
- Evidence: HCEX texture_datum (identifier, request_index, available@4, postprocessed@5, bitmap@8, texture@0xC) has NO pad;
  natural alignment already places bitmap at 8; verify typedefs (loaded 4, used 5, bitmap 8, hardware_format 0xC, size 0x20) hold.
  Brief sec.9 lists pad members in a TU-private struct as an admission blocker; round-1 precedent removed such pads
  (player_profile byte pad[3], game_engine netgame_goal pads).
- Measured: gate (gr, emu split) 25/25 EXACT + guard pass; production-path compile (tamendA) canonical-IDENTICAL to cand
  (all sections/relocs/symtab) -> object_audit PASS 42 sections/46 symbols, provider_link PASS; /W3 unchanged.
## P12 AMENDMENT B (optional): XBOX_TEXTURE_CACHE_PROTECTION = 0x404 -> PAGE_READWRITE | PAGE_WRITECOMBINE (XDK WinNT.h 0x04|0x400),
  the spelling of physical_memory_map.c's XPhysicalAlloc of the same memory and game_state_xbox.c. gate 25/25; object identical.
## P13 patches (LF, git-style, generated with git diff --no-index from LF HEAD blobs; verified)
- combined_amended.patch = worker combined + amendment A: git apply --check at HEAD rc 0; applied (non-repo) == tested tree b2.
- 05_remove_invented_pad_member.patch: after worker 01..04 (applied sequentially, non-repo) -> == b2.
- 06_optional_page_protection_constants.patch: after combined_amended -> == b3 (amendB.c).
- regen_combined.patch (HEAD->worker result) applies at HEAD (check rc 0) == worker combined content.
## P14 composition with pending owner-queue packets (research/.../owner_queue/objlights_render_debug 05+06 render_debug,
   01+07 object_lights; object_lights includes both touched headers; render_debug gets this packet's include)
- tprod (fresh worktree copy) 612/612 identical to build/base. q0 = tprod+oq patches, q1 = tamendA+oq patches (compose.py).
  q1 vs q0 over all 612 TUs: 611 identical, ONLY xbox_texture_cache differs (renames). Packet composes cleanly with them.
## P15 full-sweep status of the AMENDED tree: q1 vs q0 (above) proves every other TU identical; tamendA xtc canonical == tcand xtc.
