"""Integrator admission helper: flip config.json statuses, append semantic data entries, drop rejections.

    python scratch/campaign/admit.py --match source/x/y.c [...] [--unmatch source/..c]
        [--data-entry entry.json ...] [--drop-rejection <unit> <symbol-prefix>] ...
All edits are textual and in place (no JSON re-serialisation of config.json / rejections).
"""
import json
import re
import sys

args = sys.argv[1:]
match, unmatch, entries, drops = [], [], [], []
i = 0
while i < len(args):
    a = args[i]
    if a == '--match':
        match.append(args[i + 1]); i += 2
    elif a == '--unmatch':
        unmatch.append(args[i + 1]); i += 2
    elif a == '--data-entry':
        entries.append(args[i + 1]); i += 2
    elif a == '--drop-rejection':
        drops.append((args[i + 1], args[i + 2])); i += 3
    else:
        sys.exit('bad arg ' + a)


def edit_status(names, new):
    p = 'config/config.json'
    raw = open(p, 'rb').read().decode('utf-8')
    for n in names:
        pat = re.compile(r'(\{ "name": "%s", "index": \d+, "status": ")(\w+)(" \})' % re.escape(n))
        hits = pat.findall(raw)
        if len(hits) != 1:
            sys.exit('status: %d hits for %s' % (len(hits), n))
        raw = pat.sub(lambda m: m.group(1) + new + m.group(3), raw)
        print('status', n, hits[0][1], '->', new)
    open(p, 'wb').write(raw.encode('utf-8'))


if match:
    edit_status(match, 'Matching')
if unmatch:
    edit_status(unmatch, 'NonMatching')

if entries:
    p = 'config/semantic_data_matches.json'
    raw = open(p, 'rb').read().decode('utf-8')
    nl = '\r\n' if '\r\n' in raw else '\n'
    body = raw.rstrip()
    assert body.endswith(']'), 'unexpected end'
    body = body[:-1].rstrip()
    for e in entries:
        obj = json.load(open(e))
        txt = json.dumps(obj, indent=2).replace('\n', nl)
        txt = nl.join('  ' + l for l in txt.split(nl))
        body += ',' + nl + txt
        print('data entry', obj['unit'], obj.get('symbol'))
    open(p, 'wb').write((body + nl + ']' + nl).encode('utf-8'))
    json.load(open(p))

if drops:
    p = 'config/object_admission_rejections.json'
    d = json.load(open(p))
    before = len(d['entries'])
    for unit, sym in drops:
        d['entries'] = [e for e in d['entries'] if not (e['unit'] == unit and e['symbol'].startswith(sym))]
    print('rejections', before, '->', len(d['entries']))
    raw = open(p, 'rb').read().decode('utf-8')
    nl = '\r\n' if '\r\n' in raw else '\n'
    open(p, 'wb').write((json.dumps(d, indent=2, ensure_ascii=False).replace('\n', nl) + nl).encode('utf-8'))
