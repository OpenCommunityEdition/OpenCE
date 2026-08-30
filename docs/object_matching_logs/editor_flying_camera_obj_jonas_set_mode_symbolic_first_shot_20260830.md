# `editor_flying_camera.obj` set-mode symbolic first shot — 2026-08-30

## Result

The sole frozen candidate for `_editor_camera_set_mode` emitted every one of
January's 213 meaningful / 224 padded code bytes and all 20 relocations at the
same addresses. Eighteen relocation identities are strict exact. The two
assertion-string relocations fail symbolic ownership because the HCEA-semantic
candidate used descriptive transition names that differ from January's
compiler-owned strings. Strict equality therefore fails closed. The complete
candidate was inverse-reverted without a literal correction or second
code-producing compile.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 213 / 224 | 213 / 224 |
| Relocations | 20 | 20 |
| Strict-identical relocation pairs | 20 required | 18 |
| Normalized differing code bytes | — | 0 |
| Normalized SHA-256 | `446b413fcfb6cf06ae3d1a195995215743ac55e73858a99c509a7da7d9cd0316` | `446b413fcfb6cf06ae3d1a195995215743ac55e73858a99c509a7da7d9cd0316` |

`source/camera/editor_flying_camera.obj` remains 10/21 strict functions and
656/5,184 strict padded bytes. The target remains unwritten. No production
source, header, data owner, configuration, compiler flag, semantic exception,
parked entry, or object patch is retained. This additive ledger is the lane's
only intended tracked change.

## Authority, donor, and prior-attempt census

Before editing, every applicable campaign and repository policy document, all
three existing Editor Flying Camera ledgers, owner source/header, callers,
January disassembly/COFF/relocations, authenticated HCEA source and data
layouts, and the tree checkpoint were read. The registered-tree census covered
1,524 worktrees and 1,524 owner-source files. It found zero definitions of
`editor_camera_set_mode`. All-ref source history and exact-string searches also
found no definition or bounded prior attempt. Scratch and untracked evidence
were excluded.

Immutable identities:

- isolated starting tip:
  `96714442bdb9c54fa32a86592ff70f392726aeff`;
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January `editor_flying_camera.obj` SHA-256:
  `5e0066e8f28b6c2c79e9e546075922b263342dea5a50d9ef84e8818792d99945`;
- XDK 3911 VC7 `13.00.9254.1`, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- natural flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`;
- baseline/final owner-source Git blob:
  `473a74da467d7074b43445df7722ba045730fd37`;
- baseline/final owner-header Git blob:
  `131ee7526499b50255d10783a0d6964c845f77a8`;
- pre-edit frozen regression manifest SHA-256:
  `21815a47332c00a65d11566302d739082a2fe9edc13d2b7e59ca8d3c702ad8f2`.

The authenticated donor is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/camera/editor_camera_set_mode.c`, Git blob
`379e95cdeec2e6d22e1d9cf4a2a6632cc1b22a0e`, file SHA-256
`cd0742fd605f6230bf850e9385c6f992ea41707b11029a50326d381b32cc808c`.
Its independent `translate_funcs.c`, `update_funcs.c`, and `mode_str.c` data
sources authenticate the two modes, outgoing/incoming 2-by-2 callback table,
camera argument, and two mode-name pointers. January remains authoritative for
the PC assertions, exact enum spellings, object ownership, and code.

## Frozen typed candidate

The existing production TU already contains the complete typed
`editor_camera_globals` prefix: the flying-camera pointer is at +0x18 and the
mode short at +0x2C. Its typed dispatch envelope already places the 2-by-2
transition table at `_editor_custom_render+0xC`. The candidate extended that
envelope with two named mode strings at +0x1C, added the public cdecl
`void editor_camera_set_mode(short mode)` declaration, and used the ordinary
`console_printf(boolean, const char *, ...)` prototype.

The frozen body followed the single natural topology established independently
by HCEA and January:

1. when a camera exists and the requested mode differs, call the current
   mode's outgoing callback if the current mode is nonzero;
2. call the new mode's incoming callback if the new mode is nonzero;
3. assert each callback pointer before its indirect call, at January source
   lines 302 and 308;
