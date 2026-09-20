> Final root disposition: HOLD, zero exact credit. Independent review reproduced the exact object, but a necessary duplicated filter block lacks an independent branch-topology witness. The worker proposal below is historical, not an admission.

# Packet 7: environment lightmap shared intensity

**Final disposition: HOLD, zero admitted credit.** One scratch candidate is byte/relocation exact for `__rasterizer_environment_lightmap_draw`: **4,008 meaningful / 4,016 padded bytes, 248 relocations**, strict normalized SHA256 `7818e559325c1524c20fc78b0857de1e2fb0c63012550d71a7f272029569807e`. All 42 inherited exact functions remain exact. Completed independent review in `../environment-independent/review.md` holds the candidate because the duplicated per-branch filter statements still lack an independent source/topology witness. The remaining spot-light residual and inherited data gaps also prevent whole-object credit.

## Frozen inputs and bounded work

- Baseline production commit: `76dfc1fb779a082752550f795ca356d8c5f4b93f`.
- Unit: `source/rasterizer/xbox/rasterizer_xbox_environment.c`.
- Frozen source `baseline.c`: SHA256 `55e5f4c4877616075a862f5830d86ac2cddbb2b41974bd9db521ee52712a196c`.
- `baseline-built.obj` is the frozen build output; an independent ordinary scratch gate created `baseline.obj`, whose entire code inventory strictly agrees with it.
- Exactly **one** candidate source shape and one candidate compiler invocation. No production, header, configuration, Ninja, metadata, or commit changes. Both gates ran under the normal baseline headers, outside root's temporary projection experiment.
- Candidate: `intensity.c`, `intensity.patch`, `intensity.json`, `intensity.obj`, `intensity.gate.txt`. Source SHA256 `fae2a8428afc3ea433f95386b170276d4bc4c4d1d7f80fc4049ac6d0fa77eee4`.
- `prepare.py` reconstructs the candidate by taking only the archived g2 lightmap body and leaving the current file's prefix and suffix byte-text identical. `preparation.json` records the archive path and hash. Reusing an archived source is disclosed; the new work is the fresh primary evidence that was missing when that source was rejected.

## Prior rejected work and the new evidence

Read the unit's September 2, 9, 12, and September 14–15 wave C, w1, w3c, n3a, and n4 logs before probing. N4 established two defects: the pixel-shader store graph, and the two interpolation arguments' x87 staging. Its four structural corrections produced 4000/248; fifteen plain-local/const/array/scope experiments did not close the remaining 16 bytes. A visible-inline experiment was worse and emitted a forbidden point helper. The historical g2 `intensity = 1.0f` family was already strict-exact, but it was rejected because the identity arithmetic had no independent source evidence. That rejection was valid on the evidence then available.

Fresh actual Ghidra extraction from the supplied first-party unoptimized PE now shows a stored float factor and all six uses. `collect.py` ran Ghidra 12.1.2 headlessly in read-only mode and DIA2Dump against the supplied HCEA PDB; `collection.json` records success. This is an actual current extraction, not an old decompiler text treated as source.

Primary executable: `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906\halo_cache_symbols.exe`, SHA256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Function entry `0x7dfc50`, source assertion path `rasterizer/dx9/rasterizer_dx9_environment_lightmap.c`. See `ghidra_007dfc50.asm`, `.c`, `primary_evidence.py`, `primary-evidence.json`, and `factor-raw.asm`.

`primary_evidence.py` independently rereads the literal, raw x86 instructions, helper thunks/bodies, and RTC record from the PE:

| Operation | Raw evidence |
| --- | --- |
| Stored factor | `0x7e01ab` loads float at `0x93dd5c`; `0x7e01b3` stores EBP-0x78. Literal bytes `0000803f` are exactly 1.0f. |
| Primary off scale | `0x7e01bc..0x7e01db`: 1.0 minus primary EBP-0x70, multiplied by factor EBP-0x78, passed to scale. |
| Secondary off scale | `0x7e01e7..0x7e0206`: 1.0 minus secondary EBP-0x74, multiplied by factor EBP-0x78, passed to scale. |
| Primary on interpolation | `0x7e0212..0x7e022d`: factor times primary EBP-0x70, passed as the point helper's third argument. |
| Secondary on interpolation | `0x7e0239..0x7e0254`: factor times secondary EBP-0x74, passed as the point helper's third argument. |
| Plasma on scale | `0x7e0261..0x7e0275`: the same stored factor passed to scale. |
| Plasma off scale | `0x7e0282..0x7e0295`: the same stored factor passed to scale. |

Scale thunk `0x404d77 -> 0x42e220` and point thunk `0x409ab1 -> 0x42e0d0` were resolved and their component arithmetic checked directly, rather than trusting decompiler names. Three periodic evaluations precede these operations, preserving the January call sequence.

This supplies the missing independent evidence for a genuine shared factor even though its literal is 1.0. It is not a policy exception for invented identity arithmetic, and it is not a general compiler law.

## Exact source correspondence and limits

The primary executable's same-source-revision claim is **unverified**. It is DX9 and its surrounding graph differs from January Xbox. Only the narrow common operation graph is used; January remains final authority. The HCEA DIA functions are tiny PPC stubs (4 and 16 bytes), with six compatible parameters and source paths/lines but no useful scalar locals. Their lack of a factor record does not prove absence of the factor.

