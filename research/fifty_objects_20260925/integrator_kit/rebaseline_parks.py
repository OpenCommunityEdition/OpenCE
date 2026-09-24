"""Re-baseline named stale parks to the current build's base measurements (textual, in place).

    python scratch/campaign/rebaseline_parks.py "<unit>::<function>" [...] [--note "text"]

Runs tools.parked_functions validation against build/report.json, and for each NAMED key that is
stale only in its base block / objdiff_percent (never target), rewrites those lines in
config/parked.json. With --note, appends the sentence to that entry's evidence string.
Any other stale or invalid entry is reported and left alone.
"""
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.parked_functions import validate_parked_functions  # noqa: E402

root = Path(__file__).resolve().parents[2]
args = sys.argv[1:]
note = None
if '--note' in args:
    i = args.index('--note')
    note = args[i + 1]
    del args[i:i + 2]
wanted = set(args)

res = validate_parked_functions(root, root / 'build' / 'report.json', root / 'objdiff.json',
                                root / 'config' / 'parked.json')
path = root / 'config' / 'parked.json'
raw = path.read_bytes().decode('utf-8')
nl = '\r\n' if '\r\n' in raw else '\n'
lines = raw.split(nl)
done = []
for item in res['stale']:
    key = '%s::%s' % (item['unit'], item['function'])
    if key not in wanted:
        print('stale (not named, left alone):', key, item.get('differences'))
        continue
    if any(d['measurement'].startswith('target') for d in item['differences']):
        sys.exit('ABORT: target measurement changed for ' + key)
    fn_pat = '"function": %s,' % json.dumps(item['function'])
    unit_pat = '"unit": %s,' % json.dumps(item['unit'])
    hits = [k for k, l in enumerate(lines) if l.strip() == fn_pat and lines[k - 1].strip() == unit_pat]
    if len(hits) != 1:
        sys.exit('ABORT: %d entry matches for %s' % (len(hits), key))
    k = hits[0]
    cur = item['measurements']
    end = k
    depth = 1  # entry '{' opened on line k-2
    for j in range(k, len(lines)):
        depth += lines[j].count('{') - lines[j].count('}')
        if depth <= 0:
            end = j
            break
    span = range(k, end + 1)
    b = next(j for j in span if lines[j].strip().startswith('"base":'))
    if lines[b].strip() == '"base": {':
        for j, field in ((b + 1, 'size'), (b + 2, 'relocation_count'), (b + 3, 'normalized_sha256')):
            m = re.match(r'^(\s*"%s": )(.*?)(,?)$' % field, lines[j])
            if not m:
                sys.exit('ABORT: layout of %s near line %d' % (key, j + 1))
            lines[j] = m.group(1) + json.dumps(cur['base'][field]) + m.group(3)
    else:
        m = re.match(r'^(\s*"base": )\{.*\}(,?)$', lines[b])
        if not m:
            sys.exit('ABORT: compact layout of %s near line %d' % (key, b + 1))
        lines[b] = m.group(1) + '{ "size": %d, "relocation_count": %d, "normalized_sha256": %s }' % (
            cur['base']['size'], cur['base']['relocation_count'], json.dumps(cur['base']['normalized_sha256'])) + m.group(2)
    o = next(j for j in span if lines[j].strip().startswith('"objdiff_percent":'))
    m = re.match(r'^(\s*"objdiff_percent": )(.*?)(,?)$', lines[o])
    lines[o] = m.group(1) + json.dumps(cur['objdiff_percent']) + m.group(3)
    if note:
        e = next(j for j in span if lines[j].strip().startswith('"evidence":'))
        m = re.match(r'^(\s*"evidence": ")(.*)(",?)$', lines[e])
        lines[e] = m.group(1) + m.group(2) + '  ' + json.dumps(note)[1:-1] + m.group(3)
    done.append(key)
    print('rebaselined', key, cur['base'], cur['objdiff_percent'])
missing = wanted - set(done)
if missing:
    print('named but not stale (unchanged):', sorted(missing))
path.write_bytes(nl.join(lines).encode('utf-8'))
json.loads(path.read_text(encoding='utf-8'))
for inv in res['invalid']:
    print('invalid (left alone):', inv)
