# `rasterizer_xbox_widgets.obj` Jonas z-buffer-enable leaf

## Result

This bounded lane starts from clean local commit
`2ccd77f6b2f820b992dfdb838aa1a269389b0d7c`. The first and only
code-producing production candidate is strict exact. The object advances from
1/25 to 2/25 exact functions and from 16 to 80 exact padded code bytes. It
remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_widget_set_zbuffer_enable` | 56 | 64 | 6 | `ae8498cc929333a837b1803c4e0ee8cf53e3119dc96e905aaf12e1924aca052e` |

The hardened comparator reports `all_equal: true` for every padded byte and
all ordered relocation identities. The six relocations are:

| Address | Type | Resolved destination |
| ---: | --- | --- |
| +4 | DIR32 | `_global_d3d_device` |
| +20 | DIR32 | target source-path string |
| +25 | DIR32 | target `global_d3d_device` string |
| +30 | REL32 | `_display_assert` |
| +37 | REL32 | `_system_exit` |
| +50 | REL32 | `_D3DDevice_SetRenderState_ZEnable@4` |

## Prior-attempt and provenance census

- The January split object is authoritative. Its SHA-256 is
  `c3055aafc2db35a6ae2565112df0dbae054f6ff79bd26f1ee3f7d2878f840112`.
- No object-specific Xbox Widgets ledger existed before this lane. The
  complete object-log index and applicable campaign, Claude, agent, and
  tooling documentation were read before the edit.
- All 1,524 registered worktrees were scanned for a definition of
  `_rasterizer_widget_set_zbuffer_enable`; none contains one. A complete
  `git log --all -S` search reaches only the initial skeleton commit
  `e830efbd`. No ref-reachable prior candidate exists.
- HCEA's exact-name donor
  `reference/halocea/src/_rasterizer_widget_set_zbuffer_enable.c` calls the
  typed XDK wrapper `D3DDevice_SetRenderState_ZEnable(zbuffer_enable)`.
- January disassembly proves the one-byte cdecl parameter, the
  `global_d3d_device` assertion at source line `0x14E`, and the stdcall XDK
  render-state wrapper. The public Rasterizer wrapper is a 16-byte tail jump
  to this function, independently proving its ordinary callable boundary.

The retained source is therefore the single natural January-concordant shape:

```c
void _rasterizer_widget_set_zbuffer_enable(
	boolean zbuffer_enable)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_widgets.c",
		0x14E,
		global_d3d_device);
	D3DDevice_SetRenderState_ZEnable(zbuffer_enable);

	return;
}
```

## One-shot and source-policy boundary

The baseline source Git blob was
`441bc8ea30bb2c6a6cd61d219240c7bca74c6951`; the retained source blob is
`6a38cf49a78e60f5cf37d906fed886d2e2a64fa3`. Before the production build,
the unchanged candidate passed VC7 `/Zs`; the existing object hash and
timestamp remained unchanged, proving that syntax check emitted no object.
The next invocation through the production Ninja edge emitted the only
candidate object, which matched immediately. There was no body, predicate,
declaration, placement, spelling, topology, or compiler-control retry.

Compilation used Microsoft 32-bit C/C++ Optimizing Compiler
`13.00.9254.1` with the repository's unchanged XDK 3911 flags. `CL.Exe` has
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The baseline candidate object SHA-256 was
`94da7ed046ee68ab44c008a0e7a49449cc308f34f37e5b543afdcca5058d346d`;
the retained rebuilt object SHA-256 is
`e112d0cd94dcbcced08127ebcddaf243be057ecd10526a777c92f1060495ddf`.

The retained body is readable typed C89 with one parameter per line and an
explicit terminal `return;`. It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, compiler barrier, raw address or offset access,
pointer/integer reconstruction, pointer or union punning, undefined behavior,
synthetic anchor, byte forcing, object patch, compiler/tool exception,
semantic allowlist, or parked entry. It changes no compiler flag,
configuration, symbol mapping, or protected source.

## Whole-TU regression and XDK header COMDATs

Direct hardened comparison preserves the inherited exact function:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_widget_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

The pre-edit regression manifest reports no changed nonexact target owner.
The new function, its two target-owned strings, and its associative debug
section are the expected natural additions. Including the XDK header also
emits three external header constants:

| COMDAT owner | Bytes | Relocations | SHA-256 |
| --- | ---: | ---: | --- |
| `_D3DTEXTUREDIRECTENCODE` | 16 | 0 | `2df64c1861ed539052d35303c159771b5caeaaed5bfa04b343eb47d48329f807` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | 0 | `e29e444c4976f669d2d4606f5795ee3a6c65d326bbdf8312b31dc735487075f3` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | 0 | `45dcae937d7b20c7c6c2bacbe90deabbbaa9311f99cd26e5387a6d2a21de1404` |

All three sections carry `IMAGE_SCN_LNK_COMDAT` with selection 2
(`IMAGE_COMDAT_SELECT_ANY`), and no relocation in the object references any
of them. Each is byte-identical to all 48 pre-existing candidate definitions
found in the production object tree, with zero mismatches. They are therefore
ordinary discardable duplicate XDK header COMDATs, not new target-owned data
or a tool exception.

## Validation and disposition

- Direct hardened comparison: the new function and inherited widget-end
  function pass complete padded-byte and resolved ordered-relocation identity.
- Whole-TU pre-edit regression manifest: pass with no inherited loss and no
  changed nonexact target owner.
- Object gate: 2 exact, 0 residual, 23 unwritten of 25 functions.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets: pass.
- Semantic audit: 470 units, 4,920 functions evaluated, 4,803 semantic exact,
  4,813 accepted exact, and zero unit errors or ordinary rejections.
- Strict campaign board: 4,785/8,246 exact functions and
  663,319/1,922,669 padded code bytes.
- Protected Units sentinel: 189/189 functions and 54,864/54,864 padded bytes
  strict exact.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid; this lane adds no
  park.
- Tooling suite: 212/212 tests pass.
- `git diff --check`, changed-source policy scan, deleted-path scan, and
  retained-path review: pass.

The function is strict exact and retained. The object is not eligible for
`Matching` until its remaining 23 functions and target-owned data are closed.
