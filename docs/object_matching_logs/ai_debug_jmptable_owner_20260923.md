# `ai_debug_render_actor` jump-table ownership (2026-09-23)

`_ai_debug_render_actor_jmptable` is a label at offset `0x6000` inside
January's 24,976-byte `_ai_debug_render_actor` `.text` section, not a separate
function or COFF section. The current candidate's corresponding table begins
at `0x6020` inside its 25,008-byte parent section. The 400-byte table tails
have 97 relocated dwords and 12 bytes of padding. Both tables have 91 unique
case destinations and the same equality/alias partition across all 97 entries.

The 89 differing destination offsets follow the parent function's code-layout
drift. There is no missing case or independent gate row to fix; crediting the
388 relocated table bytes separately would double-count the parent. The
reopen criterion is a house-rule-clean exact match for the enclosing
`_ai_debug_render_actor` section. No source, symbol or credit change was made
for the label in this audit.
