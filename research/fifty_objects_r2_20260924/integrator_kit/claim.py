"""Agent claim registry for the r2 fifty-object lane (atomic; safe for concurrent agents).

    python scratch/campaign/claim.py claim   <unit> <agent> "<reason>"   -> exit 0 if claimed, 3 if held by another
    python scratch/campaign/claim.py release <unit> <agent> "<outcome>"
    python scratch/campaign/claim.py show [<unit>]
    python scratch/campaign/claim.py reserve <unit> <owner> "<reason>"   (integrator only: permanent external hold)

<unit> is a source unit without extension, e.g. source/ai/actor_combat.
Claims live in scratch/campaign/claims.json, guarded by an O_EXCL lock file. A unit
claimed by another agent (or reserved) must not be edited or probed for landing by you;
read-only study is fine. Every claim/release is appended to claims.log.
"""
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REG = os.path.join(HERE, 'claims.json')
LOG = os.path.join(HERE, 'claims.log')
LOCK = REG + '.lock'


def locked(fn):
    for _ in range(600):
        try:
            fd = os.open(LOCK, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            break
        except FileExistsError:
            time.sleep(0.1)
    else:
        sys.exit('claim registry lock timeout: ' + LOCK)
    try:
        data = json.load(open(REG, encoding='utf-8')) if os.path.exists(REG) else {}
        rc = fn(data)
        tmp = REG + '.tmp'
        json.dump(data, open(tmp, 'w', encoding='utf-8'), indent=1, sort_keys=True)
        os.replace(tmp, REG)
        return rc
    finally:
        os.close(fd)
        os.remove(LOCK)


def log(line):
    with open(LOG, 'a', encoding='utf-8') as fh:
        fh.write(time.strftime('%Y-%m-%d %H:%M:%S ') + line + '\n')


def norm(u):
    u = u.replace('\\', '/').strip()
    if u.endswith('.c'):
        u = u[:-2]
    if not u.startswith('source/') and not u.startswith('libs/'):
        u = 'source/' + u
    return u


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    cmd = a[0]
    if cmd == 'show':
        data = json.load(open(REG, encoding='utf-8')) if os.path.exists(REG) else {}
        for u, c in sorted(data.items()):
            if len(a) > 1 and norm(a[1]) != u:
                continue
            print('%-60s %-9s %-28s %s' % (u, c['state'], c['agent'], c.get('reason', '')[:90]))
        return 0
    unit, agent, text = norm(a[1]), a[2], (a[3] if len(a) > 3 else '')

    def do(data):
        cur = data.get(unit)
        if cmd == 'claim':
            if cur and cur['state'] in ('claimed', 'reserved') and cur['agent'] != agent:
                print('HELD by %s (%s): %s' % (cur['agent'], cur['state'], cur.get('reason', '')))
                return 3
            data[unit] = {'state': 'claimed', 'agent': agent, 'reason': text, 'since': time.time(),
                          'history': (cur or {}).get('history', [])}
            log('CLAIM %s %s %s' % (unit, agent, text))
            print('CLAIMED', unit)
            return 0
        if cmd == 'release':
            if not cur or cur['agent'] != agent:
                print('not your claim:', unit, cur and cur['agent'])
                return 4
            hist = cur.get('history', []) + [{'agent': agent, 'outcome': text, 'at': time.time()}]
            data[unit] = {'state': 'released', 'agent': agent, 'reason': text, 'since': time.time(), 'history': hist}
            log('RELEASE %s %s %s' % (unit, agent, text))
            print('RELEASED', unit)
            return 0
        if cmd == 'reserve':
            data[unit] = {'state': 'reserved', 'agent': agent, 'reason': text, 'since': time.time(),
                          'history': (cur or {}).get('history', [])}
            log('RESERVE %s %s %s' % (unit, agent, text))
            print('RESERVED', unit)
            return 0
        sys.exit('unknown command ' + cmd)

    return locked(do)


if __name__ == '__main__':
    sys.exit(main() or 0)
