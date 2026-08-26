# `rasterizer_xbox.obj` Jonas vblank-callback first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`592fb2d8bb1539aedff3fc508e60905820b26dbe`. The private Xbox callback
forwarder matches in the first and only production candidate compile. The
object advances from 3/95 to 4/95 strict functions and from 3/15,574 to
17/15,574 meaningful code bytes. It remains `NonMatching` and claims none of
the target's 9,048 data bytes.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_set_vblank_callback` | 14 | 16 | 1 | `63bf78a6f07971df04769bae2ba18a6b90615e87a71be92f83beb002afa345bf` |

The sole relocation is `IMAGE_REL_I386_REL32` at section offset `+0x08` to
`_D3DDevice_SetVerticalBlankCallback@4`, with zero addend. The complete target
bytes are:

```text
55 8B EC 8B 45 08 50 E8 00 00 00 00 5D C3 90 90
```

The relocation starts after the `E8` opcode. The stdcall callee owns its one
argument, which explains the absence of caller stack cleanup.

## January, compiler, and current-tool evidence

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/rasterizer/xbox/rasterizer_xbox.obj`: 71,925 bytes,
  SHA-256
  `039e8296db80d1391e59f6d8ce59fefc49618513926edbbe4e9621d8b7202182`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- The isolated setup uses pinned `csplit` SHA-256
  `708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
  and `objdiff-cli` SHA-256
  `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
- Current validation-tool identities are:
  - `tools/coff_compare.py`:
    `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
  - `tools/regression_gate.py`:
    `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
  - `tools/semantic_progress.py`:
    `5cc1bedd907e277a603b47080d78ecd1fcd668834fd564b15257a1c5b4eefa67`;
  - `tools/audit_semantic_matches.py`:
    `05b709e5899c5d43ff3aee7d4077b906cb80b60f4e8e8041c039ad7667e1a33b`;
  - `tools/audit_object_admission.py`:
    `1e17c437a53433680fa01c3c558794bc1ee29246955e028bf5c9d816faf8fcc1`;
  - `tools/parked_functions.py`:
    `deb8d6b78b3085ba896230242bf1c4629efd442beb9c8a1962d723cc063e4646`;
  - `tools/transfer_executor.py`:
    `9a30ce16b29656235de647528616e9b462016611fb2de48aace1a20f6812d888`.

The January COFF proves the external cdecl owner, one pointer-sized stack
argument at `[ebp+8]`, complete padded extent, and exact relocation. Local XDK
`D3D8.h` independently proves
`typedef void (__cdecl *D3DCALLBACK)(DWORD Context)` and declares
`void WINAPI D3DDevice_SetVerticalBlankCallback(D3DCALLBACK)`. The existing
public wrapper, its private declaration, and
`main_vertical_blank_interrupt_handler(unsigned long)` agree with the retained
`void (*)(unsigned long)` callback type. No cast or calling-convention spelling
is needed.

The current shape atlas also groups this absent recipient with strict 14/16
one-stdcall-call functions in `cseries_windows`, including the natural
one-parameter forwarder family. That is compiler-shape corroboration only;
January's relocation fixes this recipient and destination uniquely.

## Claude, HCEA, and correspondence audit

All available Claude documentation relevant to this object, ABI, compiler,
and verification workflow was reviewed before mutation. The direct Claude
repository at `C:\halo-worktrees\claude-ai-conversation-2` is at commit
`0e1fc785b7f5fb8c8d6fec8cae667005aa663957`; its only dirty tracked file is
the reviewed research cache `tools/equivalence/leaf_cache.json`. No dirty
production source was consumed. Pinned documentation and tooling identities
are:

- `AGENTS.md`:
  `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`;
- `CLAUDE.md`:
  `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
- `docs/references/prototype-inference.md`:
  `ebd2a3e72206a2b727a70adf269fa5932f1392ced43a303167b969e6042be5b2`;
- `docs/references/kb-update-policy.md`:
  `4c9e2a4ae5b521f14ea45c6debe01c73e88258b35f6b51f50e53354528e0ea42`;
- `docs/ghidra-live-delinker.md`:
  `87069190158c3f6a05bf542aa5626b488539d8890df8f508b6d5d537ed6ddbae`;
- `docs/verification_policy.md`:
  `1a394e10a67b29926e6d82aa1303327e336053413f55a1b2ff545d8bebdf5922`;
- `tools/audit/check_arg_counts.py`:
  `3e5842ca9b8f7c16142a0b35fa4417a49405d51ff422ce879ea38dea22c8ef32`;
- `tools/verify/vc71_scores.json`:
  `93e71b2f0907266c259b4e9fdf823136d0e8d27409fbc52d8f1f37f68457e35c`;
- working `tools/equivalence/leaf_cache.json`:
  `e6ccf76b86864210331042cd1cd541b3de3319125b02d0d4483eb93b0eacaf6a`;