RTC descriptor `0x7e0600` authenticates four 12-byte named colors, `psh_constants[96]`, and `vsh_constants__texscale[48]`. It does **not** recover scalar names, four scale variables, declaration order, or lexical braces. The scalar name `intensity` and the four scale temporary names are reconstructed. In the unoptimized PE each product is computed immediately before its call; the candidate names all four products before the calls. That temporary representation and placement are source reconstruction, not a claim of recovered lexical text. Independent review must consider it along with the verified product meanings and January's complete instruction/relocation match.

| Candidate hunk | Primary/January support; reconstruction limit |
| --- | --- |
| Shared 1.0 factor, four products, two plasma uses | Fresh raw evidence above. January +0xb48..0xbef has the same four scale/two point call graph; +0xb88/+0xba6 stages the two real interpolation arguments via `fld/fstp`. +0xc36 instead integer-copies the ordinary plasma scalar. This identifies the narrow expression distinction without claiming a global register-pressure rule. |
| Move illumination locals into the existing animated-illumination `else` | All their uses and producers lie in that existing conditional arm; January takes the no-illumination arm +0x9ae..0xa34 directly to +0xd7f. No new brace-only lifetime block is introduced. Exact original declaration location is not recovered. |
| Place stage-0 MAG/MIN/MIP calls in each existing texture-choice arm | January +0x386..0x40b chooses WRAP versus CLAMP/W, then executes the same three LINEAR state calls (state IDs 13/14/15, value2). The source preserves every operation and argument and uses the two real existing arms. The merged instruction tail supports the graph, but it does not uniquely prove duplicated lexical statements; that source form is reconstructed. |
| Use stock `PS_TEXTUREMODES` in two assignments | Authentic macro `xbox/include/d3d8types.h:2100` packs t3<<15, t2<<10, t1<<5, t0. PROJECT2D=1, CUBEMAP=3, NONE=0. January +0xa11..0xa1b and +0xc39..0xc45 performs precisely the conditional stage2 shift and OR with 0x18001 / 0x18021. No custom macro or header edit. |
| Put constant pixel-shader stores before the five conversion calls | January writes the two final-combiner fields at +0x9fd/+0xa07 in the none arm and +0xd1d/+0xd27 in the animated arm. The latter are followed by all five conversions +0xd31..0xd72. Current source instead ended both arms with the same field-store pair, merging away two relocations. The candidate restores the authentic store/call boundaries; the existing n4 log independently explains this pair. |
| Put bump-edge case block before specular-mask case | January actual block order is bump-color +0xe6a, bump-edge +0xe92, specular-mask +0xec4. The jump table +0xf84 has case6/7/8 -> +0xe92 and case3 -> +0xec4. Cases' operations and meanings are unchanged. |

No copied helper body, foreign prototype, synthetic padding, volatile, forced inline, new uninitialized read, or workaround macro was introduced. Inherited point/scale declaration-only context remains unchanged.

## Admission measurements and ownership

Reproduce from checkout root:

```text
python tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --source scratch/astra-wave7/environment/intensity.c --all --forbid-emitted-symbol _point_from_line3d --out scratch/astra-wave7/environment/intensity.obj
python scratch/astra-wave7/environment/audit.py
```

`audit.py` and `audit.json` compare complete normalized section bytes and canonical relocation identities. Candidate gate: **43 exact, 1 residual, 0 unwritten of 44**, versus baseline 42/2/0. Full lightmap size/hash/248 relocation records match; no address or stack-slot erasure is used for admission.

- All 42 inherited exact functions preserved; only `__rasterizer_environment_lightmap_draw` changes among existing code owners.
- 51 emitted code owners before and after; no added/removed helper, including no point helper.
- 88 named external/file-static defined owners unchanged. Compiler `$L` labels renumber with source length and are not treated as behavioral owners; their normalized internal relocation identities remain part of strict checks.
- All 38 non-debug noncode sections preserve bytes, relocations, flags, and ownership. COMMON empty before and after.
- The parked `_rasterizer_environment_specular_spot_light_begin` remains 944/64 and strictly unchanged. No spotlight probe.
- Inherited data limitations are retained: the selected January 2-byte `_specular_light_vertex_shader_permutation_index` is not emitted by the baseline/candidate; the 181-byte `_rasterizer_environment_globals` section is not strict-exact in either. No data or whole-object credit is claimed.

The alignment display has 947 instructions on both sides and six textual annotation hunks at +0x2a/+0xa3f/+0xa70/+0xaa1/+0xb48/+0xb68. Those are the inherited distinction between a link-selected symbolic datum in the split target and the same datum defined in the compiled COFF (`global_d3d_device` assertion string, zero, one). `coff_compare`'s full canonical relocation identities match. The packet does not misreport zero raw display hunks.

Independent rebuild/source review is complete. The factor and four meaningful product temporaries were judged defensible with the lexical limits above. Root authorized one independent minimization ablation restoring the common filter tail; this remains 4016/248, preserves all controls/owners, but loses strictness solely through argument setup at +0x3e1..0x406. January has only a merged tail and therefore cannot distinguish the two lexical representations on its own. The candidate consequently remains **HOLD with zero credit**. The independent packet preserves the ablation and its full alignment. No further source variants are proposed. A separate bounded read-only follow-up under `../environment-topology-followup/` checks the already-supplied corpus for an independent branch witness.
