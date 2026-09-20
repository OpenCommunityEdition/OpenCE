> Historical worker capture, preceding independent review. Root subsequently admitted ONLY the vertex function after complete wave6 validation; see the independent review and current handoff.

# Wave 6 hardware geometry: one vertex candidate, triangle held

The single fresh vertex-only replay is strict exact: `_rasterizer_vertex_buffer_new` is 354 meaningful / 368 padded bytes with all 20 ordered relocation identities equal to January. The gate advances from 15/17 to 16/17 canonical target owners with zero inherited exact losses. The candidate is unlanded and receives zero credit in this research packet pending independent review/admission. No production, shared header, target metadata, flags, or comparator changed. No Ninja ran.

## Reproduction and frozen evidence

- `prepare_vertex_only.py` creates `vertex-only.c`, `vertex-only.patch`, and `vertex-only-preparation.json` from current production plus the archived authentic-tail hypothesis. It freezes `vertex-only-baseline.c/.obj`. The baseline source hash is `c17899db1f58625414fe3c5b1d880888cbb34c4be93dbd65b357d4cda065afd9`; baseline object hash is `b29aae2d7f9c3583724f1094ac209fab71da1215f3fde46a8a130c0d467a7d82`. Preparation froze HEAD `f3daaff82197c861a9a6899f76b701969a2151a3`.
- Candidate source SHA256: `0807f2722a26392440e929c2bdca47a02ac45410119cd81111fec640930c1886`.
- One compiler invocation: `python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_hardware_geometry --source scratch/astra-wave6/hardware-geometry/vertex-only.c --all --out scratch/astra-wave6/hardware-geometry/vertex-only.obj --forbid-emitted-symbol _point_from_line3d`. Receipt: `vertex-only.gate.txt`.
- Read-only complete census: `python scratch/astra-wave6/hardware-geometry/review_vertex.py`; result `vertex-only-review.json`. No baseline recompile is represented as having occurred; the copied baseline is the validated production object.
- Primary collection: `collect_primary.py`, `primary.json`, `run_ghidra.py`, `AstraHardwareGeometryLaunch.java`, `ghidra.log`, and `collect_xdk_contracts.py`. They preserve actual first-party binary/PDB hashes, queries, raw disassembly, and actual Ghidra output. Ghidra decompilation is corroboration; raw instructions carry the claims below.

## Supported source mechanism and limits

The current source omits the vertex Unlock operation and uses `_ReadWriteBarrier` to steer stores. The candidate restores the stock `IDirect3DVertexBuffer8_Unlock` call and normal HRESULT check before committing vertex fields, retains natural error cleanup, and removes the vertex barrier/early success return. It uses existing genuine `rasterizer.h` and `rasterizer_xbox.h` owners instead of the consumer-local rasterizer global facade and foreign declarations. The genuine member is `current_lock_operation` at offset2, with existing enum values `_rasterizer_lock_vertexbuffer_new`=2 and `_rasterizer_lock_none`=0. No shared header proposal is necessary.

January `rasterizer_vertex_buffer_new-split.asm` establishes the redundant initialized NULL store: +0xf2 loads the Lock output, +0xf7 compares it against zero, +0x100 branches past failure if nonzero, and +0x104 stores zero to the same local on the known-zero arm. This is an actual target operation, not declaration filler or a manufactured fake dependency. The current production already contains this store; the candidate expresses the same cleanup through `success`.

Later first-party `/Od` cache build raw `ghidra_007eb1f0.asm` corroborates the graph: output-null test at0x7eb3ae, success clear at0x7eb3b4, failed-success test at0x7eb3b8..3be, explicit output clear at0x7eb3c4, and actual COM Unlock at0x7eb82d followed by HRESULT failure handling at0x7eb836..85b. The raw output-field stores follow at0x7eb85e..89d. XDK `xbox/include/d3d8.h` lines2243 and2257 implement `D3DVertexBuffer_Unlock` and its HRESULT-returning `IDirect3DVertexBuffer8_Unlock` wrapper. The Xbox operation naturally folds to S_OK; its discarded error arm emits no new literal. The wrapper call is a genuine API operation also observed in the later body, not a helper invented for scheduling.

This reconstructs a supported operation graph; it does not recover original source text. HCEA DIA is a later DX9 revision with a short vertex-type parameter and an additional vertex-stream parameter, so those ABI changes were not transplanted. The later build has further format conversion and different device-null behavior. Its exact lexical scopes, store ordering, and successful-HRESULT assignment are not claimed as January source identity.

