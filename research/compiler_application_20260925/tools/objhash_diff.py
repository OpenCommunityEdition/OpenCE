import json, sys
a, b = json.load(open(sys.argv[1])), json.load(open(sys.argv[2]))
ch = [k for k in sorted(set(a) | set(b)) if a.get(k) != b.get(k)]
print(len(ch), 'objects differ'); [print(' ', k) for k in ch]
