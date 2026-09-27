"""Write an LF git-style patch (a/<path> b/<path>, `git apply`-able on the autocrlf worktree) for a set of files
between two alternate roots. Files missing on one side become creations/deletions.

    python mkpatch.py <old_root> <new_root> <out.patch> <relpath> [<relpath> ...]
"""
import os
import shutil
import subprocess
import sys
import tempfile

old_root, new_root, out = sys.argv[1], sys.argv[2], sys.argv[3]
paths = sys.argv[4:]
tmp = tempfile.mkdtemp(prefix='a1patch_')
try:
    for side, root in (('a', old_root), ('b', new_root)):
        for rel in paths:
            src = os.path.join(root, rel)
            if os.path.exists(src):
                dst = os.path.join(tmp, side, rel)
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                open(dst, 'wb').write(open(src, 'rb').read().replace(b'\r\n', b'\n'))
        os.makedirs(os.path.join(tmp, side), exist_ok=True)
    env = dict(os.environ, GIT_CEILING_DIRECTORIES=tmp)
    r = subprocess.run(['git', '-c', 'core.autocrlf=false', 'diff', '--no-index', '--no-color', '--no-renames',
                        'a', 'b'], cwd=tmp, capture_output=True, env=env)
    text = r.stdout.decode('latin-1')
    # git --no-index labels the files a/a/<path> b/b/<path>; normalise to a/<path> b/<path>
    text = text.replace(' a/a/', ' a/').replace(' b/b/', ' b/').replace('--- a/a/', '--- a/').replace('+++ b/b/', '+++ b/')
    open(out, 'wb').write(text.encode('latin-1'))
    print('wrote %s (%d lines, %d files)' % (out, text.count('\n'), text.count('diff --git')))
finally:
    shutil.rmtree(tmp)