4. store the new mode; and
5. print the corresponding mode name through `console_printf(FALSE, ...)`.

The candidate source/header Git blobs were
`9e1fb893625a4583e897f25718c14e79da6c7ebc` and
`c1daa0d0a30bc36333ab614f9b89a8c24ef8ab2b`. The source working-file SHA-256
was `3b0b02427812616b76aa9761ae329b32210aa8b7a431bc0a0a128eba9be1edee`.
The preserved ignored candidate object is
`build/audit/editor_camera_set_mode_first_shot_20260830/editor_flying_camera.obj`,
5,740 bytes, raw SHA-256
`63af84552c83adea4180678fcb2b2e8f9631ad4a4952af16b81ffac148bb0990`.

## Exact fixed boundary

The one normal production Ninja edge compiled successfully. The function's
complete instruction stream, padding, normalized hash, all relocation
addresses/types, both assert line numbers, call order, table addends, and
epilogue equal January. Pairwise hardened comparison proves 18 relocation
identities exact. Only these two pairs differ:

| Offset | January-owned literal | Frozen candidate literal |
| ---: | --- | --- |
| `+0x44` | `translate_funcs[camera_mode][_translate_from]` | `translate_funcs[camera_mode][_translate_camera_end]` |
| `+0x91` | `translate_funcs[mode][_translate_to]` | `translate_funcs[mode][_translate_camera_begin]` |

The target literals are owned by COMDATs
`??_C@_0CO@LELHEFFL@translate_funcs?$FLcamera_mode?$FN?$FL_tr@` and
`??_C@_0CF@ONCNFIFO@translate_funcs?$FLmode?$FN?$FL_translate@`; the candidate
COMDATs are respectively
`??_C@_0DE@JOJNBNHM@translate_funcs?$FLcamera_mode?$FN?$FL_tr@` and
`??_C@_0CP@PDLCEHEN@translate_funcs?$FLmode?$FN?$FL_translate@`.
The exact target strings were decoded only during post-compile relocation
adjudication. Equal code bytes do not waive symbolic relocation equality.

No corrected-literal candidate was compiled. This lane's one-shot contract
prohibited a second code-producing build even though the measured residue is
fully localized.

## Do not repeat and reopening condition

- Do not repeat the rejected `_translate_camera_end` /
  `_translate_camera_begin` assertion spellings.
- Do not treat the equal normalized code hash as strict admission; the two
  literal relocations remain unequal.
- Do not sweep control flow, declarations, callback indexing, include order,
  qualifiers, or compiler settings; all code generation is already exact.
- A separately authorized future one-shot may reopen only from the now
  authenticated January-owned `_translate_from` / `_translate_to` strings,
  with the body and typed layout otherwise unchanged. This ledger does not
  claim or pre-admit that uncompiled correction.
- Assembly, volatile/register/pragma/intrinsic/attribute/barrier, raw offsets,
  representation puns, undefined behavior, byte forcing, and object patches
  remain prohibited.

## Restoration and validation

The include, enum/layout extension, public prototype, and body were removed by
inverse `apply_patch`. Final source and header Git blobs exactly equal their
baseline identities above. The normal restored object rebuild has
phase-specific raw SHA-256
`e606fa32215dbc85069d6f3c1f3d1eceeae937c9720c1c065c0e9e2c3df46ef8`.
The frozen whole-TU regression reports all ten inherited exact functions
`still_exact`, with no changed nonexact, newly exact, failure, or warning.

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,810 accepted exact, zero ordinary
  rejection, and zero unit errors.
- Progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Strict board: 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Protected Units: 189/189 and 54,864/54,864 padded bytes. Sentinel
  `_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Complete Python tooling suite: 212/212 pass; its only warning is the
  sandbox-denied optional pytest cache.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- Final `git diff --check`, deleted-path review, protected-source review, and
  source-policy audit: pass.

The frozen production candidate was readable typed C89 with one parameter per
line and an explicit terminal return. It contained no prohibited construct or
compiler/tool exception. No candidate source or production object survives,
and nothing is pushed from this lane.
