# `hs_compile.obj` inspect-parser closure (2026-09-09)

Base commit: `105d9acaacda589e21a4389d7e6bd69db0b42aba`

## Outcome

`_hs_parse_inspect` is now exact. The retained change initializes the already
required typed syntax-node local at declaration time with `hs_syntax_get`.
That is a natural source reconstruction, complies with the campaign's
declaration/initialization convention, and restores January's C2 register and
load schedule without a carrier, volatile access, pragma, assembly, or other
compiler-steering construct.

| function | meaningful bytes | padded bytes | relocations | normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hs_parse_inspect` | 191 | 192 | 15 | `412ed6da10b4221175a0f895c57e4bf17bf609b96e668738db554b94768211d7` |

The function name and linkage were already established by the owning HS
source and target symbol. No header or prototype relocation was required.

## Verification

- Hardened semantic COFF audit: exact instructions, relocations, and section
  shape; credited by both `objdiff` and `semantic-coff`.
- Combined semantic audit: no lost accepted-exact functions and no unit
  errors relative to the published base.
- `_point_from_line3d` emitted-symbol guard: passed.
- Scoped fake-match scan: zero review leads.
- `git diff --check`: passed.
