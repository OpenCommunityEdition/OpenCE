# Error Geometry Fable exact-subset independent review (2026-09-05)

## Verdict

**PASS for function-level integration:** the committed Fable subset is a
credible reconstruction of the real Error Geometry file/open and primitive
writer graph. Ten new functions are strict, `error_geometry_polygon` is an
honest high-confidence residual, both inherited functions remain unchanged,
the true owner header carries the public ABI, and the only existing external
header consumer is unchanged in every runtime and ownership record.

**Do not admit the whole object as Matching.** Natural rectangle construction
also emits a 32-byte `_set_real_point3d` SELECT_ANY COMDAT which January Error
Geometry does not own. Current canonical SELECT_ANY copies pass the ordinary
duplicate-symbol phase, so this does not invalidate the reconstructed
functions. January's selected AI Debug owner is SELECT_NODUPLICATES, however,
and the mixed target/candidate pair fails LNK2005 in both input orders. No
successful final link or discard artifact exists. The explicit
`candidate-only-comdat-owner` veto in `config/object_admission_rejections.json`
is therefore required and correctly separates partial code credit from
whole-object completion.

No source formulation was retried for this review. In particular, the point
setter was not manually expanded, forced inline, suppressed, anchored, or
given altered COMDAT metadata.

## Frozen inputs and scope

- January split object: `build/split/source/tool/error_geometry.obj`, SHA-256
  `fe872618f054d9641d5974279d2fb9c4a68a55eb11915662853194903371c0c6`.
- Frozen pre-packet canonical object:
  `scratch/error-geometry-fable-packet-before-20260905/source/tool/error_geometry.obj`,
  SHA-256 `d2561e99565246d418936464c320c185045ecb8fb43c4d3feef6bcd9caf83cc8`.
- Frozen actual canonical candidate:
  `scratch/error-geometry-fable-packet-after-20260905/source/tool/error_geometry.obj`,
  SHA-256 `45570b2b36f931bd0cf0e9f685f5295c98a4269e7adeb810bf09481f4c36892a`.
- Isolated unchanged-source gate object:
  `scratch/error-geometry-natural-isolated-20260905.obj`, SHA-256
  `c82db074ea22c4349ec7eb3155ebdabfa1c57e479ddc2dcaf42d6923ae0f1c8a`.
- Reviewed source SHA-256:
  `9d5cd0f4d3fd1d4c019e839e2222a248268d23bc8576ba4eed8f840795c2b400`.
- Reviewed owner header SHA-256:
  `d736c4b83e3a9f93e9c8ea59205c6cf3186b829795f8ed2efb2ce4584e80851a`.
- Reviewed semantic symbol metadata SHA-256:
  `f973ebab7f2387fe8596809fdecac03b9efeeddf9b6ddd14e13b3e3c4c8f8236`.

The donor provenance is the committed Error Geometry source at Fable commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`. The current dirty donor's later
polygon-mesh and bounded-triangle additions are not in this packet and receive
no conclusion here.

## Source, linkage, and ABI review

The source uses `tool/error_geometry.h` as the public owner and includes it in
the implementation. The header uses the established `real_math.h` types and
declares all currently implemented public functions: initialize, dispose,
set-name, point, line, triangle, polygon, rectangle, the three bounded
wrappers, and the variadic comment writer. It does not prematurely declare the
still-unwritten transform setter or private file-open helper.

The private `error_geometry_file_is_open(void)` owner is 80 bytes/10
relocations and exactly matches January, including static storage class 3,
function type `0x20`, selection 1, and the AL boolean result. All 12 January
references are inside Error Geometry; the complete target/base census found no
cross-object reference. Its semantic name is supported by the exact Halo
symbol-atlas pairing
`../../research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
(record 2106) and by the lazy-open behavior. It is not claimed as a direct
HCEX-PDB recovery.

The public functions have external storage class 2 in January and the
candidate. PDB2 atlas labels corroborate their public names; the parameter
types and stack ABI are independently grounded by the full January bodies,
their internal call graph, and the existing Connected Geometry triangle call.
No local extern or prefix facade is introduced.

The 64-byte filename followed by a 52-byte `real_matrix4x3` is a complete
116-byte external `.data` owner. Its initial payload, 8-byte alignment,
`0xC0400040` flags, zero relocations and normalized hash
`4f61d488466ad3c2101f591022495972265241bbe75195680f7296923e6c5ba2`
all match January. The separate file pointer is a four-byte external `.bss`
owner with 4-byte alignment, `0xC0300080` flags, zero relocations and hash
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
Neither is COMMON. `error_geometry_file` is directly corroborated by the
January assertion text `error_geometry_file==NULL`; `error_geometry_globals`
is a descriptive complete-owner name, not asserted to be original PDB
spelling. The in-place metadata renames preserve storage and boundaries.

