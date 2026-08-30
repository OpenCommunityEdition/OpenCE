# `lra_cache.obj` global source/artifact census (2026-08-29)

This is an additive no-go record for `source/memory/lra_cache.obj` at
canonical commit `c4af544228c14b198bcdbf9397118434f1b37099`. It does not
change production source, headers, configuration, matching credit, compiler
flags, parked entries, or object artifacts.

## Rules and prior record

The current policy ledger and the complete 202-line historical strategy ledger
were read before the census. The relevant compiler records were also read:
`tools/c2dbg32/README.md`, `BACKEND_MODULE_MAP.md`, and `IR_LAYOUT.md`, plus
the C2 attribution/correction ledgers, the campaign playbook, matching
methodology, campaign/audit tool documentation, and the Claude spill-assignment
research in `research/reloc_identity_residuals.md` and
`research/single_function_residual_diagnoses.md`.

No `CLAUDE*.md`, `AGENTS.md`, or applicable `TREE*.md` exists in the repository
ancestry, at `source/`, or at `source/memory/` in the canonical checkout. A
finite check of the same locations in all registered worktrees found none.

The prior experiment boundary is therefore authoritative: E1-E50 already close
the declaration, predicate, loop/switch/goto, aggregate/separate-local, zero
alias/default/reset, `register`, barrier-placement, and available-compiler
families. The current rules additionally forbid the only historical control
that retained the shared bottom test, `_ReadWriteBarrier()`.

## Reachable and dirty source census

The repository had 1,514 registered worktrees after creating this isolated
lane. Every physical `source/memory/lra_cache.c` was hashed, and all ref-reachable
history for that path was enumerated.

Only four committed C blobs exist:

| Blob | Provenance | `_lra_allocate` disposition |
| --- | --- | --- |
| `6a1b59fc7d3f3c91955f6c26c9fd5821cf83dd7c` | initial skeleton | no function body |
| `d0a7a402f4da0b5dad8737ec497d3a7c1c7a7c77` | `52448097` / `claude/lra-cache-17` foundation | old private-helper ABI; 384-byte residual family |
| `2443ddb085b95d6a43290cb8eddc483f07c82067` | `4c769166`, `6b630d60`, and canonical `c4af5442` | current lawful early-return body; 368-byte residual |
| `c310323588b14056f918484c1e2191887e4877fa` | `b298f939` / `71cc36e5` | 400-byte single-return body using forbidden `_ReadWriteBarrier()` |

