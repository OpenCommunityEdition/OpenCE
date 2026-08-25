# Units declaration-format and explicit-return compliance (Jonas, 2026-08-25)

## Result and bounded scope

This byte-neutral compliance wave starts from clean cumulative Units commit
`5cf42dbbee6acafd5f9e3a0a61c183ade4a2b627`. It edits only
`source/units/units.c` and applies two established source rules to code added
during the Units campaign:

- the fifteen identified one-line declarations now place every formal on its
  own line; and
- the `void` helper `code_00198400` now ends with an explicit `return;`.

The declaration set is `code_0019a170`, `code_0019a640`, `code_00198170`,
`code_001981f0`, `code_00198230`, `code_00198050`, `code_00198070`,
`code_00198190`, `code_0019dff0`, `valid_real_normal2d`,
`biped_stop_melee_attack`, `first_person_weapon_message_from_unit`,
`weapon_stop_reload`, `weapon_prevents_grenade_throwing`, and
`ai_handle_killing_spree`.

No function body other than the terminal no-value return changed. No header,
protected source, configuration, semantic exception, parked record, completion
label, storage declaration, typed-access form, or unrelated formatting changed.
This wave claims no new matching function or data owner.

## Immutable authority and source identities

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Cumulative source blob at `5cf42dbb`:
  `a2d6ae15ed26a052c99290d65f7168b01e2ba074`.
- Retained source blob before commit:
  `c2ef1a3b08bec91cc2c534443f09732b6f3003cd`.
- XDK 3911 VC7 `CL.Exe` 13.00.9254.1: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Authenticated csplit v0.0.2 and objdiff-cli v3.3.1 SHA-256:
  `708f957c37caa66df90ade35d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
  and `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The normal generated rule remains `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with
the repository include graph unchanged.

## Parse-only gate and sole edited-source emission

Before editing, a fresh ordinary compile of the exact `5cf42dbb` source
established the isolated-worktree baseline. It was frozen as
`build/audit/units_format_return_baseline.obj`: 143,149 bytes, raw SHA-256
`6221d3181815e3e89bf1adedaea67820d68584171263bb5f3fdf39abd853cde6`.
That unedited baseline emission is not a candidate experiment.

After the bounded source packet, `git diff --check`, the one-path scope audit,
and the prohibited-construct scan passed. A production-flag VC7 `/Zs` pass
succeeded and left the frozen baseline object byte-identical. The next and only
code-producing action on edited source was the ordinary generated edge:

```text
[1/1] CL build\base\source\units\units.obj
units.c
```

The immutable first-shot object is 143,149 bytes with raw SHA-256
`709dbe8426c1eecde42023ad6daa8e79998b65678b5bd35bcf967ab16f751522`.
There was no retry, tuning, source adjustment, compiler-option change, or
second edited-source emission.

The raw COFF hash changes because source line/debug metadata changes. The
complete acceptance view is unchanged:

- all 211 baseline and candidate emitted code owners are common, with zero
  additions, removals, or changed hardened fingerprints; and
- all 184 baseline and candidate `.rdata`, `.data`, and `.bss` owners are
  common, with zero additions, removals, or changed hardened fingerprints.

## Complete Units censuses

The direct hardened comparison against all 189 January underscore-function
owners remains 168 strict exact / 12 emitted nonexact / 9 absent. The exact set
contains 32,320 padded code bytes and 1,387 relocation identities. Every one
of the 168 inherited exact owners remains exact.

The helper affected by the explicit-return source rule remains strict exact:

| January owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_00198400` | 448 | 24 | `6b902fccee7df603d88f668e360fa10a819bf8d6b54300f4210e8db825830e19` |

Across all 129 January `.rdata`, `.data`, and `.bss` owners, the result remains
126 strict exact / zero present nonexact / three absent, totaling 6,471 exact
logical bytes and seven relocations. No inherited data owner is lost.

The retained change is ordinary readable typed C. It adds no assembly, raw
address or byte-offset access, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined behavior, `volatile`, `register`,
optimizer pragma, intrinsic, force-inline/noinline/attribute annotation,
compiler barrier, synthetic anchor, or byte-forcing expression.

## Full pre-commit gates

- Complete `halobetacache_build libcmt_build`: pass, all 568 copied-build
  catch-up edges. The already-adjudicated Units object was current and did not
  compile. Warnings were inherited from unrelated translation units.
- Authenticated csplit and objdiff report generation: pass. `build/report.json`
  is 1,573,455 bytes with SHA-256
  `34258ba197881ea1e5ba773d6946caad5016ada45fd6a014ea943932c35b69ee`.
- Semantic audit: 470 units, 4,237 functions evaluated, 4,095 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,156 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 objects, 4,130/11,060 functions, 501,438 accepted
  code bytes, and 1,835,088 accepted data bytes. Halo is 273/468 objects,
  3,963/7,574 functions, and 488,524 accepted code bytes.
- Object admission: zero candidates and zero revocations; the sole
  contradiction is the inherited unrelated `source/shell/shell_xbox`
  completion label.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- Direct code/data censuses, complete code/runtime-owner A/B, helper
  fingerprint, scope check, policy scan, `git diff --check`, and final Units
  Ninja dry run: pass.

## Committed-state replay plan

No committed-state replay is claimed in the implementation commit. After the
source and this initial ledger are committed by Jonas Volman and the worktree
is clean, a one-unit accepted-state regression snapshot will pin that commit.
The generated Units object path will be resolved and proven inside this exact
isolated worktree, deleted with literal-path semantics, rebuilt once through
its ordinary Ninja edge, and checked immediately. Direct code/data censuses,
the helper fingerprint, full owner A/B, and a final Ninja dry run will be
repeated before one additive ledger-only replay commit. The same clean
snapshot/delete/rebuild/check sequence will then run at corrected HEAD. No
push, amend, rebase, history rewrite, or worktree removal is performed.
