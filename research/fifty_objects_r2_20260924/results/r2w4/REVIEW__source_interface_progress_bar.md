# REVIEW source/interface/progress_bar

approve: True

amended_patch: None

## checks
All work was done in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_progress_bar\ (evidence in LEDGER.md there). HEAD is 5d3ca708. The task named 7b9de585, and the only change between the two is one docs ledger. The tracked tree was clean before and after. Claim review_critic_3_progress_bar was taken and then released.

1. Applying the patch: `git apply --check` of 06 at 5d3ca708 is clean, and GNU patch applies it cleanly to copies taken from git. Applied in sequence after 01 and 03-05, then 07 and 09-21, the progress_bar .c/.h and symbols.json rows come out identical to the patch applied alone. (08 failed only because my file-list script split the path "saved games"; it is not related to 06.)
2. Split emulation: I ran csplit.exe myself on config copies. The control split equals build/split for all 833 objects. With the patched symbols.json, only progress_bar.obj changes: bytes, relocations and order are identical, and symbols differ only by the 11 renames.
3. Gate: production gives 53/1/0 and the candidate 53/1/0, with all 54 rows the same under the rename map. The candidate was run against the patched split through with_split.py, with CL=/I pointing at the shadow source. A /E run confirms the shadowed progress_bar.h is the one used. The only residual is _draw_layer_int at 336 vs 352, which already exists.
4. Production vs candidate objects, keyed by owner: 126/126 sections have identical content and the symbol multiset is identical. The only differences are that the UnlockRect and Texture8_Release COMDATs swap order, and their two .debug$F sections swap position (A15==B16 and A16==B15). In d3d8.h, IDirect3DTexture8_Release is at line 2107, between D3DTexture_UnlockRect (2102) and IDirect3DTexture8_GetLevelDesc (2118). That is the January slot 0xd1bb0; IDirect3DResource8_Release is at line 2020 and does not fit it.
5. Admission battery through the redirected split: object_audit is FAIL(25) on both sides, with identical text after rename and sort. pdb_storage has 29 disagreements on both sides, identical. surplus_identity finds 0 on both sides. provider_link passes in both link orders, 19 rows identical.
6. Data: every .data/.rdata/.bss section is identical. objdiff 3.3.1 (sha1 3130e428) mini project gives 6931/7280 code, 53/54 functions and 19501/19501 data for both production and candidate, so the packet earns zero credit.
7. Names, checked independently with xnames.py, which resolves every relocation to a name instead of masking it (Sept-2001 cachebeta.map + xbe vs the January split):
   - 10 of 11 pairs have the same instruction shape, and every reference points to the same target. Examples: is_stuff_ready reads _progress_bar_mode+8 in both builds. draw_layer and draw_initial_layer both call draw_layer_int, and only draw_initial_layer passes the 1.0 alpha literal. progress_bar_globals references sit at a constant +560 from Sept _player_ui_globals.
   - make_stuff_ready matches January's setup except that Sept calls uncompress_grayscale_buffer(new_logo) where January calls load_loading_texture(&texture0).
   - Call graph and callers agree. The Sept map lists every name as a static in progress_bar.obj.
8. Header consumers: game, ui_widget and render (the only other includers of progress_bar.h) compile to identical non-debug sections with the renamed header.
9. /W3 gives 12 warnings on both sides, identical.
10. Strip test and /Od: the patch only renames identifiers and removes three casts. There are no new scopes, declarations or decoration, and the uncast IDirect3DTexture8_Release(textureN) call is the natural spelling. Other TUs also use typed release wrappers (VertexBuffer8, IndexBuffer8, BaseTexture8, Surface8).
11. House rules: these are first-party Sept-2001 map names, including the odd authentic this_is_awful and the misspelled do_convoluation_coords. The same Sept-map source was accepted as authority in 05255584 (hardware_geometry). The unit is not on any held list, and no config entry is keyed on the old names. pytest tools/test_progress_bar_base_layer_runtime.py passes 21/21.

## issues
No blocking issues. I approve 06 as submitted: a zero-credit names and hygiene packet that does not complete the object. The object still has the draw_layer_int residual, audit FAIL(25) and 29 PDB storage disagreements, all of which already exist.

Non-blocking notes for the integrator:
- **Regenerate csplit** after applying the patch.
- **Pinned audit harness:** tools/audit/progress_bar_base_layer_runtime.py hard-codes NAME="_draw_gravy_base_layer" for its first and ordinary roles. Any future ordinary-role run against a newly built object will fail the symbol lookup. Its CI tests don't use the name and still pass. Leave the harness alone (editing it changes its recorded sha) or update it on purpose.
- **Stale evidence text:** owner-queue item 7 (hardware_geometry) cites progress_bar.c:1812's IDirect3DResource8_Release((D3DResource *)...) as precedent. The patch removes that spelling. The item is moot, since 05255584 already settled it by the Sept map.
- **Overstated claim:** the manifest says the COMDAT order "equals January's". That holds only for the three texture thunks (UnlockRect, Release, GetLevelDesc). Overall, our object's emission order still differs from January's image layout (tga loaders, is_stuff_ready and the Halo functions are interleaved in January). It is not scored.
- **Follow-up, not in this packet:** the 24 progress_bar D3D thunk rows, including the renamed _IDirect3DTexture8_Release@4, lack "static": true. January had them static; they are absent from PDB publics and the Sept map lists them as statics. hardware_geometry is the precedent for flagging them. Separately, draw_layer, do_convoluation_coords, draw_fade_layer, progress_bar_is_stuff_ready and progress_bar_rendering_enabled are public in our source but static in January. Changing their storage is not byte-inert: the never-called ones would be dropped.
