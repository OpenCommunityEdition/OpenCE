# `rasterizer_xbox_active_camouflage.obj` credibility recovery (2026-08-30)

## Result

The object remains honestly incomplete. The only accepted ordinary-C
implementation is `_rasterizer_active_camouflage_set_visibility`: 32 padded
bytes, three relocations, and normalized SHA-256
`3c3d3a4c5367ce81d27babd47f164172f684935f32a1a090e36e4f9cac3d1b7a`.
Eleven of twelve January code owners remain unwritten.

This audit removed four literal `__declspec(naked)` assembly transcriptions
which had been exact by construction. Their `_code_00148dd0` through
`_code_00148e00` names were csplit address placeholders, not original names,
and the accompanying "custom callback ABI" explanation was false. Target
bytes, relocations, XDK 3911 declarations, and strict cross-object donors prove
that all eight anonymous sections surrounding the visibility setter are stock
XDK `D3DINLINE` owners. They must be emitted naturally by reconstruction of
the real typed Direct3D callers; this audit does not manufacture unused
wrappers or preserve handwritten copies just to improve the score.

The visibility function remains exact after replacing the invented six-byte
aggregate `bss_0045e8e0` with the three independently authenticated globals:

```c
boolean local_active_camouflage_visibility_flag = FALSE;
boolean local_active_camouflage_debug_cached_flag = FALSE;
short local_active_camouflage_debug_cache_count = 0;
```

VC7 emits the January six-byte `.bss` shape exactly: external symbols at
offsets zero, one, and four, with two bytes of natural alignment before the
`short`. The public function prototype now lives in the associated
`rasterizer/rasterizer.h` header rather than being improvised in a consumer.

## Provenance and constraints

The authoritative target is the January Xbox object split from the configured
executable. The isolated audit began at canonical commit
`bd2074f0ca277ff40b9efc6c1e00e30be4965ca5` and used Microsoft VC7
13.00.9254.1 with the configured `/O2 /Oy- /DDEBUG /Dxbox` translation-unit
flags. No compiler flag, comparator, split, linker, or semantic exception was
changed.

The audit followed `docs/matching_methodology.md`,
`docs/exact_match_acceleration_playbook.md`, the object-ledger rules, and the
current no-fake-match house rules. The checkout contains no tracked
`CLAUDE.md` or `AGENTS.md`; the applicable project methodology and historical
object ledgers were read directly. January bytes remain authoritative.

HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
(`src/blam/rasterizer/dx9/rasterizer_active_camouflage_set_visibility.c` and
its data definitions) independently preserves all three global names and
their `boolean`, `boolean`, `short` types. The January executable itself also
contains the assertion string
`local_active_camouflage_debug_cached_flag`. HCEA is corroborating source and
name evidence, not a replacement target.