- `kb.json`:
  `6c5f1d3dc73686770b7888d656042cbbf43a9e3fa0289d4cc637e8cbf759ba52`.

The direct Claude repository contains only a later public wrapper that calls a
raw donor-build address. Its source blob is
`4665ec2f2b383dbe3ba779a2e5defc333317fb2b`, with physical SHA-256
`6980d7fe0ee246e64b9bf39ccd0d883f4194cd5dfd49e41f1bfb8d0232f2bfb7`.
The raw address, `void *` spelling, thunk assumptions, and donor configuration
were rejected. They were not transplanted into production.

The clean HCEA cseries cache at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` has an empty later-build private
stub and a public forwarding wrapper:

- private blob `8e7dc85bb1985e9b6f280ae67bd0cf513805dd87`, physical SHA-256
  `ced9f2b00f83c5bac0355d302dd2e82956f0b996b212f2a28ea72e3b09812ad8`;
- public blob `1cd794ed6854f41695230fbacb7551da1e2e427f`, physical SHA-256
  `d2c05ffe45ac71112869c70789eec8ea771d2ee2b3dd69cc1ec19196d6691f77`;
- `README.MD` SHA-256
  `e6960053faa47d59fb22cc9af1565a9c6e5b4a23390f8025f897f8a10a275ec0`.

The independently cached HCEA following-camera repository at commit
`0fd88176fe19df400576d2cbb447d3363080cb52` records the same architecture.
Its empty private stub is blob `751b98066c384472af202454af4db9b0d263e499`,
physical SHA-256
`9e2af7a39b03028060c1568fdc6a85da57597af375b1ec526c0e6bb7322c044e`;
its public forwarding wrapper is blob
`bf21b782e96f940b4747f32aa8d6670f022b46e3`, physical SHA-256
`e7fd097a5593075252362fa132cfff81cf45b7ddab2d601521dfd47c1e0f7013`.
These sources authenticate name, one-parameter ABI, and public/private
forwarding topology. Their empty later-build private bodies do not override
January's explicit Xbox D3D relocation.

The clean Stian research cache at commit
`e638cf5fa17ed5308ac5c101ef65e9c69ced65f7` contributes a later-build
`FUN_00155c10(void *callback)` that directly calls
`D3DDevice_SetVerticalBlankCallback(callback)`. Its source blob is
`9cfd34d6bc31c4fb8488ef1fd22ad4abf384538b`, physical SHA-256
`0274c6572b28aa644c702ae8c4612f0fccc21ac871169e822814439995cdb112`.
Its `CLAUDE.md` SHA-256 is
`97d4c63b9dd9d642cbe6774c5f3a578c60f2de002fce3ac888cd4ee635cc4c3d`.
This is independent semantic corroboration only; its raw addresses, pointer
spelling, and later-build completion claims were not used as byte authority.

Claude's untried-object worktree at commit
`deb5554ab8d16da80da934e2aa7081bc7d1f594f` records this exact private symbol
as target-only in `research/residual_frontier.json`, SHA-256
`db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
Its many untracked scratch artifacts were not read as production authority.
Complete current history and ledger searches found no prior private-body
candidate compile; the only later source history added the already-exact
public wrapper in `source/rasterizer/rasterizer.c`.

