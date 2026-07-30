# `rasterizer_xbox_vertex_shaders.obj` strict matching log

## Final status

- Object: `source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.obj`
- Compiler: XDK 3911 VC7 `13.00.9254.1`
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Functions: none
- Target-owned sections: `2/2` strict exact
- Status: `Matching`

## Reconstruction

The object contains a table describing 67 Xbox vertex shaders and the
immutable compiled shader instruction tokens referenced by that table.  The
table was already reconstructed, but its 34,628-byte read-only payload was
only declared as an external symbol.

The January target payload was recovered without interpretation loss and is
represented as little-endian `unsigned long` instruction tokens in
`rasterizer_xbox_vertex_shaders_data.inc`.  Keeping the payload as dwords
makes the generated shader instructions inspectable while reproducing the
original bytes.  The table continues to address the payload through a byte
pointer, preserving every original relocation addend.

This is data reconstruction.  It uses no assembly, byte patch, undefined
behavior, compiler-flag change, comparator exception, or semantic-match
credit.

## Strict evidence

| Symbol | Target/candidate size | Relocations | Target/candidate normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rdata_00293850` | `34,628/34,628` | `0/0` | `ecd7c682d76eb1c72f5fac8b47893bedc278eccb3fa6bd68bded2ee1462b4dd6` |
| `_vertex_shader_table` | `1,072/1,072` | `67/67` | `143ec2af77afb46098560069290c9ee4f6451d37d25c273f89c40d081e1cc32a` |

`tools.coff_compare.section_infos_equal` returns true for both symbols.
Every table relocation has the target address, `IMAGE_REL_I386_DIR32` type,
payload destination, and byte addend.

Candidate-only `.drectve` and `.debug$S` sections are normal compiler
metadata and own no January target bytes.

## Reopen conditions

None.  The complete target-owned object is strict exact.  Any future edit
must preserve both section fingerprints and all 67 relocation identities.
