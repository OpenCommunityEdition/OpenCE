"""Emulate the csplit step with a patched symbols.json, entirely under scratch/campaign/workers/A4/.
    python mksplit.py <label> [edits.json]
edits.json: list of {"find": "<exact full line text>", "replace": ["<line>", ...]}  (line-surgical; each find must
match exactly one line of config/symbols.json; replacement lines keep the file's EOL). No reserialisation.
Writes scratch/campaign/workers/A4/cfg_<label>/ (5 config files) and split_<label>/, then lists every object that
differs from build/split (byte compare)."""
import filecmp, json, os, shutil, subprocess, sys
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
os.chdir(WT)
label = sys.argv[1]
edits = json.load(open(sys.argv[2], encoding='utf-8')) if len(sys.argv) > 2 else []
base = os.path.join('scratch', 'campaign', 'workers', 'A4')
cfg = os.path.join(base, 'cfg_' + label)
out = os.path.join(base, 'split_' + label)
os.makedirs(cfg, exist_ok=True)
for f in ('config.json', 'contribs.json', 'relocs.json', 'splits.json'):
    shutil.copyfile(os.path.join('config', f), os.path.join(cfg, f))
raw = open(os.path.join('config', 'symbols.json'), 'rb').read()
eol = b'\r\n' if b'\r\n' in raw else b'\n'
lines = raw.split(eol)
for e in edits:
    find = e['find'].encode('utf-8')
    idx = [i for i, l in enumerate(lines) if l == find]
    if len(idx) != 1:
        sys.exit('find matched %d lines: %s' % (len(idx), e['find'][:100]))
    i = idx[0]
    lines[i:i + 1] = [r.encode('utf-8') for r in e['replace']]
open(os.path.join(cfg, 'symbols.json'), 'wb').write(eol.join(lines))
if os.path.exists(out):
    shutil.rmtree(out)
os.makedirs(out)
r = subprocess.run([os.path.join('build', 'tools', 'csplit.exe'), '-i', 'cachebeta.exe', '-p', cfg, '-o', out],
                   capture_output=True, text=True)
open(os.path.join(base, 'split_%s.log' % label), 'w').write(r.stdout + r.stderr)
if r.returncode:
    sys.exit('csplit failed rc=%d (see log)' % r.returncode)
diff = []
n = 0
for dp, dn, fn in os.walk(out):
    for f in fn:
        if not f.endswith('.obj'):
            continue
        n += 1
        p = os.path.join(dp, f)
        rel = os.path.relpath(p, out)
        q = os.path.join('build', 'split', rel)
        if not os.path.exists(q) or not filecmp.cmp(p, q, shallow=False):
            diff.append(rel)
print('split_%s: %d objects, %d differ from build/split' % (label, n, len(diff)))
for d in diff:
    print('   ', d)
