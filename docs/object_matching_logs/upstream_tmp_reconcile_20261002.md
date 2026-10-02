# Transport, plane helper and main static-storage reconciliation — 2026-10-02

Independently reconciled against canonical `5e55d8f83ad7142962946b8a023d9638cd900b69` on
`jonas/exact-pilots`. Three owner-approved coherent packets were applied separately, with a fresh
clean build and full gate after each. No whole-lane merge or unpublished research was imported.

| Packet | Donor commit | Canonical commit | Exact functions | Meaningful / padded code gain |
| --- | --- | --- | ---: | ---: |
| T, transport | `e191c0f1436939f5a98c088b1dfb46e2c336cc0d` | `25f719236d4bec7621357e89b95e02b006824727` | 2 | 457 / 464 B |
| P, plane helper | `78294dd367ca3014ddd2eda28cd5744a2ef386aa` | `38fdd8cb2431d1cc4793d40f9a4e21e81c040136` | 2 | 1,126 / 1,136 B |
| M, main storage | `41640b4db320570d38173f9620cd0eab1a0f2d0d` | this commit | 1 | 538 / 544 B |

## Exact section receipts

All five fresh canonical sections are strictly identical to their January split counterparts,
including relocation identities. The SHA-256 values below are normalized by the unchanged strict
COFF comparator, not hashes of a linked executable.

| Function | Meaningful B | Padded B | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_transport_initialize` | 416 | 416 | 33 | `47bd2187b394eb1f7975b4bc618fc0d041f563687eb841257fcae4296bd71db0` |
| `_poll_ep_array_compare_proc` | 41 | 48 | 0 | `10d0cbdd04c0ad15a3cd14bffdde707c085ce85c4b9fed4c036fb4c103ca9c55` |
| `_compute_ground_plane` | 334 | 336 | 14 | `edeff771d101548840cb5f3b2949765f5455b6e452e22f5f4ba90d79ca918ad2` |
| `_bsp3d_test_sphere_recursive` | 792 | 800 | 22 | `35304d9cd3dbf061092fc383fa53e18c78003d1c12cb9cddce7cf9b6e69381ed` |
| `_main_frame_rate_debug` | 538 | 544 | 52 | `17546b1f02410f30e6fc728300d275c40d65bf6cb6a35a096a8d735f96512d83` |
| Total | 2,121 | 2,144 | | |

## Source-policy scope

- T changes the return type to `long` in both the owner header and definition, consistent with
  January's full-EAX return. The repeated startup stores and natural single-exit/callback form are
  reconstructed from the approved evidence. No shared declaration was moved merely to steer bytes.
- P removes only the outer parentheses of the approved plane helper, preserving the `/Od` argument
  order. Its coupled `_item_accelerate` repair removes the unattested double local and passes the
  float expression directly in the attested order. The entire items object remains identical.
- M replaces the invented packed aggregate and six padding bytes with January's nine objects.
  `window` and `recursion_lock` are later-first-party-attested function-local statics. The six
  frame-rate statics' function scope, their six names, two enum identifiers, `current_window`, and
  the exact textual position of `main_game_render` remain disclosed as inferred/descriptive as
  narrowly approved. Explicit zero initializers reproduce declaration-order storage. The eight
  private symbol rows and the exact function's park retirement are in the same commit.
- No new assembly, cast device, padding, filler, pragma, compiler flag, PCH change, scorer change,
  verifier change, semantic exception or object status flip was introduced. DD2, DD3, the ai_debug
  casts, pill parentheses and other held forms remain held and unchanged. `scenario_paths` is
  untouched. Main remains NonMatching; 95/95 exact functions are not a whole-object admission.

## Canonical park reconciliation

The five exact entries were retired with their source changes. Four residuals affected by P were
remeasured on canonical rather than copied from the lane. Their original rejection/reopen text and
previous measurements are retained in `config/parked.json`. These receive no exact credit.

| Residual | Previous objdiff % | Fresh canonical % | Size / relocations |
| --- | ---: | ---: | ---: |
| `_render_camera_build_frustum` | 88.8082 | 90.11173 | 3,408 / 112 |
| `_bsp3d_test_pill_recursive` | 97.21691 | 98.086395 | 1,504 / 31 |
| `_convex_polygon3d_clip_to_plane` | 80.76382 | 82.37186 | 1,120 / 26 |
| `_build_structure_lens_flares` | 99.29274 | 99.198944 | 4,336 / 156 |

The lens-flare percentage decreases; its source is untouched and it was non-exact before and after.
The lane's separate unpublished fidelity rebaselines were not imported.

## Fresh canonical gates

Each packet passed: clean compile, progress, the unchanged 8,252-row strict stable sweep, parks,
object-admission audit, fake-match scan, tools tests and whitespace checks.

- Exact sweep: +2 after T, +4 after T+P, +5 after T+P+M; zero regressions at every gate.
- Halo functions: 7,475 -> 7,480 of 7,573.
- Halo meaningful code: 1,599,712 -> 1,601,833 of 1,770,166 (+2,121).
- Halo data: unchanged at 3,311,591 / 3,922,163. Halo objects: unchanged at 396 / 468.
- Strict section-owner rows: 7,648 -> 7,653 of 8,252. Main is freshly strict exact 95/95.
- Parks: 66 -> 64 -> 62 -> 61, zero stale and zero invalid.
- Admission candidates: 8 -> 9 (main added as a review candidate only); no revocation or contradiction.
- Tests at every gate: 1,663 passed, 5 skipped, 159 subtests passed. The lane's 1,700 figure is not
  claimed for this canonical tree; its additional unpublished tools changes were not imported.
- Fake-match scan: the same 26 inherited review leads.
- Fresh board-wide `/W3` compiles: 4,091 before and after, identical per unit and warning code over
  all 612 CL targets. Fresh `/W4` checks across the nine affected units: 603 before and after, also
  identical by unit/code (including main's 90). Existing warning locations may move with source.
- All 621 configured build objects plus the inherited extra libcmt/chkstk object were fingerprinted
  at every checkpoint. T changes one object; P changes five objects/six text sections; M changes
  only main.obj. No external/COMMON provider attribute changes or new helper copies occur.
- M's `.bss` is 0xA63, align 8, with the public short at +0x6D8 and eight class-3 objects at +0x6E0,
  +0xA3C, +0xA5C and +0xA5E..+0xA62. The halt/render relocation target names change to these statics
  without changing their destinations; both functions stay exact. Function/section order changes
  are confined to the approved main packet.
- All tracked production files outside the seven approved source/config paths are hash-identical
  to the baseline. The existing dirty README and unrelated untracked research were preserved.
- Q10 credit and all scorer/verifier pins are unchanged: 93 records, 661,012 bytes. This is bounded
  object/section verification, not proof of a complete executable link or playability.

Private full receipts are in `scratch/astra_tmp_reconcile_20261002/` (before, T, P, M gates;
warning compilations; object fingerprints; final scope/storage assertions). They include local
build artifacts and are not published. The approved production changes and this audit log are.
