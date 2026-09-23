# Three authenticated libcmt arithmetic helpers

`ullshr`, `llmul`, and `lldiv` were original Microsoft Macro Assembler
translation units, not C routines. The XDK 3911 `libcmt.lib` archive has
SHA-256 `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its extracted members identify `..\i386\ullshr.asm`, `..\i386\llmul.asm`,
and `..\i386\lldiv.asm` in their `.file` records, and CodeView names MASM
6.15.8803. Their member SHA-256 values are, respectively,
`74fa17a76f6adb9d926d6a30c46d42646fd94c3c873d1356cb0b5364b7a1b3e9`,
`73056341590c56991ab00ad79c8659238ba5a08668edeeb8ce9b883d78e5e435`,
and `65185866bfe44480d8ec92033953b8c362bb9f56b3ae624c2484f390ea94239c`.

| January object / public owner | Runtime text | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `ullshr.obj` / `__aullshr` | 31 | 0 | `909b1a39b7337f2fa8d8626167f578bf92643cf163ee37f401893b5b813db166` |
| `llmul.obj` / `__allmul` | 52 | 0 | `3d0898d998acc8af3ea17032ff56fd1b963c685b072f669c52d4b8267f9d0a78` |
| `lldiv.obj` / `__alldiv` | 170 | 0 | `8964dcf0d769cb61dd4354feac00491ae8c1b34eb9627a4c5ec2d4350243d395` |

For each unit the authenticated member and January split own exactly one
nonempty runtime section, `.text`, with flags `0x60500020`, the same bytes,
and no relocations or runtime data. The only public runtime definition is the
listed function at offset zero, COFF type `0x20` and external storage class 2.
The member and split target therefore agree on code, data, owner, and symbol
shape, independently of the new source.

The new readable MASM sources use ordinary instructions and an authentic
16-byte-aligned `.text` segment. Modern installed `ml.exe /c /coff` assembles
each source to the same complete runtime section and public owner as January:
`section_infos_equal` is true for all three. Candidate-only zero-length
`.text$mn` and `.data` sections and `.debug$S` are assembler bookkeeping, not
additional runtime ownership. No object copying, byte directives, binary
patches, compiler flag changes, or comparator exceptions were used.

A link-only probe referencing all three symbols selected the new candidate
object for each provider in both candidate-first and archive-first input
orders with the original XDK archive present. Both links succeeded and their
maps named `ullshr_candidate.obj`, `llmul_candidate.obj`, and
`lldiv_candidate.obj` as the selected definitions. The probe was never run.

The pre-configuration full `ninja halobetacache_build libcmt_build` was clean
(`no work to do`). This does **not** yet build the new files because central
config still names the absent `.c` units. Coordinated admission must change
only config indices 793, 798, and 799 from `.c` to `.asm` with `Matching`,
then regenerate Ninja, build fully, verify stable regression and admission
audits, run tests, and check the final diff. The three objects can add three
whole-object matches and 253 exact library-code bytes, but those bytes must
be reported as authenticated original-assembly provenance, **zero recovered-C
code credit**, consistent with `llshr.asm`.
