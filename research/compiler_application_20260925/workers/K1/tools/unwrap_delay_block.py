"""Remove the declaration-less bare block around the delay/look/speech section (dedent one tab). usage: IN OUT"""
import sys
src, out = sys.argv[1], sys.argv[2]
N = '\r\n'
lines = open(src, newline='').read().split(N)
start = None
for i in range(len(lines) - 1):
    if lines[i] == '\t\t\t\t{' and lines[i + 1].startswith('\t\t\t\t\tdelay_time = (short)('):
        start = i
        break
assert start is not None
depth = 0
end = None
for j in range(start, len(lines)):
    depth += lines[j].count('{') - lines[j].count('}')
    if depth == 0:
        end = j
        break
assert lines[end] == '\t\t\t\t}', repr(lines[end])
body = [l[1:] if l.startswith('\t') else l for l in lines[start + 1:end]]
lines = lines[:start] + body + lines[end + 1:]
open(out, 'w', newline='').write(N.join(lines))
print('unwrapped lines', start + 1, end + 1)
