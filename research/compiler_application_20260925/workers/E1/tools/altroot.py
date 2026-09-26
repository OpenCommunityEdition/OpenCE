"""E1 helper: run an unmodified battery tool with its hard-coded paths redirected (read-only wrapper).

    python -B altroot.py --split <split_root> [--base-obj <unit>=<obj> ...] <tool.py> [tool args...]

Every open()/glob of 'build/split/...' is redirected to <split_root>/..., and 'build/base/<unit>.obj' to the given
candidate object. Used to run object_audit / pdb_storage / surplus_identity against a scratch csplit regenerated from a
proposed symbols.json, without touching build/split or the tools."""
import builtins
import glob as _glob
import os
import runpy
import sys

args = sys.argv[1:]
split_root = None
base_map = {}
while args and args[0].startswith('--'):
    if args[0] == '--split':
        split_root = args[1].rstrip('/')
        args = args[2:]
    elif args[0] == '--base-obj':
        u, o = args[1].split('=', 1)
        base_map['build/base/%s.obj' % u] = o
        args = args[2:]
    else:
        sys.exit('unknown option ' + args[0])


def redirect(p):
    if not isinstance(p, str):
        return p
    q = p.replace(os.sep, '/')
    if q in base_map:
        return base_map[q]
    if split_root and q.startswith('build/split/'):
        return split_root + '/' + q[len('build/split/'):]
    if split_root and q == 'build/split':
        return split_root
    return p


_open = builtins.open


def patched_open(file, *a, **k):
    return _open(redirect(file), *a, **k)


builtins.open = patched_open
_g = _glob.glob


def patched_glob(pattern, *a, **k):
    return _g(redirect(pattern), *a, **k)


_glob.glob = patched_glob
_relpath = os.path.relpath


def patched_relpath(path, start=os.curdir):
    return _relpath(path, redirect(start) if isinstance(start, str) else start)


os.path.relpath = patched_relpath
tool = args[0]
sys.argv = [tool] + args[1:]
runpy.run_path(tool, run_name='__main__')
