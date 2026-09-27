"""Safe alternate-root management for A1 (never follows the shared xbox junction).

    python mkroot.py clone <src_root> <dst_root>   # copy <src_root>/source only, then add an xbox junction
    python mkroot.py rm <root>                     # unlink the xbox junction FIRST (os.rmdir), then delete

Roots must live under scratch/campaign/workers/A1/roots/.
"""
import os
import shutil
import subprocess
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
ROOTS = os.path.join(WT, 'scratch', 'campaign', 'workers', 'A1', 'roots')
XBOX = r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\halo-exact\xbox'


def check(path):
    p = os.path.normcase(os.path.abspath(path))
    assert p.startswith(os.path.normcase(ROOTS) + os.sep), 'refusing: %s is not under %s' % (path, ROOTS)
    return os.path.abspath(path)


def unlink_xbox(root):
    j = os.path.join(root, 'xbox')
    if os.path.lexists(j) or os.path.isdir(j):
        os.rmdir(j)  # removes the junction itself; never the target's contents
    assert os.path.isdir(os.path.join(XBOX, 'include')), 'shared xbox target missing!'


def junction(root):
    j = os.path.join(root, 'xbox')
    if not os.path.isdir(j):
        subprocess.run(['cmd', '/c', 'mklink', '/J', j, XBOX], check=True, capture_output=True)
    assert os.path.isdir(os.path.join(j, 'include'))


def rm(root):
    root = check(root)
    if not os.path.exists(root):
        return
    unlink_xbox(root)
    assert not os.path.exists(os.path.join(root, 'xbox'))
    shutil.rmtree(root)


def export(dst, rev='HEAD'):
    """git archive <rev> source into <dst> (CRLF via the repo's core.autocrlf, like the working tree)."""
    dst = check(dst)
    rm(dst)
    os.makedirs(dst)
    arc = subprocess.run(['git', '-C', WT, 'archive', rev, 'source'], check=True, capture_output=True).stdout
    subprocess.run(['tar', '-x', '-C', dst], input=arc, check=True)
    junction(dst)


def clone(src, dst):
    src, dst = check(src), check(dst)
    rm(dst)
    os.makedirs(dst)
    shutil.copytree(os.path.join(src, 'source'), os.path.join(dst, 'source'))
    junction(dst)


if __name__ == '__main__':
    if sys.argv[1] == 'clone':
        clone(sys.argv[2], sys.argv[3])
    elif sys.argv[1] == 'export':
        export(sys.argv[2], *(sys.argv[3:4]))
    elif sys.argv[1] == 'rm':
        rm(sys.argv[2])
    elif sys.argv[1] == 'junction':
        junction(check(sys.argv[2]))
    print('ok', sys.argv[1:])