The filename algorithm preserves January's 59-byte base-name limit, explicit
terminator, `.wrl` suffix, close-before-change, and reinitialize/remove order.
Geometry writers use the real project types and owner APIs, preserve the
identity transform payload, scale output coordinates by 100, preserve alpha to
transparency conversion, and retain January's per-face/per-vertex formatting
and flush order. Bounded wrappers halve alpha only for the bounds geometry and
then emit the original primitive. Counts below three assert their valid signed
domain and return without opening the file.

## Code and inherited-owner audit

The independent full-property object census is
`scratch/error_geometry_fable_canonical_owner_audit_20260905.json`, generated
by the adjacent `.py` script. It loaded all 833 configured target objects and
572 current base objects with zero parse failures. It compares symbol value,
storage, type, section flags/alignment/selection, bytes and ordered relocation
destinations rather than hashes alone.

The frozen pre-packet object has 2 code owners, 4 non-code owners and no
COMMON. The candidate has 14 code owners, 40 non-code owners and no COMMON.
The 12 added code definitions are the 11 intended source functions plus the
surplus `_set_real_point3d`. Ten intended functions are strict, totaling
2,955 meaningful / 3,056 padded bytes. `error_geometry_polygon` is retained as
a 496-byte/50-relocation residual. The two inherited exact functions,
`error_geometry_initialize` and `error_geometry_dispose`, have identical raw
bytes and full owner/relocation records after mapping only
`_data_00307850` to `_error_geometry_globals` and `_bss_004561b4` to
`_error_geometry_file`. No inherited code or data owner is lost.

The only added undefined symbols are the expected natural dependencies:
`__fltused`, `csstrcat`, `csstrncmp`, `csstrncpy`, `fflush`, `fopen`,
`fprintf`, `matrix4x3_transform_point`, and `vfprintf`. There is no emitted
`_point_from_line3d`, no extra private function, and no new COMMON owner.

The polygon target and candidate both contain 148 instructions and 50
relocations. Their normalized hashes are respectively
`2f599a54025e36e52dc74243de9776633d0bc5e37f55fb1b2c6bcdd402200eaf`
and `1b20ffa8e5964e947257b0722839dd1a51d0f7b3752bbc0f88d33b239efdf0c3`.
The isolated ordinary objdiff is 99.91892%. The meaningful instruction delta
is the order of two independent post-loop reloads; source-required pooled
constant ownership accounts for remaining annotation differences. This is a
credible scheduler residual, not exact credit and not a reason for another
source-spelling pass.

## Literal and constant ownership

The packet naturally emits 36 new literal/constant sections totaling 585
physical bytes. Twenty-five have direct exact January Error Geometry owners.
The other eleven total 50 bytes and have compatible selected target owners:
newline (TIFF print), `"w"` (TIFF file), `"point"` (AI Debug), `"color"`
(Decals), `"bounds"` (Bitmap Extract), `"format"` (Bitmap Group), and real
constants 0.01, 0.5, 1.0, 100.0 and `-REAL_MAX`. Every pooled owner is the same
payload, size, zero-relocation body, alignment, flags, external storage and
SELECT_ANY selection 2. No incompatible literal owner or data COMMON was
found. The two renamed writable globals are existing 120 physical bytes, not
new literal credit.

## Behavior review

`scratch/error_geometry_runtime_differential_review_20260905.py` runs the
January split object and frozen actual candidate in separate Unicorn machines.
Calls within Error Geometry execute their real object code. Only genuine CRT,
project string, and matrix APIs are modeled; no host file is opened or
modified. The comparison checks exact writer format strings and promoted
arguments, API/event order, full 116-byte globals, file-pointer state, emitted
primitive topology, input immutability, stack/nonvolatile registers, x87
state, and direction flag.

The final result passes 24/24 cases covering lazy-open failure/success,
initialize/dispose, unchanged and truncated names, point/line/triangle,
polygon sizes 2/3/5, rectangle, all bounded wrappers, comment open failure,
and a genuine `vfprintf` mixed-vararg case (`%d`, `%f`, `%s`, and `%%`). The
source-required polygon residual is therefore behaviorally equivalent
throughout its real caller graph in the tested domain. Unsupported format
conversions fail closed in the harness. The source-required string-mutation
calls also record their buffers at API entry, before the model mutates them.

