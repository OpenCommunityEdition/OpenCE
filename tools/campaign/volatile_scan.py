"""Line-indexed version of the volatile scan.

The find/replace version could not test 38 of the 54 sites because a 3-line
context is not unique in a file full of near-identical argument structs. This one
edits the real file by line number and restores it after each probe, so every
site is testable.
"""
import subprocess, os, re, json, shutil

SRC = 'source/hs/hs.c'
BAK = 'scratch/hs_scan_backup.c'
shutil.copy(SRC, BAK)
orig = open(SRC, encoding='latin-1').read()
lines = orig.split('\n')
sites = [i for i, l in enumerate(lines) if 'volatile ' in l]
print(f'{len(sites)} sites with a "volatile " token')


def gate():
    r = subprocess.run(['python', 'tools/campaign/gate.py', 'source/hs/hs'],
                       capture_output=True, text=True)
    if 'COMPILE FAILED' in r.stdout:
        return None
    m = re.search(r'== exact (\d+)\s+residual (\d+)', r.stdout)
    return int(m.group(1)) if m else None


def write(ls):
    open(SRC, 'w', encoding='latin-1', newline='\n').write('\n'.join(ls))


base = gate()
print(f'baseline: {base} exact\n')
removable, needed, failed = [], [], []
try:
    for i in sites:
        probe = list(lines)
        probe[i] = probe[i].replace('volatile ', '', 1)
        write(probe)
        got = gate()
        tag = lines[i].strip()[:62]
        if got is None:
            failed.append((i + 1, tag))
            print(f'  compile-fail  line {i+1:5d}  {tag}', flush=True)
        elif got == base:
            removable.append((i + 1, tag))
            print(f'  REMOVABLE     line {i+1:5d}  {tag}', flush=True)
        else:
            needed.append((i + 1, tag, got))
            print(f'  load-bearing  line {i+1:5d}  -> {got}  {tag}', flush=True)
finally:
    write(lines)
    print('\nsource restored')

print(f'== {len(removable)} removable, {len(needed)} load-bearing, {len(failed)} compile-fail')
json.dump({'removable': removable, 'needed': needed, 'failed': failed},
          open('scratch/volatile_scan2.json', 'w'), indent=1)
