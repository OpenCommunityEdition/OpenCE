# Five further authenticated libcmt assembly helpers

This continues the authenticated-MASM lane recorded in
`libcmt_three_arithmetic_asm_20260923.md`. The source units are
`strrchr.asm`, `strncmp.asm`, `ulldiv.asm`, `ullrem.asm`, and `ulldvrm.asm`.
The XDK 3911 `libcmt.lib` members' `.file` records name the corresponding
`..\i386\*.asm` translation units, and their CodeView records name Microsoft
Macro Assembler 6.15.8803. Each authentic member independently matches its
January split runtime section before the new source is considered.

| Object / public owner | Text bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `strrchr.obj` / `_strrchr` | 39 | 0 | `61e62304347e294cb52e76f3b5cc064b748e44dd24d48e3550c00e35746640a3` |
| `strncmp.obj` / `_strncmp` | 56 | 0 | `e2f944563c7483cd4085f1878325de050424e2b192c3220c5e73a802f698e0da` |
| `ulldiv.obj` / `__aulldiv` | 104 | 0 | `84d17e172ff5e6bce23b7837cc15439649f685d00b3cdff9cc07cae79a0a88f2` |
| `ullrem.obj` / `__aullrem` | 117 | 0 | `e85ca4198a284fa33b4141d18479ff2edfded90a25e35567ac9d55bd2bccb611` |
| `ulldvrm.obj` / `__aulldvrm` | 149 | 0 | `106b501e819fc91bbe63e9d1399d58966a0ad64e9db30078334ccdf21f01fdf8` |

Each modern-ML source rebuild has exactly one nonempty runtime `.text`
section, flags `0x60500020`, no relocation, no runtime data, and exactly the
listed external public owner at offset zero with COFF type `0x20` and storage
class 2. The strict `section_infos_equal` result is true against **both**
the January split and the independently extracted XDK 3911 archive member
for all five. Empty `.text$mn`/`.data` and `.debug$S` are MASM bookkeeping.

A link-only probe references these five and the three earlier helpers. VC7
`Link.Exe` succeeded with the new candidate objects before and after the
authentic XDK archive on the input line. Both maps selected all eight new
objects for their symbols; no probe binary was executed. The source contains
mnemonics and labels, not copied bytes, binary directives, or object patches.

Central admission still requires config indices 689, 692, 796, 794, and 795
to switch from absent `.c` to these `.asm` sources, then a fully regenerated
Ninja build, stable regression and ownership audits, tests, and final diff
check. Together with the three earlier helpers this would yield **eight
whole-object closures and 718 exact original-library ASM bytes, but zero
recovered-C code bytes**. Keep that provenance classification distinct from
normal C reconstruction accounting.

The related `_stricmp` and `_strnicm` trials were **not** admitted: modern ML
encoded equivalent register/register instructions differently (and one loop
alignment no-op had a different length), so their candidate text failed strict
comparison. Those production trial files were removed; their current
`MISSING` status must remain unchanged. The more complex `chkstk` alias,
`strstr` cross-object label, and large `memchr` member were not part of this
admission batch.
