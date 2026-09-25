# REVIEW source/rasterizer/xbox/rasterizer_xbox_dynavobgeom

approve: True

amended_patch: None

## checks
All checks ran at HEAD 5d3ca708. Everything I wrote is under scratch/w/review_critic3_rasterizer_xbox_dynavobgeom/ (LEDGER.md R1-R6 and MANIFEST.md). I edited no tracked file and ran no ninja, configure or git mutation. The claim was taken and then released.
1) Name evidence, checked myself. The Sept-2001 cachebeta.map lists `0001:0014cda0 _submit_screen_vertex rasterizer_xbox_dynavobgeom.obj` in its Static symbols section. It is 0x40 bytes and sits between unlit_geometry_draw (0x270) and psuedo_dynamic_screen_quad_draw, which is January's slot, size and neighbour order. A second first-party map, the Sept-2001 retail cache.map, lists `@submit_screen_vertex@4` (static, 0x40) directly before quad_draw. The fastcall @4 also shows exactly one 4-byte parameter, which matches our signature.
2) Body identity (body.py, body.txt). The Sept xbe bytes at VA 0x15dda0 match the January split function in all 0x40 bytes outside the 3 rel32 fields. The Sept call targets resolve through map section 0002 (D3D) to _D3DDevice_SetVertexDataColor@8 and _D3DDevice_SetVertexData2f@12 twice, which are exactly January's relocation targets.
3) Callers. There are no E8 callers in Sept and no relocations to the function in the January split: the quad_draw loop is inlined and the static is still emitted. So the caller evidence is empty in both builds, which is consistent but proves nothing.
4) No contradicting evidence anywhere I looked:
- January cachebeta.exe strings: none of the related tokens appear.
- Atlas (halo_ce and hcea, every build): neither name. The Sept atlas tier holds only publics, which explains "not in the atlas".
- HCEX.pdb via DIA2Dump (I confirmed the tool works on create_weapon): neither name.
- HaloCEA reference source: no helper name.
- Aug-2001 map: the object predates the helper.
- cachebeta.pdb publics list [0014EE80] and [0014F130] but nothing at [0014F0F0], so the function was a January file static, which agrees with `static` and "static": true.
5) Patch application. `git apply --check` returns rc 0. Applied with GNU patch to HEAD copies, it changes 4 .c lines (symbol comment, prototype, call, definition) plus symbols.json line 5720 (offset 0x14F0F0), edited in place. git grep finds no other reference in parked.json, the semantic files, rejections, ownership, tools or tests. Two historical docs logs mention the old name in prose only. The new name appears nowhere else in the tree, and no other critic_3 patch touches symbols.json near 5720.
6) Emulated split. Running csplit with the HEAD config reproduces build/split 833/833. With the patched symbols.json, only this object changes, and objcmp shows it identical apart from the rename (133 symbols, 20 B string-table shrink).
7) Gate, run through a redirect wrapper with tools unmodified:
- candidate vs split_cand: 17 exact / 0 / 0, `_submit_screen_vertex` EXACT 64.
- production vs split_ctl: 17/0/0.
- candidate vs the old split: 16 plus 1 UNWRITTEN. This is expected and means the .c and symbols.json changes must land together.
- objcmp build/base = prod.obj = cand.obj apart from the rename (51 non-debug sections, 195 symbols).
8) Section-4 battery on production and candidate:
- object_audit FAIL(2) on both sides.
- pdb_storage: 1 disagreement on both sides.
- surplus_identity: 0 non-identical.
- provider_link (both link orders): identical on both sides; with --baseline=prod.obj there is no new surplus.
- tinfo --data: identical apart from the .debug$S path.
9) objdiff 3.3.1 (sha1 3130e428) gives the same result for prod, cand and the build: 2159/5112 code, 16/17 fns, 525/525 data, 98.69271. Zero credit.
10) House rules and held classes. Only identifiers change, so the strip test and the /Od declaration-order check do not apply. Rule 7 prefers this authentic first-party name over the invented descriptive one. The dynavobgeom hold (SDK-table coalescing and the consumer-local global_window_parameters) is untouched, and the packet claims no admission.

## issues
No blocking issues and no amendment needed. The patch can land as-is, with zero credit; the object stays NonMatching and held.
1) The config/symbols.json edit and the .c edit must land in the same commit. With the .c change alone, gate reports the target function as UNWRITTEN against the old split. After applying, run a csplit regen; only rasterizer_xbox_dynavobgeom.obj changes.
2) Optional: two historical docs logs still mention the old name in prose (docs/object_matching_logs/rasterizer_xbox_dynavobgeom_obj_opus_reconciliation_20260903.md:12 and sound_dsound_xbox_obj_opus5_100k_20260914.md:40). They are historical records, so no update is required.
3) Pre-existing observation, not caused by this patch. object_audit FAILs (2) on both production and candidate on one .bss row. January's .bss is 1 byte (`?warned@?1??_rasterizer_dynamic_unlit_geometry_draw@@9@9` at 0). Ours is 2 bytes: the external `_reported_too_many_transparent_geometry_groups` at 0 and `warned` at 1. provider_link reports the same symbol as NO JANUARY PROVIDER. The dynavobgeom entry in config/object_admission_rejections.json says "all 31 January-owned sections/symbols match", which the current audit contradicts. The integrator may want to correct that entry's wording separately.
