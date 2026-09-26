"""K2: apply named edit sets to a CRLF source copy.  python apply.py <in.c> <out.c> <edit_id> [<edit_id> ...]
Edits live in edits.py as EDITS[id] = [(old, new), ...] with LF newlines; they are matched against the CRLF file."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from edits import EDITS
src = open(sys.argv[1], 'rb').read().decode('latin-1')
for eid in sys.argv[3:]:
    for old, new in EDITS[eid]:
        o = old.replace('\n', '\r\n'); n = new.replace('\n', '\r\n')
        twice = o.startswith('@@TWICE@@')
        if twice:
            o = o[len('@@TWICE@@'):]
        c = src.count(o)
        if c != (2 if twice else 1):
            sys.exit('edit %s: %d matches for %r' % (eid, c, old[:80]))
        src = src.replace(o, n)
open(sys.argv[2], 'wb').write(src.encode('latin-1'))
print('wrote', sys.argv[2], 'edits', sys.argv[3:])