The clean correspondence repository at
`C:\halo-worktrees\fable-crossbuild-bridge`, commit
`bf520b6938d256ecc9a55d93e994a9f5dd389ec1`, was also reviewed.
`EVIDENCE_MODEL.md`, `REPRODUCTION.md`, and `FINAL_REPORT.md` have SHA-256
values `2520a4a0b516752cc1e74bc92a2e494f1b425044de3eb124ad6b214cdff9c607`,
`6ed5dddd92e6c33f602ed911e5af328a1057c57186da41e135accb07536caf25`,
and `0bb6498b9ba55a28eaa47990a5cf9a1a1b090ba77d83606e3be48808d5ba13f1`.
Its correspondence JSON SHA-256 is
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`,
and exact-tier ranking SHA-256 is
`f3fdc2e18c2f173f0bc9c6bec5aa6c8250ed92c99137f79a8d4a5c0d295e2476`.
The callback is classified `exact_unique_crossobject`, cdecl, and one
parameter. The documents explicitly preserve the rule that cross-build
evidence cannot replace January/XDK strict comparison.

Across these materials, the binary-first, C89, ABI, narrow-validation, and
fresh-artifact guidance was used as research discipline. Donor-only hooks,
raw addresses, compiler controls, configuration, caches, and completion labels
were never treated as production rules or comparator exceptions.

## Retained source and policy

The baseline C blob is `6cb93341454a2bad233263f58dc0ea9b2ee2d29b`;
the retained C blob is `8ece4b731294f1deaf3725245c50c00b2c95184c`.
Its canonical CRLF form is 12,605 bytes with 427 CRLF endings, zero bare LF
endings, and SHA-256
`4d2c39b0efaea1cb7d19e4b21ebf1c59a0d4ff5e24107331e7fa70e3e1a64c42`.
The untouched `rasterizer_xbox.h` remains blob
`c41ec24a916b27759cda6d856783152e11292b2f`; its 673-byte CRLF form has
36 CRLF endings, zero bare LF endings, and SHA-256
`f5acb3fb83dd56923612ae88a0f89d80128a63e0e835f7e3a1b8ed88db0d02c2`.

The sole retained definition is:

```c
void _rasterizer_set_vblank_callback(
	void (*callback)(unsigned long))
{
	D3DDevice_SetVerticalBlankCallback(callback);
	return;
}
```

It is translation-unit implementation detail and therefore adds no header
declaration. The body is readable C89, each parameter is on its own line, and
the void function has an explicit terminal return. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, compiler control,
barrier, raw address or raw offset access, pointer/integer reconstruction,
cast or union pun, undefined behavior, synthetic anchor, byte forcing,
storage definition, or comparator exception.

## First-shot discipline and strict comparison

The fresh isolated baseline object is 2,012 bytes with SHA-256
`6af1514022b479541d11e604b0f2a0c47155da02cf55b749a91fd0f9dbfbbe2c`.
The pre-wave whole-TU manifest is 1,715,466 bytes with SHA-256
`cb4f6a7c6c4aaf9cbedc66f994d3804f1c4469a513ae3469f306d41a4f5e7df7`.

The frozen source passed XDK `/Zs` with production include paths and flags;
the restored baseline artifact retained its exact size and hash. Its resolved
path was proved inside the isolated worktree, the literal object was deleted,
absence was verified, and the dry run named one edge. The ordinary selected
object edge then ran exactly once:

```text
[1/1] CL build\base\source\rasterizer\xbox\rasterizer_xbox.obj
rasterizer_xbox.c
```

The preserved first artifact is 2,221 bytes with SHA-256
`1b8eaabb36b2c8e5991723789a8ee706e9cedcbed4839c8c72e95f2b0074fdf6`.
There was no declaration, type, spelling, control-flow, source-shape, or
compiler-tuning retry.

Direct hardened comparison proves the new function and all three inherited
functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_reset_state` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_spin_begin` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_spin_end` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_set_vblank_callback` | 14 | 16 | 1 | `63bf78a6f07971df04769bae2ba18a6b90615e87a71be92f83beb002afa345bf` |
| **Strict total** | **17** | **64** | **1** | |

The already-exact public caller in `source/rasterizer/rasterizer.obj` also
remains strict: `_rasterizer_set_vblank_callback` is 9 meaningful / 16 padded
bytes, has one REL32 at `+5` to `__rasterizer_set_vblank_callback`, and has
normalized SHA-256
`6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`.

The generic pre-wave gate reports `NEWLY_EXACT`, an empty
`changed_nonexact` array, and the expected symbol-set change. Direct COFF
inventory proves the change is exactly the new external COMDAT plus its one
undefined XDK import. The three inherited 16-byte code COMDATs, their
associative debug sections, and the three inherited XDK encode-table rdata
COMDATs are unchanged. The candidate defines no BSS, tentative COMMON, or
runtime data owner and receives no matched-data credit. This reviewed direct
evidence is not a waiver or adjudication.

## Validation

The complete Halo and libcmt object graphs pass, and both they and the
`all_source` dry run report no pending work. Canonical outputs are:

- `build/report.json`: 1,574,677 bytes, SHA-256
  `c94383071ef2cc959304387a85fda378f6503b1d95d1bcb180e93ca266b4362a`;
- `build/semantic_report.json`: 2,984,352 bytes, SHA-256
  `de53121c842c40f723ba60346ea5388e17d4604ba534a3a8d810a7ea9420418f`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,156/11,060 exact functions,
  502,574/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,989/7,574 exact functions,
  489,660/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,263 functions:
4,121 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,466 missing,
4,182 accepted exact, and zero unit errors.

Admission is zero candidates and zero revocations, with only the inherited
unrelated `source/shell/shell_xbox` completion-label contradiction. Its
384-byte JSON has SHA-256
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`.
Parks remain three active, zero stale, and zero invalid; the 3,950-byte JSON
has SHA-256
`cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
All 179 tooling tests pass, and all 14 selected tracked/configuration/report
JSON documents parse.

The tracked scope is exactly
`source/rasterizer/xbox/rasterizer_xbox.c` and this ledger. There are no
deletions. The complete `source/units/**` tree and all eight protected paths
have zero intersection with the wave. No push, amend, rebase, history rewrite,
or worktree removal is performed.
