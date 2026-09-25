# MANIFEST - review of critic_3 patch 20 (rasterizer_xbox_hardware_bitmaps rename)

- Base/tip: 5d3ca708 (clean; untracked research/ pre-existing). No tracked edit; no ninja/configure/git mutation.
  Nested throwaway repo applytest/ (slug-local) used only for git apply checks.
- Change under review: `_rasterizer_bitmap_cube_map_changed` -> `_rasterizer_bitmap_cm_changed`
  (symbols.json 5810 in place, static kept; .c prototype/definition/call + listing comment). Verdict: APPROVE unchanged.
- Evidence: Aug-2001 + Sept-2001 beta maps + Sept-2001 retail map name the slot cm_changed (size 0x210); January atlas
  4cc87b45 exact at 0x558120; Sept body 528/528 bytes identical (reloc-masked); sole caller rasterizer_bitmap_changed in
  Sept and January; no January literal contradicts; `cm` abbreviation attested in January strings.
- Target vs candidate: gate 18/1 both (rows identical modulo name); `cm_changed` EXACT 528 / 26 relocs; objcmp split and
  compiled objects IDENTICAL modulo name; parked _rasterizer_bitmap_new measurements unchanged.
- Data: all .rdata sections byte+reloc identical (objcmp); objdiff 1584/1584 both.
- Symbols/storage: object_audit FAIL(1) both (pre-existing park only), 0 symbol diffs; pdb_storage 0/0; surplus 0/0;
  provider_link PASS both orders both.
- Credit: zero (name only). Not run: full ninja / pytest / stable sweep (integrator).
- Follow-ups (pre-existing, out of scope): rule-9 consumer-local prototypes; listing-comment placeholders for 0x157d50,
  0x157f10 and the D3D thunks; docs ledger row still carries the old name (historical).
