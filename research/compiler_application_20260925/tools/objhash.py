"""sha256 of every build/base object with the COFF TimeDateStamp (bytes 4..8) masked; writes path->hash JSON."""
import hashlib, json, sys
from pathlib import Path
out = {}
for p in sorted(Path('build/base').rglob('*.obj')):
    b = bytearray(p.read_bytes()); b[4:8] = b'\0\0\0\0'
    out[p.as_posix()] = hashlib.sha256(b).hexdigest()
Path(sys.argv[1]).write_text(json.dumps(out, indent=0))
print(len(out), 'objects')
