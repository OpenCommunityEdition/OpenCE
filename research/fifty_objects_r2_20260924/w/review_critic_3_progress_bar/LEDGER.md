# review_critic_3_progress_bar LEDGER (adversarial review of critic_3 patches/06_progress_bar.patch)

Tree: HEAD 5d3ca708 (task named 7b9de585; delta = one docs ledger). Tracked tree clean before/after. Claim label
review_critic_3_progress_bar (task slug string was garbled). Nothing tracked edited; no ninja/configure/git mutation.

## R1 apply
- `git apply --check` of 06 at 5d3ca708: clean. GNU patch onto git-blob copies (shadow/): clean.
- Sequential 01,03,04,05,06,07,09..21 onto blob copies: 06 applies in sequence; progress_bar.c/.h and symbols.json
  rows 3600-3660 equal the single-patch shadow (08 failed only because my file-list awk split "saved games").
- Old names absent from source/config/tools after the patch except tools/audit/progress_bar_base_layer_runtime.py
  (pinned manual harness, NAME "_draw_gravy_base_layer"; fixture-free pytest 21/21 pass, unaffected).

## R2 emulated split (own csplit runs, build/tools/csplit.exe, cfg copies)
- split_ctl == build/split 833/833. split_pb vs split_ctl: only progress_bar.obj differs; sections, bytes, relocations,
  order identical, symbol multiset identical after the 11-name map.

## R3 gate (gate.py; cand via with_split.py -> split_pb, CL=/I shadow/source; /E proves the shadow header is used)
- prod 53/1/0, cand 53/1/0, all 54 rows SAME under the rename map (only residual: _draw_layer_int 336!=352, pre-existing).
- objcmp prod vs cand (keyed by owner): 126/126 sections, content identical; the only differences are the COMDAT order
  swap (_D3DTexture_UnlockRect@8 now precedes _IDirect3DTexture8_Release@4) and the two associated .debug$F
  sections swapping ordinal (A15==B16, A16==B15). Symbol multiset identical.
- d3d8.h: Device8_End 1815, Resource8_Release 2020, D3DTexture_UnlockRect 2102, Texture8_Release 2107,
  Texture8_GetLevelDesc 2118: January slot 0xd1bb0 (after UnlockRect, before GetLevelDesc) fits 2107 only.
- /W3: 12 warnings both sides, identical.

## R4 battery (with_split redirect, tools unmodified)
- object_audit FAIL(25) both sides; outputs equal after rename + sort/whitespace normalisation (24 D3D thunk storage
  2/3 rows + _draw_layer_int residual, all pre-existing).
- pdb_storage 29 disagreements both sides, identical after rename. surplus_identity 0/0. provider_link PASS both orders
  (19 rows, identical).
- objdiff 3.3.1 (sha1 3130e428) mini project: prod 6931/7280 code, 53/54 fns, 19501/19501 data; cand identical.

## R5 name evidence (independent, xnames.py: NAME-resolved, not masked)
- Sept-2001 cachebeta.map static section lists every proposed name in progress_bar.obj with January's sizes.
- 10/11 pairs have identical instruction shape with every relocation resolving to the same target
  (e.g. is_stuff_ready -> _progress_bar_mode+8 in both; draw_layer/draw_initial_layer -> draw_layer_int; render's
  progress_bar_globals refs at a constant +560 offset from Sept _player_ui_globals; _wobble_phase == Sept _last_t+4).
  this_is_awful differs only where my resolver misread immediates 0x40005/0x11229 as addresses.
- make_stuff_ready (0x4b0) vs January setup (0x490): identical except Sept's uncompress_grayscale_buffer(new_logo) where
  January calls progress_bar_load_loading_texture(&texture0); then create_mask_texture, same render states/pitch loop.
- Header consumers game, ui_widget, render: non-debug sections identical with the renamed header.

## R6 verdict
APPROVE as a zero-credit names/hygiene packet. No object completion. Integrator notes: csplit regen; the pinned
harness name goes stale for any future ordinary-role run; owner-queue item 7 evidence text cited the old
progress_bar.c:1812 Resource8+cast spelling (moot: hardware_geometry already settled in 05255584).
Follow-up (not in this packet): 24 D3D thunk rows could take "static": true (hardware_geometry precedent); the 5
public-in-source/static-in-January functions are not byte-inert storage flips.
