import re
src = open('scratch/tools/object_audit.py', encoding='utf-8').read()
out = src.replace("    t = cc.load(open('build/split/%s.obj' % unit, 'rb').read())",
                  "    t = cc.load(open(os.path.join(os.environ.get('SPLIT_ROOT', 'build/split'), '%s.obj' % unit), 'rb').read())", 1)
out = out.replace("ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))",
                  "ROOT = 'C:/halo-worktrees/claude-compiler-application-20260925'", 1)
out = out.replace('"""Whole-object admission audit', '"""A2 copy (only changes: SPLIT_ROOT env overrides build/split; ROOT pinned). Whole-object admission audit', 1)
open('research/compiler_application_20260925/workers/A2/tools/object_audit_split.py', 'w', encoding='utf-8', newline='\n').write(out)
