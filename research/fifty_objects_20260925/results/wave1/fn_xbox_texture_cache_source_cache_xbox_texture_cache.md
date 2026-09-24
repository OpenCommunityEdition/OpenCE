# source/cache/xbox_texture_cache

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_texture_cache_steal_memory"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\production_crlf.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\cand_gate_all.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\cand_object_audit.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\cand_provider_link.txt",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\xbox_texture_cache\\LEDGER.md"
]

## production_changes
1) Apply C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\xbox_texture_cache\production.patch to source/cache/xbox_texture_cache.c with `git apply`. The repo has core.autocrlf=true and the file is CRLF; `git apply --check` passes. If you use GNU patch instead, apply production_crlf.patch with `patch --binary -p1`; I checked that it reproduces cand.c byte for byte. The edit is three hunks inside texture_cache_steal_memory only: (a) after `long stolen_size = page_count * XBOX_TEXTURE_CACHE_PAGE_SIZE;` insert
	byte *stolen_address =
		base_address + XBOX_TEXTURE_CACHE_STEAL_GUARD_SIZE;
(b) the first XPhysicalProtect's first argument becomes `stolen_address,` (it was `base_address + XBOX_TEXTURE_CACHE_STEAL_GUARD_SIZE,`); (c) `return stolen_address;` (it was `return base_address + XBOX_TEXTURE_CACHE_STEAL_GUARD_SIZE;`). end_guard_address stays computed from base_address. It must NOT become stolen_address + stolen_size: that is the old sm1 shape, and the bytes refute it. 2) Retire the config/parked.json entry {unit: source/cache/xbox_texture_cache, function: _texture_cache_steal_memory, class: unclassified}. The function now matches January's hash 7ffbda42..., so the parked body's bytes move. 3) No header, symbols.json, config.json or relocs changes. 4) After integration, run a full ninja and a per-function stable sweep. The edit is local to this .c file. The added local changes the TU name count only for later functions in this TU, and the candidate gate shows all 25 still EXACT.

## evidence
Baseline production gate: 24 exact, 1 residual (steal_memory [sha]). The alndiff showed one difference: January keeps the late writable pointer `lea R,[ebx+0x104000]` in ESI, production uses EDI.

The /Od build has no body for this function (it is the PC build, pc_texture_cache). HCEX has no record for it.

Oracle tests with no effect (all residual): 0..44 dummy names before the function (TU name-count oracle) and 1..12 dead locals.

Allocator decode: I retargeted Lane A's res9 alloc-oracle (dbg32c plus oracle.py/mapweb.py) to this worktree; C2.dll sha1 is identical, and R0..R3 all pass. In production, the CSE writable temp's ESI cost is 1001 = 600 from the stolen_size argument web + 400 from the end-guard web + 1. Both are uncoloured neighbours whose only allowed register is ESI, so the chooser takes EDI.

Lab probes: every computed argument of one call (CSE temp, assignment inside the argument, stolen_size as arg3) interferes with the other argument webs. A variable LOADED at its use does not. Lab LH (a param loaded as arg1) reproduces January's `push esi; mov esi,..; push esi` and its pop order. A census of the whole binary found the push/redefine/push-in-one-call pattern at 17 January sites. All but this function use loads or copies.

Hypothesis: a named writable-address local, initialised early and rematerialised at its use. Result: R1, R2, R3, R4 and the void* variant are all strictly EXACT, so declaration order and type do not matter. Final gate with --all --forbid-emitted-symbol _point_from_line3d: 25 exact, 0 residual, 0 unwritten; steal_memory is 224 padded / 16 relocations / sha 7ffbda42.

Oracle on the final candidate: stolen_size is coloured ESI first, the named local's early web is left uncoloured and rematerialised, and its late piece takes ESI at zero cost. This is January's allocation.

Whole-object audit: object_audit shows every January-owned .text, .rdata and .bss section OK. It reports 3 storage differences, all XDK D3DINLINE wrappers (IDirect3DBaseTexture8_IsBusy@4, IDirect3DDevice8_IsBusy@4, IDirect3DDevice8_KickPushBuffer@4): split 2, ours 3. pdb_storage shows none of the three is a cachebeta public, so the true storage is file-static and ours already agrees. The same class of difference exists in rasterizer_xbox_debug, which is already admitted as complete.

Surplus (already present in production, unchanged by this edit): .drectve, 10 string literals, 3 XDK tables, 4 __real constants, and the _add_vectors3d/_dot_product3d COMDATs. surplus_identity reports both COMDATs IDENTICAL to their providers. provider_link (both orders): SELECTED-PROVIDER LINK PASS.

build/report.json: data 6344/6344 = 100%. Code is 3248/3457 with steal_memory the only non-100% function, so this edit gives 3457/3457.

No admission rejections exist for this unit.

Strip test / house rules: the change is a real named local that carries the value used by the READWRITE protect call and by the single return. Declaration and initialisation are combined (rule 17). It adds no cast, parenthesis, pragma or dummy. The unnamed spelling is the residual; the named-local spelling is exact.

## blockers
None found for the function. Two object-level items for the integrator to confirm under existing precedent: (1) object_audit exits FAIL(3) only because of the XDK D3DINLINE storage differences. cachebeta.pdb publics show these wrappers are file-static, which matches ours, and it is the same class as the admitted rasterizer_xbox_debug. (2) The candidate-only surplus (strings, XDK tables, reals, add_vectors3d/dot_product3d) was already present and all of it passes the provider link in both orders.

## reopen_criteria
Not applicable: the function is exact. Reopen only if the full-build sweep after integration shows a regression elsewhere. None is expected: the change is local to this TU, and all 25 of its functions were verified exact.

## task notes
This run started fresh: scratch/w/xbox_texture_cache/ did not exist before it. What closed the function is a mechanism worth reusing. When January redefines a callee-saved register between two pushes of the SAME call with a computed value (for example `push esi; lea esi,[..]; push esi`), the value is a named local computed early and rematerialised at its use. It is not a CSE temp built at the call: VC7 makes computed argument temps interfere with the call's other argument webs, while rematerialised or loaded webs do not. January's draw_gravy_layer (progress_bar, still residual, size 336 vs 352) shows the related register-copy form `push ebx; mov ebx,edx; push ebx` and may respond to the same idea. The retargeted allocator oracle is at scratch/w/xbox_texture_cache/ao. Run it as `python ao/oracle.py <src> <unit> <fn>` from that directory; it takes about 1 second per compile.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\xbox_texture_cache\LEDGER.md has every probe as hypothesis, evidence, change, result: baseline, O1/O2 name-count oracles, v1/v2/v3/v5/v7, the allocator-oracle decode, labs L1/L2/L3/LA/LC/LD/LG/LH, the pattern census, S1/S3, the exact R1-R4 variants, and the final audit.
