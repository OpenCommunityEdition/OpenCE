"""Run scratch/tools/{pdb_storage,surplus_identity}.py against a CANDIDATE object instead of build/base/<unit>.obj.
The tool source is read unchanged and only its hard-coded 'build/base/%s.obj' path is redirected (read-only).

    python battery_wrap.py pdb_storage|surplus_identity <unit> <candidate.obj>
"""
import os
import sys

WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
tool, unit, cand = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
src = open(os.path.join(WT, 'scratch', 'tools', tool + '.py'), encoding='utf-8').read()
needle = "'build/base/%s.obj' % unit"
assert src.count(needle) == 1, 'unexpected tool source'
src = src.replace(needle, repr(cand))
os.chdir(WT)
sys.argv = [tool + '.py', unit]
exec(compile(src, tool + '.py', 'exec'), {'__name__': '__main__'})
