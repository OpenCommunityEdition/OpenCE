"""Splice-test unique versions of given functions from EVERY branch's units.c."""
import subprocess, sys, re, hashlib, os
sys.path.insert(0, 'tools')
import coff_compare as cc

names = sys.argv[1:]

branches = subprocess.run(['git', 'for-each-ref', '--format=%(refname:short)', 'refs/heads'],
                          capture_output=True, text=True).stdout.split()
blobs = {}
for b in branches:
    r = subprocess.run(['git', 'rev-parse', f'{b}:source/units/units.c'], capture_output=True, text=True)
    if r.returncode == 0:
        blobs.setdefault(r.stdout.strip(), b)
print(f'{len(branches)} branches, {len(blobs)} unique units.c blobs', flush=True)


def get_fn(text, name):
    for m in re.finditer(r'^[A-Za-z_][^\r\n;]*\b' + re.escape(name) + r'\(', text, re.M):
        start = m.start()
        brace = text.find('{', m.end())
        semi = text.find(';', m.end())
        if brace == -1 or (semi != -1 and semi < brace):
            continue
        i = brace
        depth = 0
        while i < len(text):
            c = text[i]
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    return text[start:text.index('\n', i) + 1]
            i += 1
    return None


cur = open('source/units/units.c', encoding='latin-1').read()
cur_lf = cur.replace('\r\n', '\n')
crlf = '\r\n' in cur

bn = open('build.ninja').read()
key = 'build build\\base\\source\\units\\units.obj:'
i = bn.index(key)
j = bn.index('cflags = ', i)
k = bn.index('\nbuild ', j)
cf = re.sub(r'\s+', ' ', bn[j + 9:k].replace('$\n', ' ')).strip()
toks = [('/I' + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in re.findall(r'/I"[^"]+"|\S+', cf)]
CL = r'C:\halo-worktrees\claude-untried-objects\xbox\bin\vc7\CL.Exe'
target = cc.load(open('build/split/source/units/units.obj', 'rb').read())

texts = {}
for blob, b in blobs.items():
    r = subprocess.run(['git', 'cat-file', 'blob', blob], capture_output=True)
    texts[b] = r.stdout.decode('latin-1').replace('\r\n', '\n')


def gate(code_lf, fname):
    body = code_lf.replace('\n', '\r\n') if crlf else code_lf
    open('scratch/_hunt.c', 'w', encoding='latin-1', newline='').write(body)
    if os.path.exists('scratch/_hunt.obj'):
        os.remove('scratch/_hunt.obj')
    r = subprocess.run([CL, '/nologo', '/c'] + toks + ['/Isource/units', '/Foscratch/_hunt.obj', 'scratch/_hunt.c'],
                       capture_output=True, text=True)
    if not os.path.exists('scratch/_hunt.obj'):
        return 'COMPILE-FAIL'
    ours = cc.load(open('scratch/_hunt.obj', 'rb').read())
    try:
        ti = cc.section_info(target, fname)
        oi = cc.section_info(ours, fname)
    except Exception:
        return 'NO-SYM'
    return 'EXACT' if cc.section_infos_equal(ti, oi) else 'residual'


for name in names:
    plain = name.lstrip('_')
    cur_fn = get_fn(cur_lf, plain)
    if cur_fn is None:
        print(f'{name}: not in current source', flush=True)
        continue
    seen = {hashlib.sha256(cur_fn.encode()).hexdigest()}
    for b, text in texts.items():
        v = get_fn(text, plain)
        if v is None:
            continue
        h = hashlib.sha256(v.encode()).hexdigest()
        if h in seen:
            continue
        seen.add(h)
        verdict = gate(cur_lf.replace(cur_fn, v, 1), name)
        print(f'{name}: {b} -> {verdict}', flush=True)
        if verdict == 'EXACT':
            open(f'scratch/win_{plain}.c', 'w', encoding='latin-1', newline='').write(v)
