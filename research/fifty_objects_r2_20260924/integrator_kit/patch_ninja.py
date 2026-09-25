"""Patch build.ninja's cl rule to the absolute backslash CL path (run after every configure)."""
import os

BS = chr(92)
root = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
cl = BS.join([root.replace('/', BS), 'xbox', 'bin', 'vc7', 'CL.Exe'])
p = os.path.join(root, 'build.ninja')
s = open(p).read()
old = 'command = xbox/bin/vc7/CL.Exe'
n = s.count(old)
s = s.replace(old, 'command = ' + cl)
open(p, 'w').write(s)
print('patched', n, cl)
