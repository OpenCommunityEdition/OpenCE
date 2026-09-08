# Rasterizer Lights: verified frame-start reconciliation

Baseline: `a34d984c259abd1267bea3ce7b4ce207112fd533`.

## Outcome and scope

`rasterizer_lights_begin_for_new_frame` now reproduces January's **289
meaningful / 304 padded bytes and all 15 relocations**. The unit moves from
7 exact / 3 residual / 3 unwritten to **8 exact / 2 residual / 3 unwritten**.
No function or data owner is added. The remaining Lights bodies, including
the intentionally safe map-reset residual, are unchanged. This does not make
the whole object Matching.

The implementation includes the genuine `rasterizer_debug_options.h` owner
and removes its incompatible local prefix and foreign extern. The two
statistics-mode uses adopt the owner's `stats` member. The public function
already has its prototype in `rasterizer_lights.h` and a real Xbox frame caller;
no dummy caller, foreign prototype, shared-header edit or new inline is added.

Adjusted canonical totals become **930,363 meaningful exact code bytes,
6,283 credited functions, 391 / 833 Matching objects**, with unchanged
2,061,024 matched data bytes. The stable section sweep separately reports
**6,329 / 8,245 strict functions**, one gain and zero regressions.

## Source-credibility decision and honest search history

The read-only completed Opus donor is
`C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`, tip
`bd8c008e85a1543877d0843bab3f1c2dffab9a13`. Its first reconstruction in
`74e015673b3629d2999bbd13ac8cec9ab96367a0` captures the query result and
denominator in separate locals, then assigns a third `new_visibility` local.
Commit `7841c6ac306c4087af584a380b9e2aa44c45126d` removes that single-use
third local and puts its arithmetic directly in `MIN`.

This is **not a first-shot or original-source-authenticated spelling**.
The retained donor artifacts contain at least twenty earlier begin-specific
variants; the committed collapse is at least the twenty-first candidate after
the baseline. The three-local replay is a 304-byte residual differing only in
the `mov cl, al` / byte-zero-test schedule. No further source-shape search was
performed to admit this packet.

Root accepts the resulting expression because it is independently ordinary,
meaningful C: capture one query result, normalize it to a byte-scale fraction,
and clamp it with the existing cseries macro. It removes a single-use
intermediate rather than adding an inert dependency, alias, compiler directive,
unusual lifetime, or fabricated control-flow path. Source-search provenance
does not itself prove an expression nonsensical; neither does exactness alone
prove its authenticity. This judgment does **not** waive the separate holds on
FrameStatistics declaration permutations or a rejected predecessor body.

January's query call is at function `+0x93`; it then reads the denominator at
`+0x9E`. Separate local initializations preserve that order. The exact getter
has no direct reference to the Lights-private parameter array, so the older
candidate's earlier denominator load is **not claimed as a demonstrated runtime
mutation bug**.

The later independent HCEA reconstruction corroborates normalization,
clamping and asymmetric smoothing but uses a third local. It does not prove
January's exact local-variable topology.

## Numeric and macro contract

The `MIN(a,b)` macro is `((a)>(b)?(b):(a))`. Its second operand here consists
only of captured scalar locals, so the query executes once and any repeated
abstract arithmetic has no side effects. VC7 emits one multiply/add/divide.

The positive denominator guard excludes zero and the `LONG_MIN/-1` division
case. The real getter asserts a nonnegative result on nonfatal returns and
returns one when occlusion is disabled. That assertion alone does not prove
the multiplication safe. January's actual renderer initializes a 640x480,
non-multisampled back buffer; the single visibility quad supplies the physical
bound `0 <= visible_pixels <= 307200`. For any positive signed-long
denominator, the largest numerator is:

`255*307200 + (LONG_MAX>>1) = 1,152,077,823 < LONG_MAX`.

The smoothing inputs are bytes; their numerators are at most 1020 and 510.
Arbitrary large mock GPU results are outside this render-domain proof and are
not used to claim defined signed arithmetic. The tests are not a GPU simulation
or a certification for hypothetical higher-resolution/multisampled ports.

## Verification

- Full `ninja all_source progress semantic_progress`: pass.
- Frozen audit of all **572 rebuilt / 833 January objects**: only Lights
  changes beyond timestamps. Its other **43 runtime sections** retain bytes,
  ordered relocation identities/addends, symbol offsets/types/storage and
  COMDAT selection. COMMON and undefined imports are unchanged.
- The changed public owner matches the actual target. Its normalized hash is
  `ba6000638ca4648f9c8be2129eb56614787733165ab716dc580fa0647dc7bfe0`.
- No added `point_from_line3d`, helper, BSS, literal or foreign storage owner.
- Root's fresh-compile runtime tests execute both January and rebuilt x86,
  covering 84 normalization/smoothing records, six screenshot/debug gate
  combinations, empty and full 1,024-record queues, ABI preservation, unchanged
  parameter bytes and guarded output memory. A reversed smoothing branch is
  rejected by the negative control. Renderer/query services and the preexisting
  private result-address getter are explicit contract stubs; the latter's EAX
  input ABI is checked, but its addressing body is not executed. The frame body
  itself is not translated into a C/Python shim.
- Independent review caught a future regression-test gap: gate success and
  normalized byte hashes alone did not require ordered relocation equality.
  The fixture now explicitly requires `cc.section_infos_equal` between the
  January and freshly compiled sections before either executes. The full suite
  was rerun after that correction on 2026-09-07 with the same passing counts.
- Focused runtime tests: **10 passed**. Full suite: **1,019 passed, two skipped,
  26 subtests passed**.
- Parks: 297 active, zero stale/invalid. No park entry exists for this former
  residual, so none is removed. Admission: zero candidates/contradictions,
  five inherited rejections, zero revocations.
- Focused source scanner: zero review leads. `git diff --check`: pass.

This run uses January COFF/hex/disassembly and runtime execution. Ghidra's live
RPC was rechecked and is offline; the installed headless tool and cross-build
project remain available. IDA was not available in the inspected installation
inventory. Neither unavailable live tool is claimed as verification performed.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/lights-frame-before-20260906.json` | `c352a537c603a0735471c795f71d2bd7243ed01171e2cf9103465bf7733a26e8` |
| `scratch/lights-frame-final-20260906.json` | `f63f9735ed1d128358f3be65ca4d0fdb43a05bb17d4e4d86683501d12f5c7a48` |
| `scratch/lights_frame_packet_audit_20260906.py` | `80e4190dd754a3503aa066e7b7d7c977f3197437d4ef7338656a7f406b10ba1c` |
| `scratch/lights-frame-final-audit-20260906.json` | `137b8b15df5c280a7900f5defb86060f63ea3e4d1ca276af7c26902d937c2229` |
| `tools/test_rasterizer_lights_frame_runtime.py` | `6a566ef3c3409612566125aad9d33746f9ab20b337a422ce1a8e40584a232d80` |
| `source/rasterizer/rasterizer_lights.c` | `1ef2e8c24c493db5c17f9f9ab868154f146a55aee432c919089c36625d4edd47` |

The independent chronology/domain review is
`scratch/rasterizer-lights-begin-natural-audit-20260906.md`, SHA-256
`3dc6014ef17f4eed84f056dfce0a802cacd1ca834a075ee31f9151f1edde0564`.
It retains all candidate, donor and target hashes, including the non-exact
first-spelling replay. The complete shared statistics owner is documented in
[the preceding counter migration](rasterizer_frame_statistics_obj_shared_counter_owner_20260906.md).
