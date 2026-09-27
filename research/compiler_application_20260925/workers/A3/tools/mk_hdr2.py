"""A3: apply the HDR-2 proposal (single owner definition of struct hs_external_global_definition in hs.h) to an
alternate root. CRLF preserved; every replacement asserts exactly one match.

    python -B research/compiler_application_20260925/workers/A3/tools/mk_hdr2.py <root>
"""
import os
import sys

root = sys.argv[1]
NL = '\r\n'


def patch(rel, old, new):
    p = os.path.join(root, rel)
    t = open(p, 'rb').read().decode('latin-1')
    o, n = old.replace('\n', NL), new.replace('\n', NL)
    assert t.count(o) == 1, (rel, t.count(o), old[:60])
    open(p, 'wb').write(t.replace(o, n).encode('latin-1'))
    print('patched', rel)


FULL = ('struct hs_external_global_definition\n'
        '{\n'
        '\tchar const *name;\n'
        '\tshort type;\n'
        '\tshort pad;\n'
        '\tvoid *address;\n'
        '};\n')
patch('source/hs/hs.h', 'struct hs_external_global_definition;\n', FULL)
patch('source/hs/hs_globals_external.c', FULL + '\n', '')
patch('source/hs/hs.c',
      'struct hs_external_global_definition\n{\n\tchar const *name;\n\tshort type;\n};\n\n', '')
patch('source/hs/hs_runtime.c',
      'struct hs_external_global_definition\n{\n\tchar const *name;\n\tshort type;\n\tshort unused;\n\tvoid *address;\n};\n\n',
      '')