Marathon 2 source commit
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c` has no active-camouflage or
Direct3D homolog. Its `motion_sensor.c` and `interface.c` are style-only
evidence for Bungie's use of semantically named file-scope scalar state,
`boolean`/`short` cseries types, and explicit prototypes grouped with their
owner. It supplies no target logic or bytes.

## Stock XDK owner recovery

The complete anonymous wrapper family is:

| January address | Size / relocs | Correct owner |
| --- | ---: | --- |
| `0x00148930` | 432 / 32 | `_D3DDevice_SetRenderState` |
| `0x00148AE0` | 80 / 5 | `_D3DDevice_SetTextureStageState` |
| `0x00148B50` | 544 / 32 | `_IDirect3DDevice8_SetRenderState@12` |
| `0x00148D70` | 96 / 5 | `_IDirect3DDevice8_SetTextureStageState@16` |
| `0x00148DD0` | 16 / 1 | `_IDirect3DDevice8_SetVertexShaderConstant@16` |
| `0x00148DE0` | 16 / 1 | `_IDirect3DDevice8_SetVertexData2s@16` |
| `0x00148DF0` | 16 / 1 | `_IDirect3DDevice8_Begin@8` |
| `0x00148E00` | 16 / 1 | `_IDirect3DDevice8_End@4` |

All eight compare strict-equal against naturally emitted owners in
`rasterizer_xbox_motion_sensor.obj`, including padded bytes, relocation
addresses/types, and resolved relocation destinations. The four small owners'
normalized hashes are, in order,
`da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8`,
`da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8`,
`db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4`,
and `c9740afccf5e7e01a66dfdb669d22a8462ac214622a1e8eff90b803ddb2d68c5`.
The first equal hash pair has distinct, correctly resolved callees.

This evidence corrects the earlier audit's impossible-cdecl conclusion. The
proper outer owners have ordinary stdcall decorations and callee cleanup;
`_code_...` never encoded an ABI.

## Experiment ledger

| ID | Candidate | Strict result | Source credibility | Disposition |
| --- | --- | --- | --- | --- |
| E00 | Inherited four naked-assembly copies | Exact by construction | Literal target transcription with false owner names and false ABI story | Deleted; four obsolete `asm-implemented` parks removed |
| E01 | Invented `bss_0045e8e0` aggregate and field names | Visibility exact; six-byte owner exact | Layout-compatible but historically unauthenticated source model | Replaced |
| E02 | Three HCEA-named cseries scalar globals | Visibility 32/32 bytes, 3/3 relocations exact; `.bss` size/symbol offsets exact | Natural, meaningful C with independent name/type evidence | Retained |
| E03 | Rename eight target sections from XDK headers plus natural donor proof | Eight of eight direct comparisons strict-equal | Corrects metadata only; no artificial source emission | Retained |
| E04 | Delete fake wrappers and leave true owners unwritten | Honest TU status becomes 1/12 | Accepted ordinary-C tally remains one; no score inflation | Retained |

The separate globals are not a compiler-steering trick: each has a distinct
authenticated identity and semantic role, and their natural alignment happens
to reproduce the target layout. No raw byte array, alias, address arithmetic,
`volatile`, pragma, naked function, `_emit`, inline assembly, undefined
behavior, unused emission anchor, or fabricated callback is retained.

## Honest boundary

The object is not complete and must not be represented as such. The eleven
unwritten owners are `_real_alpha_to_pixel32`, the eight XDK inline owners,
`_rasterizer_active_camouflage_cache_primary_render_target`, and
`_rasterizer_active_camouflage_draw`. The real-alpha one-shot remains parked
as an honest fuzzy boundary in its separate dated ledger; this audit did not
resume that lane.

Reopen the XDK family only when the real cache/draw implementations are
reconstructed with typed Direct3D calls that naturally instantiate the same
header owners, or when authenticated original source gives stronger evidence.
Do not repeat the naked wrappers, stand-alone dummy callers, unused function
anchors, address-based private names, aggregate placeholder state, or a broad
compiler-search campaign. Exact bytes are necessary but not sufficient.

The source contains no reference to `point_from_line3d`, so it cannot disturb
the January inline schedule or emit a `point_from_line3d` COMDAT. Matrix Math,
Vehicles, Units, AI Debug, and unrelated protected sources were not touched.

## Validation

The final isolated-tree validation before the committed-state regression was:

- the focused campaign gate reports exactly `1` exact, `0` residual, and `11`
  unwritten owners out of twelve;
- direct strict comparison of the visibility setter reports
  `all_equal: true`, 32/32 padded bytes, three identical relocation addresses,
  types, and named BSS destinations;
- direct strict cross-object comparison of all eight XDK owners reports
  `all_equal: true`;
- the pre-edit whole-TU manifest fails only on the intentional target owner
  renames, four removed fake base owners, authenticated BSS ownership change,
  and the visibility function's corresponding relocation-name change;
- complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` goals pass. The semantic run scanned 470 units,
  evaluated 4,982 functions, accepted 4,886 exact functions, and reported zero
  unit errors;
- the strict board reports 278/619 fully exact objects, 680,071/1,922,669
  padded code bytes, and 4,863/8,246 functions after excluding six fuzzy parks
  and four assembly implementations;
- object admission has no new candidate. Its one contradiction/revocation is
  the pre-existing unrelated `action_converse` state;
- parked-function validation reports 10 active, zero stale, and zero invalid;
- `fake_match_scan` scanned the edited C file with zero findings; the explicit
  source inventory likewise found no inline assembly, naked function,
  `volatile`, raw-byte emission, address placeholder, BSS placeholder, or
  `point_from_line3d` reference;
- both edited JSON files parse, `git diff --check` is clean, and all 221 pytest
  tests pass with the cache provider disabled.

After the metadata owner recovery, the split target object SHA-256 is
`95e4dcf475f8f897f46b9b34dbe5a1c9c89f1ad0275b862005c16d2021f4aa73`;
the final pre-commit rebuilt candidate object SHA-256 is
`e3c806c727bfe5ff021deff7e09fee73a6ac4d681537da352269c6c7b94d4598`.
Raw object inequality is expected because eleven owners are unwritten.

Finally, a clean committed-state whole-TU snapshot was written at
`1460e0787810ff13944394a08e5594ddcbdd6251`. The candidate object was deleted
and rebuilt from source before the check. Regression reports zero failures and
warnings, no changed non-exact or newly exact owner, and the visibility setter
as the sole `still_exact` function. A second check after the ledger-only commit
amendment produced the same clean result.
