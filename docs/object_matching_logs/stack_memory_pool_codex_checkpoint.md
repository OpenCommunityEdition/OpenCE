# `stack_memory_pool.obj` Codex checkpoint

Date: 2026-08-20

This is a Codex-owned checkpoint ledger. It does not replace or modify any
Claude-authored research or Markdown.

## Scope and provenance

- Published baseline: `a0bad121b7b4e37335acb3ab9a7825fb99861a7a`.
- Unit: `source/memory/stack_memory_pool`.
- Configuration remains `NonMatching`.
- The current source was revalidated from the strongest readable-C donor at
  `94c0c7539008a6cb266baa42e10cc27991879d63`.
- All 27 local `claude/*` worktrees were also audited. None had uncommitted
  edits to this unit. The best Claude source blob was `d000cf521c3f0c6e46352a06cbd5e635b4240456`
  at 26/33 strict functions; it supplied corroborating names, assert strings,
  and source-topology evidence but was superseded by the 30/33 donor.

The five user-frozen objects (`vehicles`, `ai_debug`, `ai_script`, `actions`,
and `units`) are outside this checkpoint and were not edited.

## Hardened strict result

The XDK 3911 rebuild and `tools.coff_compare`/regression-gate inventory report:

- 30/33 target functions strict-exact.
- 3,984/4,944 padded function bytes strict-exact.
- 3,729 meaningful function bytes strict-exact.
- 250/303 function relocations are carried by strict-exact functions.
- All 24 target-owned RDATA COMDATs are strict-exact: 966/966 bytes and zero
  relocations.
- The target owns no DATA, BSS, or common symbol in this unit.
- The candidate-only two-byte `"p"` RDATA COMDAT remains an audited split-owner
  difference; it is not counted as target-owned exact data.

The complete function inventory is all 33 target functions. Every function is
strict-exact except the three residuals below. Relative to the published
baseline, this preserves all 14 baseline-exact functions and adds 16 strict
functions, 2,848 padded exact bytes, and 183 relocations. Target-owned RDATA
improves from 21/24 sections (797 bytes) to 24/24 (966 bytes).

## Residuals

| Function | Target/candidate size | Target/candidate relocs | Target hash | Candidate hash | Disposition |
|---|---:|---:|---|---|---|
| `_code_0010e490` | 128/128 | 4/4 | `9ced5444b8465ec8...` | `48c8ee3b769d843b...` | Redundant-guard/tail-merge optimizer fixed point. |
| `_code_0010ea00` | 640/640 | 36/36 | `00ddbe408b35e153...` | `c49ac9850cc6a1eb...` | Value-lifetime and null-epilogue merge fixed point. |
| `_code_0010ef70` | 192/192 | 13/13 | `cc68570f2905b455...` | `4d1bc2fd72947f08...` | Private-ABI/register-lifetime reload fixed point. |

Each residual has already exceeded the documented legal-C hypothesis limit.
They remain uncredited and the object remains `NonMatching`.

## House and scope audit

The retained source uses readable typed C and contains no inline assembly,
`volatile` steering, forced inlining, pragmas, compiler barriers, object-byte
forcing, or undefined-behavior steering. It keeps one parameter per line and
explicit returns. The measured early exits and shared `unlock_block` epilogue
are readable control flow required by the strict target shape.

Only this ledger and `source/memory/stack_memory_pool.c` are in the checkpoint
diff. No configuration, symbol, park manifest, frozen-object source, or
pre-existing Markdown is changed.

## Validation

- Forced unit rebuild: passed.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Strict semantic audit: 470 units, 3,604 functions evaluated, 3,452
  `semantic_exact`, 3,536 `accepted_exact`, zero unit errors.
- Progress generation: passed.
- Admission audit: zero candidates, zero revocations; only the pre-existing
  `shell_xbox` contradiction remains.
- Park validation: seven active, zero stale, zero invalid.
- Tool tests: 179/179 passed.
- A clean final regression snapshot and forced-rebuild check are required on
  the local checkpoint commit before integration.
