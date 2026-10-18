#!/usr/bin/env python3
"""Index actual definitions for a Darwin spawn link failure; no code generation.

Candidate matches are lexical evidence, not proof that their provider compiles
or that its runtime initialization exists. Ambiguous names remain explicit.
"""
import argparse
import json
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('link_log', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
text = args.link_log.read_text()
names = sorted(set(re.findall(r'"_([^"\s]+)", referenced from:', text)))
sources = {}
for root in ('source', 'port/linux', 'port/mac'):
    for path in Path(root).rglob('*.c'):
        source = path.read_text(errors='replace')
        source = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: '\n' * m[0].count('\n'), source, flags=re.S)
        sources[str(path)] = source
rows = []
for name in names:
    candidates = []
    for path, source in sources.items():
        if name not in source:
            continue
        pattern = r'(?m)^(?:[A-Za-z_][\w \t*]*[ \t*])?' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{'
        for match in re.finditer(pattern, source):
            # Return types are frequently on the preceding line. Exclude
            # explicitly static definitions; they cannot satisfy the linker.
            prefix = source[max(0, match.start()-80):match.start()]
            if re.match(r'\s*static\b', match[0]) or re.search(r'\bstatic[^;{}]*\n\s*$', prefix):
                continue
            candidates.append({'path': path, 'line': source.count('\n', 0, match.start())+1, 'kind': 'function'})
        if candidates and any(c['path'] == path for c in candidates):
            continue
        pattern = r'(?m)^[A-Za-z_][\w \t*]*\b' + re.escape(name) + r'(?:\s*\[[^\]\n]*\])*\s*(?:=[^;]*;|;)'
        for match in re.finditer(pattern, source):
            line = match[0].strip()
            if re.match(r'(?:extern|static|return|typedef|goto)\b', line) or '(' in line.split('=')[0]:
                continue
            candidates.append({'path': path, 'line': source.count('\n', 0, match.start())+1, 'kind': 'global'})
        common = re.search(r'(?m)^HALO_COMMON\(\s*'+re.escape(name)+r'\s*,', source)
        if common:
            candidates.append({'path': path, 'line': source.count('\n', 0, common.start())+1, 'kind': 'linker_common_storage'})
    if not candidates:
        for path in Path('source').rglob('*.h'):
            header = path.read_text(errors='replace')
            pattern = r'(?m)^(?:[A-Za-z_][\w \t*]*[ \t*])?' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{'
            match = re.search(pattern, header)
            if match:
                candidates.append({'path': str(path), 'line': header.count('\n', 0, match.start())+1,
                                   'kind': 'header_inline_requires_emitting_translation_unit'})
    if candidates and all(c['kind'].startswith('header_inline') for c in candidates):
        status = 'engine_inline_provider_requires_native_emission'
    elif not candidates:
        status = 'no_c_definition_found_review_headers_or_platform'
    elif any(c['path'].startswith('source/') or c['path'].startswith('port/linux/game/') for c in candidates):
        status = 'engine_or_shared_game_provider'
    else:
        status = 'platform_provider_requires_native_port_review'
    rows.append({'symbol': name, 'status': status, 'candidates': candidates})
census_available = bool(re.search(r'^Undefined symbols:', text, re.M))
blocking_diagnostics = [line for line in text.splitlines() if re.search(r'duplicate symbol|symbol\(s\) not found|collect2:|ld: (?:error|file not found)', line)]
report = {'link_log': str(args.link_log), 'unresolved_count': len(rows) if census_available else None,
          'undefined_symbol_census_available': census_available,
          'link_complete': False if census_available or blocking_diagnostics else None,
          'blocking_diagnostics': blocking_diagnostics,
          'scope': 'Lexical provider census; no unresolved symbol is stubbed or treated as initialized.',
          'symbols': rows}
args.output.write_text(json.dumps(report, indent=2)+'\n')
if not census_available:
    print('No undefined-symbol census available; inspect linker diagnostics; link completion is not established')
    raise SystemExit(0)
print(f'{len(rows)} unresolved symbols; {sum(bool(r["candidates"]) for r in rows)} with definition candidates (including headers)')