## Independently authenticated inherited bug

The preliminary assumption that Create* always writes the out-pointer is false. Fresh raw January XDK inspection proves that `D3DDevice_CreateVertexBuffer` can return `E_OUTOFMEMORY` at0x5de713..719 without writing the supplied output; only its successful path writes at0x5de72e. `D3DDevice_CreateIndexBuffer` likewise returns at0x5de5d3..d8 before its success store at0x5de5f7. Stock IDirect3DDevice8 Create wrappers merely forward arguments.

The January vertex caller then loads `[ebp-4]` at function+0xb3 after the returning error-report branch; that local is not initialized in the prologue. The current production has the same declaration and failure-path read. Candidate preparation asserts that the entire current declaration/Create/out-pointer-read/failure prefix is textually unchanged except for the explicit `BUG:` comment. It retains both existing pointer checks, all guards, and the same lifetime. No initializer or store was removed, no extra uninitialized read was added, and this defect is not newly used as the matching mechanism. Root explicitly authorized preserving this independently authenticated original bug under user house rule21. The previous archive's blanket “no UB” description is incorrect and is superseded by this evidence.

`D3DVertexBuffer_Lock` does assign its output on every returning path: the common store is0x5de787. Thus the restored lock-failure NULL store operates on an initialized output, independently of the inherited Create failure bug.

## Preservation and remaining ownership issue

`vertex-only-review.json` verifies:

- All 15 inherited exact target owners remain strict exact. Only the vertex constructor changes among the 17 existing code owners. Triangle constructor source and complete code/relocations are unchanged.
- All 15 nondebug noncode sections, 1,111 raw bytes including linker directives, are identical to the frozen baseline. All eight named January data owners match exactly: 572 raw bytes / 584 bytes rounded separately to four-byte boundaries. COMMON remains empty. No `_point_from_line3d` definition is emitted.
- Both baseline and candidate fake scans return zero review leads. This does not erase the separately documented inherited Create bug or inherited triangle barrier/goto debt.
- No new anonymous stub, local struct facade, consumer foreign prototype, assembly, intrinsic, pragma, flag, or target exception was added. Six inherited anonymous stubs remain untouched. The existing `_ReadWriteBarrier` declaration/pragma remains because the triangle function still uses it; the vertex invocation is removed.
- Two natural candidate-only COMDAT owners appear: `_D3DVertexBuffer_Unlock@4` and `_IDirect3DVertexBuffer8_Unlock@4`, 16 padded bytes each. Their complete sections equal the corresponding archived anonymous shapes `_code_00158470@4` and `_code_001584b0@4`. Those anonymous stubs remain, so this is an explicit duplicate-emission issue, not a silent rename or ownership repair. Equal empty-wrapper bytes alone do not establish a unique original name. No metadata correction is proposed on that basis.

Whole-object admission is not justified: triangle remains residual and the two extra natural wrappers require explicit ownership disposition. The proposed function-level ceiling is only 354 meaningful / 368 padded bytes. The inherited wrappers are not counted as new progress. Independent review must retain the unlanded status until admission is decided.

## Rejected triangle reopening

The Sept16 archive (`archived-cand4.c/.obj`, `archived-cand5.c/.obj`, `archived-w15_hardware_geometry.jsonl`) has a triangle constructor that is strict code-exact, but writes the same failure report in two different source branches. The archive records about330 earlier triangle variants; repeating them is prohibited.

January raw `rasterizer_triangle_buffer_new-split.asm` shows one merged report block at+0xb8 reached from device-null and later failure branches. This does not distinguish two source copies from compiler tail merging. Fresh later DX9 `ghidra_007eaca0.asm/.c` instead returns its initial success when the device is null and has materially different placement of field stores and cleanup. It therefore does not authenticate the archived duplicate-source report. No triangle replay was compiled. Its 337 meaningful / 352 padded bytes remain zero-credit and parked.

Reopen triangle only with primary evidence establishing the missing source-level branch distinction, or a different independently supported source mechanism that preserves exact controls and owners. Reopen exact wrapper naming only with identity evidence beyond indistinguishable empty code. The sole fresh compiled shape in this packet is the supported vertex-only restoration above; no additional spellings or field-order probes are proposed.