The seven raw working-file SHA-256 groups reduce to those four contents plus
line-ending/filter representations and one dirty partial reconstruction:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/lanes/lra-cache-exact`
  (`HEAD 94633a7f658208ba27e911d18d25143056ad5b55`) has modified
  `lra_cache.c`/`.h`, but defines only the lifecycle/validation subset and has no
  `_lra_allocate` body. Its built object has no `_lra_allocate` symbol.
- No other worktree contains a novel dirty allocator body.

The complete committed path history is seven commits and the four blobs above.
There is no hidden source candidate and no source/header/config collision to
integrate.

## Built-artifact census

All fixed-path worktree rebuilds were inspected. There were 1,092
`build/base/source/memory/lra_cache.obj` files. Their raw object SHA-256 values
form 1,070 groups because compiler metadata records checkout-specific paths, but
the hardened COFF parser reduces every present `_lra_allocate` to only three
normalized code/relocation forms:

| Count | Padded bytes | Relocation addresses | Normalized SHA-256 | Provenance |
| ---: | ---: | --- | --- | --- |
| 500 | 368 | `0x13, 0x5F, 0x83, 0x88` | `bd671ca359d22c92b9d24e5428f9c62f6fc4397f1643ed41d5a7df44ccdaa9ef` | current lawful baseline |
| 215 | 384 | `0x13, 0x61, 0x88, 0x8F` | `cfb23751433d74d9f6efe1756a6087e288c7ad1c869a994618b875b7ab02e4cc` | old-helper-ABI family |
| 2 | 400 | `0x13, 0x6B, 0x90, 0x95` | `9ea9b8efcaea76ab4d35f520558c7e2315033e96fa194550de868b95b7f40e0c` | documented barrier form |

The remaining 375 base objects are old skeleton builds with no section-relative
`_lra_allocate` symbol. A recursive search of every LRA-named worktree found no
additional object path beyond its ordinary base/split pair.

All 1,115 available split targets are byte-identical at the raw-object level:
SHA-256 `6ecd5583976033242fdd8a89d7f946d591bc19cc296091b2cb9d3707dbd85b7e`.
The January owner is 400 padded bytes, has relocations at
`0x13, 0x6B, 0x89, 0x8E`, and normalized SHA-256
`93beee39f5b398662960640adbdef20b5569e5407bff1526a6021970f3001e2f`.

No rebuilt artifact is strict exact. The closest 400-byte artifact is precisely
the already documented E32/E38 barrier family: its last two relocations are
seven bytes late and its normalized code differs. There is no unintegrated
object from which a lawful source body can be recovered.

## Donor and compiler provenance

The locally complete histories of the supplied source donors were searched by
path inventory and pickaxe:

- `stianeklund/halo`: 6,224 commits; no LRA Cache path, symbol, or
  implementation;
- `pastudan/halo`: 6,659 commits; no LRA Cache path, symbol, or
  implementation;
- `punpckhdq/halo`: 141 commits; only the initial HCEX skeleton blob
  `6a1b59fc...`, with no allocator body.

The locally available HalocEA, Baboon, demon, Nimbus, HaloPC Restored, and
xboxrecomp working trees/histories contain no `lra_allocate`, `lra_new`,
`LRA_CACHE_SIGNATURE`, or LRA Cache implementation. The HCEA prototype/retail
research artifacts also contain none of the LRA symbol names, diagnostic
strings, or a source donor. No alternate January local/scope/line record exists;
the known PDB module still contains compiler records only.

The compiler evidence is unchanged. XDK compiler 13.00.9210 reproduced the
documented best form byte-for-byte, while the locally present SP4/SP5 C2
components reject the campaign p2 stream and are not compatible adjudicators.
The corrected C2 IR record leaves the callee-saved/spill decision unlocated; it
does not supply a defined-C source control or January provenance.

## Protected exact inventory

An existing rebuild from the canonical source hash
`89d3db50efc65d9599bf05391281bbebb263a20cfe4a4e8c19a185e69e4efd33`
was re-audited against the January object. All 13 inherited siblings remain
strict exact:

| Function | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_lra_full` | 32 | 0 | `aadc0b52b8c1221cf915fcae0ccb8523c97139ca0a00334310add95f7c961e83` |
| `_code_0010b9f0` | 16 | 0 | `21c4d57ab9cba336579640a56fa4aeaf0e225122d873fb5cea4f59d9982a2b13` |
| `_code_0010ba00` | 16 | 0 | `8c6f9b23a045e09d762bfe2dd6abd3487d313d1e3bc01e643fde3f3cc0c39a72` |
| `_code_0010ba10` | 32 | 0 | `70e14824c183c54f34216b7d7a95c2b9d9882ff1e75b9c6d338c8cac0a9ec2c0` |
| `_code_0010ba30` | 128 | 6 | `6aab6630fbe789fb85fbbd0ee4c035918e521ebe2d1f713156b27cc2a6240aa6` |
| `_code_0010bab0` | 128 | 11 | `20efb86936868f30cf8ccc34d63d1a924c9f0c5b761276072325b430ad267562` |
| `_lra_new` | 288 | 19 | `c1ee7abc66169e2b850d7bc72a9b2201dbfb8a1ad989c1bdd1b2b552291977fe` |
| `_lra_dispose` | 80 | 5 | `68b0d3d2abace2014526db8c6b80ed5e146fcc0516637f8af9f8d0b9bddde829` |
| `_lra_flush` | 80 | 1 | `8f7843dcb920395c3bc501501d02e8eb8607f4a21ef71a034fa509da0ea65b6f` |
| `_lra_free` | 96 | 6 | `6fe13519324101d6037397fd15da03e18bf39a3c79229683640aacd5082866d1` |
| `_lra_lock` | 80 | 6 | `4b8568dba7e7d0c1546094484388b2a49b50b290d2bb5e11627949fb8fc404e1` |
| `_lra_unlock` | 80 | 6 | `94cfc56d9da2f45484b1f7d2a49c72e9e93a07aad1c823fdc65cdf505bdd99e2` |
| `_code_0010bdf0` | 32 | 1 | `b3e71fb5aa448be835c2a6905d8bc0bca66881aa5338cb6c33d7db2bb90329ce` |

The five target-owned string COMDATs also remain strict exact: 152 logical
bytes, 160 padded bytes, zero relocations. No protected Units source or artifact
was touched.

## Disposition

No genuinely new production candidate exists. Compiling a source variant would
repeat an exhausted family or reintroduce a forbidden compiler control, so the
one-candidate evidence rule forbids it. `_lra_allocate` remains the sole residual
at 388 meaningful / 400 padded target bytes versus 365 meaningful / 368 padded
lawful baseline bytes.

Reopen only for an authoritative January source/local record, a real donor with
this exact loop-carried zero dependency, a campaign-compatible original C2/QFE,
or a newly proven defined-C control outside E1-E50. This lane intentionally
performed no candidate compile and made no production change.
