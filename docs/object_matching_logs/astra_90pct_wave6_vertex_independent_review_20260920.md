> Independent pre-landing review. Root subsequently completed whole-tree wave6 verification and admitted ONLY the vertex function.

# Independent vertex-only admission review

**Recommend function-only admission of `_rasterizer_vertex_buffer_new`: +354 meaningful / +368 padded bytes, one function.** All 15 inherited exact functions survive. The triangle constructor remains unchanged and residual. Two ordinary stock SDK copies are disclosed below; they receive no extra credit, prove no anonymous-symbol rename, and do not establish whole-object admission.

The reviewed candidate is worker `vertex-only.c`, SHA-256 `0807f2722a26392440e929c2bdca47a02ac45410119cd81111fec640930c1886`. I copied frozen baseline and candidate into this independent directory and ran the normal scratch gate separately for both, with the point-emission guard. All measured function sections in both independent objects agree with the worker's corresponding objects. No production/header/config edits, flags, aliases or Ninja invocation were used.

## Source and primary evidence

The January target itself has the null locked-output re-store at function +0x104, after setting failure, and then a separate successful copy/output path. The fresh worker Ghidra capture at later DX9 `0x7eb1f0` contains the corresponding lock-output clear and an actual Unlock call followed by HRESULT failure handling and output initialization. This later implementation corroborates the operation graph; it is not proof of January lexical text or exact revision identity. The January XDK header supplies the real Xbox API semantics: its vertex Unlock is empty, and its interface wrapper returns `S_OK`. Thus restoring the real operation and result handling naturally specializes to the target's no-op Unlock path. The candidate does not add a helper copy in the caller, extra dependency, redundant error path to the triangle function, or compiler steering.

The source also replaces the consumer-local rasterizer prototype/global façade with existing real owner headers and uses the existing lock-operation enum. The target accesses `rasterizer_globals+2`; `rasterizer.h` owns `current_lock_operation` at offset2 and `_rasterizer_lock_vertexbuffer_new = 2`. Shared headers are unchanged. The vertex-only barrier is removed; the inherited barrier use in the unchanged triangle body remains.

## Inherited failure-path bug is not introduced or widened

I independently reread January `cachebeta.exe`, SHA-256 `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`. `CreateVertexBuffer` at `0x5de6e0` returns `0x8007000e` at `0x5de713..0x5de719` without writing the caller's output. Its output-pointer write occurs only on the successful path at `0x5de72e`. The original vertex caller nevertheless reads the output at +0xb3 after an error. This proves the existing failure-path bug independently of reconstructed C.

The candidate's complete declaration/Create/output-test/failure prefix is text-identical to production after removing its new `BUG:` comment. The comment identifies the API failure, absent output write, and original read precisely. No new local lifetime or uninitialized read is added. The other pointer, `locked_vertices`, is written by the actual January Lock implementation at `0x5de787` on its sole returning path before the caller tests it. The restored Unlock result is assigned by its wrapper before use. Preserving this already-existing, independently authenticated bug follows the user's explicit house-rule21 ruling; it is not a general allowance to introduce undefined behavior.

## Precise disposition of the two added SDK copies

The candidate naturally emits two additional static SELECTANY (`selection=2`) sections through the unchanged stock `D3DINLINE` definitions in `xbox/include/d3d8.h`:

| Copy | Header definition | Independent object finding |
|---|---|---|
| `_D3DVertexBuffer_Unlock@4` | Empty void method, line2243 | 16 padded bytes; `ret 4` plus padding; 0 relocations |
| `_IDirect3DVertexBuffer8_Unlock@4` | Calls that empty method and returns `S_OK`, line2257 | 16 padded bytes; zero return and `ret 4` plus padding; 0 relocations |

Neither has any incoming runtime code relocation in the compiled object. The two sections add 32 object bytes, but no new runtime operation, mutable state, side effect or fallback. They are authentic no-op/constant-return stock API artifacts caused by the genuine Unlock expression, rather than invented candidate-only behavioral helpers. The SDK's existing `static __forceinline` definition is not changed or newly introduced as a tuning directive.

Their full bytes and relocation records equal the retained target/current anonymous no-op and zero-return sections `_code_00158470@4` and `_code_001584b0@4`. Such tiny bodies do **not** uniquely authenticate original names. All anonymous owners remain unchanged; no alias/config change, removal, duplicate credit, or claim of unique correspondence follows. This narrow disposition fits the previously documented identical SELECTANY/XDK-copy class and the user's ban on newly introduced behavioral helpers. It is not a blanket waiver for other new owners.

The archived full `cand4` must remain rejected: it contains additional unsupported triangle/source changes. This review admits only the bounded vertex replay with an unchanged triangle body. The earlier archive rejection cannot be converted into a whole-donor approval merely because two of its natural wrappers are harmless here.

## Independent guard result

The fresh gates give 15 exact / 2 residual before and 16 exact / 1 residual after. Only the existing vertex constructor section changes. Triangle source and full object section remain identical. All eight named January data sections, 572 raw bytes, remain strict exact; the complete non-debug noncode inventory and COMMON are unchanged. No point helper is emitted, and all existing owners survive. Baseline and candidate fake scans both have no findings. `review.json` retains complete owner lists, full byte/relocation measurements, data/COMMON checks, source-prefix proof, wrapper metadata, raw January API instructions and independent build equivalence.

No production landing was performed by this reviewer. Root still owns whole-tree verification and final admission recording.