The CLI accepts explicit target, candidate and output paths plus optional
SHA-256 pins; it does not depend on donor state. This recorded run pins the
January and actual-candidate object hashes listed above. The harness remains
intentionally bounded to valid non-null geometry, nonnegative small counts,
the default identity transform, and API-level formatting rather than host CRT
locale bytes. Exact functions and direct January code evidence cover the
remaining contract; those exclusions are not reclassified as tested behavior.

## Header consumer audit

`scratch/error_geometry_header_consumer_independent_audit_20260905.py` uses
the same full section/symbol comparator as the prior 46-unit Observer header
audit against the frozen Connected Geometry objects. Its JSON result is PASS:
all 27 section identities, bytes, flags, selections, relocation targets,
program-symbol metadata, COMMON inventory and COFF runtime header fields are
preserved. There are no added/lost sections or definition transitions. This
confirms the true owner-header expansion does not perturb its only existing
external consumer; a timestamp-only raw-object change is not runtime credit.

## `_set_real_point3d` boundary

The candidate-only helper is 32 bytes, zero relocations, flags `0x60501020`,
external storage class 2, type `0x20`, SELECT_ANY selection 2 and normalized
hash `8bcddd57839f98d10c8d64854238ba75564306cc9ef4d8055ea3eccd128911a0`.
It has zero incoming object relocations because all 24 rectangle setter calls
were naturally inlined. The out-of-line COMDAT is compiler-emitted, not an
authored anchor, and receives no code credit.

The full canonical census finds ten same-byte definitions and all ten are
SELECT_ANY. Canonical AI Debug has a genuine `set_real_point3d` call in its
actor-prop visualization (`source/ai/ai_debug.c:1741`); it is not a retention
anchor. January selects a single same-byte AI Debug copy with
SELECT_NODUPLICATES selection 1.

`scratch/error_geometry_set_point_ordinary_link_audit_20260905.py` ran the
unmodified Error Geometry candidate with frozen January and canonical AI Debug
objects under the ordinary XDK linker, both input orders. With the January
owner, `_set_real_point3d` produces LNK2005 in both orders before LNK1120 (237
unrelated unresolved externals). With the canonical SELECT_ANY owner, both
orders pass the duplicate-symbol phase but end at LNK1120 (167 unrelated
unresolved externals). Repeating with `/OPT:REF` and a real
`/INCLUDE:_set_real_point3d` root yields the same classification. No DLL or
nonempty map is produced, so this is not proof that a complete link discards a
specific copy.

The scoped consequence is precise:

- current canonical base/base ownership is not proven incompatible, so the
  ten strict functions and polygon residual may be retained;
- mixed January-selected ownership is demonstrably incompatible, and a
  candidate-only code owner remains, so whole-object Matching stays vetoed;
- the veto may reopen only with authentic source topology or an ordinary
  successful compatible link/discard proof, after the four unwritten target
  functions and all other object debt are resolved.

## Independent artifacts

- Canonical owner census script:
  `scratch/error_geometry_fable_canonical_owner_audit_20260905.py`, SHA-256
  `72beddad76d9e56c7473c5de37996cd0e7b18580c17b38f255e54e222c1c9936`.
- Canonical owner census JSON:
  `scratch/error_geometry_fable_canonical_owner_audit_20260905.json`, SHA-256
  `b8d967d933f9c61a66bc6eed7e31662103bcac0ee9d59708c36897c7870e9a0e`.
- Ordinary-link script:
  `scratch/error_geometry_set_point_ordinary_link_audit_20260905.py`, SHA-256
  `388ba402b9fc2cb50d24dde16fd9fb54383a50b437394a7c300bae85de73b2bb`.
- Ordinary-link JSON:
  `scratch/error_geometry_set_point_ordinary_link_audit_20260905.json`, SHA-256
  `cbb046c6885e829abfc39608661207e8443e0b636313fcb4601ea91a7507e678`.
- Behavior script/result (final reusable 24-case revision):
  `scratch/error_geometry_runtime_differential_review_20260905.py` /
  `.json`, SHA-256
  `d96d629be593c8372f1786498152461718f8d7357f0cd2c1b8ec3b5303f95eac` /
  `56d085a146540df5c1fe64041e8a2b0cff7eb55cac409a048d96efe6b722b8ca`.
- Header-consumer script/result:
  `scratch/error_geometry_header_consumer_independent_audit_20260905.py` /
  `.json`, SHA-256
  `c2883e361f60c522f2b342ed8797b11e30402f73c36ae081863c0bea229598e2` /
  `b9755e098dd474655214a104a04bafb9e15594ae0fefff166a24885f082f06a3`.

This ledger approves the reviewed function-level packet and polygon park only.
It does not approve the dirty Fable additions, the four remaining January
functions, the surplus COMDAT, or whole Error Geometry object completion.
