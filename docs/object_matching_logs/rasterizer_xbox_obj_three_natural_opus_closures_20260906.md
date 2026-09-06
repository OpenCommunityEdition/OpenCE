# Xbox rasterizer: three natural Opus closures, 2026-09-06

## Result

Against canonical `0664c27c2bfe270f1ad3ee0324926a60f7244678`, admit three
ordinary source closures, **1,584 meaningful exact bytes / 1,600 padded bytes**.
The scratch gate advances from 69 exact / six residual / 20 unwritten to
**72 exact / three residual / 20 unwritten**. No whole-object Matching claim.

| Function | Meaningful | Padded | Ordered relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `rasterizer_set_texture` | 486 | 496 | 35 | `dc5b1cd08b24f3dd90c859db380212cd047b32fe695713ef5020c92cf0332b6d` |
| `rasterizer_set_frustum_z` | 314 | 320 | 22 | `984bc304482a6a7983d946e2b254c7d41987cf7657680eb98dd71c98392396b9` |
| `rasterizer_set_stencil_mode` | 784 | 784 | 78 | `903201d61d15f0c4dcad96fd81acb8049e8bfc201883061924dc95f3eaea8852` |

The read-only donor is `C:/halo-worktrees/opus-30k-rasterizer-widgets-20260905`,
tip `bd8c008e85a1543877d0843bab3f1c2dffab9a13`. The bounded proposed patch is
`scratch/rasterizer-xbox-three-natural-closures-20260906.patch`, SHA-256
`8463fc77e5cbe7e7781c6aa7303e5372fa1aaf3e4f3217aec3b5a71000892d3b`.
It changes no public declaration, linkage, helper inventory, file-scope
ordering, compiler option, or raw layout access. It does add the meaningful
block-local typed matrix-row view described below.

## Source admission

`rasterizer_set_texture` follows donor commit `129272529`: bind the valid
requested texture and record dimensions in its success branch; gate the
default-texture path on `!success` and bind/store dimensions there separately.
January independently has two bind calls at section +0xE7/+0x16F and matching
dimension stores at +0xF8/+0x104 and +0x185/+0x191. The old shared tail was
432 bytes / 31 relocations. Restoring the two genuine success paths supplies
the missing behavior structure, not a call inserted merely to emit an SDK
wrapper. The original failure diagnostic and continuation are unchanged.
In particular, replacing its `display_assert` with a fatal macro that adds
`system_exit` would alter the observed continuation and is not part of this
packet. Public argument and return contracts remain in `rasterizer_xbox.h`.

`rasterizer_set_stencil_mode`, also from `129272529`, applies the debug policy
to the public `long stencil_mode` input and then narrows once into the private
cached `short mode`. January tests the debug flag first, loads the dword input
only on the enabled edge, produces zero on the disabled edge, then compares DI
against the cached word at +4. This coherent policy/type boundary leaves every
switch arm and D3D call intact; no signature or declaration permutation is used.

`rasterizer_set_frustum_z`, from `4f7a3a39c`, binds the existing matrix row
`global_window_parameters.frustum.world_to_view.n[row]` to `const real *n` in
the row loop. The three products use that same typed row, avoiding repeated
long member paths without adding state, casts, or control edges. The owner
prototype already lives in `rasterizer_xbox_internal.h`; matrix layouts remain
in their genuine render/rasterizer headers. Donor controls with local names
`n` and `node` both produce the same exact section. This is a natural
reconstruction, not a recovered original source spelling.

### Held: model skinning and unresolved owners

The proposed fourth donor closure, `rasterizer_set_model_skinning`, remains
unchanged and parked. Donor `ff1144479` moves a scale load before the matrix
declaration; its stated purpose is to swap EAX/ECX allocation. Scratch
`body_J.txt`, `body_N.txt`, and `body_R.txt` enumerate declaration permutations.
No independent source/type evidence supports that ordering as an authentic
correction. Same-size, same-relocation exactness does not establish provenance.

`SetupSmartStates`, window-begin storage/BSS ownership, callless Unlock names,
and existing surplus helpers remain outside admission. Do not import the
donor's 95/95 snapshot as if it proved whole-object ownership or completion.

## Accounting and verification

Only the three corresponding parks are retired. Raw objdiff undercounts the
stencil function at 96.65072% due to compiler-local switch-label representation.
An entry in the existing hardened `semantic_matches.json` mechanism rechecks
the whole section and ordered relocations before crediting its 784 target
bytes; no comparison rule or target byte is changed.

The donor's 30,074 meaningful-byte headline includes texture/frustum's **800**
bytes but omitted the already-strict stencil's **784**. Keep that newly
recognized credit separate from headline-backlog subtraction.

The combined verification, capture hashes, consumer proofs, and reproduction
commands are recorded in the
[Devices export companion ledger](devices_obj_export_function_values_opus_reconciliation_20260906.md).
Full build and stable sweep pass: four combined gains / 2,112 padded bytes,
zero regressions; tests 953 passed / two skipped / 26 subtests passed.
Canonical reaches **925,807 meaningful exact code bytes**, with 391/833
Matching objects unchanged.

The complete canonical Xbox audit preserves **81 inherited nonselected code
owners**, including all existing fuzzy owners, and proves all three selected
sections strict with correct value/type/storage metadata. All **148 non-code
sections**, their symbols/flags/bytes/relocations, and five COMMON definitions
are unchanged. No code owner is added and no forbidden point helper is emitted.

Nine pre-existing candidate names remain absent from the current split-name
inventory: the D3D/interface CubeTexture UnlockRect and Palette Unlock pairs,
interface SetFlickerFilter/SetSoftDisplayFilter, `dot_product3d`,
`plane3d_distance_to_point`, and `plane3d_from_point_and_normal`. They are
unchanged, not approved by this packet. The earlier donor-only controls listed
five because donor naming metadata differs; that smaller count is not the
canonical inventory. No final-link or completed data-owner claim is made.
