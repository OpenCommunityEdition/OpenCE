"""E1 helper: write an LF-normalised unified diff (git-apply compatible) from a pristine repo file to a copy.

    python -B mkpatch.py <repo_relative_path> <copy_path> <out.patch> [<repo_relative_path2> <copy2> ...]

Both sides are read with CRLF folded to LF (the index stores LF; core.autocrlf=true), and the header uses
a/<path> b/<path> so `git apply --check <out.patch>` validates against HEAD."""
import difflib
import sys

args = sys.argv[1:]
out = args[-1]
pairs = list(zip(args[0:-1:2], args[1:-1:2]))
chunks = []
for rel, copy in pairs:
    a = open(rel, encoding='latin-1', newline='').read().replace('\r\n', '\n').splitlines(True)
    b = open(copy, encoding='latin-1', newline='').read().replace('\r\n', '\n').splitlines(True)
    d = list(difflib.unified_diff(a, b, 'a/' + rel, 'b/' + rel, n=3))
    if not d:
        continue
    chunks.append('diff --git a/%s b/%s\n' % (rel, rel))
    for line in d:
        if not line.endswith('\n'):
            line += '\n\\ No newline at end of file\n'
        chunks.append(line)
open(out, 'w', encoding='latin-1', newline='\n').write(''.join(chunks))
print('wrote', out, sum(1 for c in chunks if c.startswith('@@')), 'hunks')
