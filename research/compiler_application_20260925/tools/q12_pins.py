"""Pin the Q12 evidence: archive the core checked objects and hash everything that was read or written.

usage (worktree root): python -B research/compiler_application_20260925/tools/q12_pins.py <gates/Q12 dir> <objs.json>
objs.json = {"all": [...every checked object...], "core": [...objects to archive...]} (paths relative to the root).
Writes:
  objects/<build path>          byte copies of the core objects (units and link-receipt providers, January and ours)
  PINS_objects.tsv              every checked object: raw sha256, TimeDateStamp-masked sha256, size, archived copy
  PINS.sha256                   sha256sum -c compatible (run from the worktree root) for every file that persists in
                                the repository: archived objects, logs, receipts, tools, configs
Raw hashes of build/ objects hold for this build instance only (the COFF TimeDateStamp changes on every rebuild);
the masked hash and the archived copies stay verifiable."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def h(data):
    return hashlib.sha256(data).hexdigest()


def masked(data):
    b = bytearray(data)
    b[4:8] = b'\0\0\0\0'
    return h(bytes(b))


def main():
    out = Path(sys.argv[1]).resolve()
    sel = json.loads(Path(sys.argv[2]).read_text())
    arch = out / 'objects'
    rows = ['path\traw_sha256\ttimestamp_masked_sha256\tbytes\tarchived_copy']
    for rel in sel['all']:
        data = (ROOT / rel).read_bytes()
        copy = ''
        if rel in sel['core']:
            dst = arch / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / rel, dst)
            assert dst.read_bytes() == data
            copy = dst.relative_to(ROOT).as_posix()
        rows.append('%s\t%s\t%s\t%d\t%s' % (rel, h(data), masked(data), len(data), copy))
    (out / 'PINS_objects.tsv').write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    persist = sorted(p for p in out.rglob('*') if p.is_file() and p.name != 'PINS.sha256')
    extra = [ROOT / x for x in (
        'research/compiler_application_20260925/tools/data_identity.py',
        'research/compiler_application_20260925/tools/q12_evidence.py',
        'research/compiler_application_20260925/tools/q12_pins.py',
        'research/compiler_application_20260925/tools/debugs_objname.py',
        'research/compiler_application_20260925/tools/secdiff.py',
        'research/compiler_application_20260925/gates/Q12_identity.txt',
        'research/compiler_application_20260925/gates/Q12_ownership.txt',
        'scratch/tools/provider_link.py',
        'scratch/tools/pdb_storage.py',
        'scratch/tools/object_audit.py',
        'scratch/campaign/keyed_diff.py',
        'tools/semantic_progress.py',
        'tools/coff_compare.py',
        'config/semantic_data_matches.json',
        'config/symbols.json',
        'xbox/bin/vc7/Link.Exe',
    )]
    lines = ['%s  %s' % (h(p.read_bytes()), p.relative_to(ROOT).as_posix()) for p in persist + extra]
    (out / 'PINS.sha256').write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
    print('archived %d objects; PINS_objects.tsv %d rows; PINS.sha256 %d files' % (
        len(sel['core']), len(sel['all']), len(lines)))


main()
