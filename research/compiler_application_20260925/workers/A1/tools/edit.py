"""Tiny reproducible text-edit helper for A1 alternate roots (never touches the worktree's tracked files).

    python edit.py <root> <spec.py>

<spec.py> defines edit(sub, rm, write, read) and calls sub(relpath, old, new[, count]) etc. Files keep their
existing line endings: specs are written with LF and converted to the file's EOL before matching.
"""
import os
import sys

WT = os.path.normcase(os.path.abspath(r'C:\halo-worktrees\claude-compiler-application-20260925'))
root = os.path.abspath(sys.argv[1])
assert os.path.normcase(root).startswith(os.path.join(WT, 'scratch', 'campaign', 'workers', 'a1').lower()) or \
    'temp' in root.lower(), 'refusing to edit outside the A1 scratch roots: ' + root


def _p(rel):
    return os.path.join(root, rel.replace('/', os.sep))


def read(rel):
    return open(_p(rel), 'rb').read().decode('latin-1')


def write(rel, text, eol='\r\n'):
    text = text.replace('\r\n', '\n').replace('\n', eol)
    open(_p(rel), 'wb').write(text.encode('latin-1'))


def sub(rel, old, new, count=1):
    text = read(rel)
    eol = '\r\n' if '\r\n' in text else '\n'
    o = old.replace('\r\n', '\n').replace('\n', eol)
    n = new.replace('\r\n', '\n').replace('\n', eol)
    found = text.count(o)
    assert found == count, 'EDIT %s: expected %d match(es), found %d for %r' % (rel, count, found, old[:120])
    open(_p(rel), 'wb').write(text.replace(o, n).encode('latin-1'))


def rm(rel):
    os.remove(_p(rel))


spec = {}
exec(compile(open(sys.argv[2]).read(), sys.argv[2], 'exec'), spec)
spec['edit'](sub, rm, write, read)
print('edited', root, 'with', os.path.basename(sys.argv[2]))
