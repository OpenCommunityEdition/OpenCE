"""Generate header-only MS inline weak definitions for Apple GCC engine units.

Run on the modern build host; does not inspect Xbox SDK headers. The initial
engine slice uses whole-source units and dead stripping. Undefined weak symbols
must be audited at link time before expanding the slice.
"""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.linux_msvc_semantics import scan_tags, scan_inline_functions, render

if __name__ == '__main__':
    headers = list(Path('source').rglob('*.h'))
    output = Path('build/mac-audit/mac_engine_semantics.h')
    output.parent.mkdir(parents=True, exist_ok=True)
    text = render(scan_tags(headers), scan_inline_functions(headers, True))
    # GCC does not macro-expand a name inside #pragma weak.
    text = text.replace("#pragma weak random\n", "#pragma weak halo_mac_game_random\n")
    output.write_text(text)
