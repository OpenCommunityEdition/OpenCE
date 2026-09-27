"""K2: /Od slot census for render_actor (data only). Lists every [ebp-X] slot with its store/read/lea counts and the
VA of first/last access; flags write-only slots (stored, never read, never address-taken) and linear dead stores
(store followed by another store to the same slot with no read and no control transfer between).
    python -B od_slots.py <od_full.txt>"""
import re, sys, collections
L = []
for line in open(sys.argv[1]):
    m = re.match(r'([0-9a-f]{8})\s+(\S+)\s*(.*)$', line.rstrip())
    if m:
        L.append((int(m.group(1), 16), m.group(2), m.group(3)))
slot_re = re.compile(r'(byte|word|dword|qword)? ?(?:ptr )?\[ebp - (0x[0-9a-f]+)\]')
stats = collections.defaultdict(lambda: dict(st=0, rd=0, lea=0, first=None, last=None, w=set()))
events = []
for va, mn, ops in L:
    for m in slot_re.finditer(ops):
        off = int(m.group(2), 16)
        s = stats[off]
        s['first'] = s['first'] or va
        s['last'] = va
        s['w'].add(m.group(1) or '-')
        dst = ops.split(',')[0]
        if mn == 'lea':
            kind = 'lea'
        elif (mn in ('mov', 'movss', 'movsd') and slot_re.search(dst)) or mn.startswith('fst') or mn.startswith('fist'):
            kind = 'st'
        elif mn in ('add', 'sub', 'and', 'or', 'xor', 'inc', 'dec', 'shl', 'sar', 'shr') and slot_re.search(dst):
            kind = 'rd'   # RMW counts as read
        else:
            kind = 'rd'
        s[kind] += 1
        events.append((va, mn, ops, off, kind))
print('write-only slots (stored, never read/lea):')
for off, s in sorted(stats.items()):
    if s['rd'] == 0 and s['lea'] == 0:
        print('  -0x%x st=%d widths=%s first=%08x last=%08x' % (off, s['st'], sorted(s['w']), s['first'], s['last']))
print('linear dead stores (store; ...no read/jump...; store same slot):')
last_store = {}
for va, mn, ops in L:
    if mn.startswith('j') or mn in ('call', 'ret'):
        last_store.clear()
        continue
    for m in slot_re.finditer(ops):
        off = int(m.group(2), 16)
        dst = ops.split(',')[0]
        is_st = ((mn in ('mov', 'movss') and slot_re.search(dst)) or mn.startswith('fst') or mn.startswith('fist'))
        if is_st and mn != 'lea':
            if off in last_store:
                print('  -0x%x stored at %08x then again at %08x' % (off, last_store[off], va))
            last_store[off] = va
        else:
            last_store.pop(off, None)
