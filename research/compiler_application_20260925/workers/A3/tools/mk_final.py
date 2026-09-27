"""A3: build the FINAL candidate files (CRLF preserved) from the probe copies, normalising hs_runtime.c to its own
'x = y' assignment style and adding the BUG comments the owner-gated HCEX-ABI option needs.

    python -B research/compiler_application_20260925/workers/A3/tools/mk_final.py

Outputs under workers/A3/final/:
  hs_runtime/source/hs/hs_runtime.c            landable (H-1, H-1c, H-2, H-3, H-4b, H-4c)
  hs_runtime_owner/source/hs/hs_runtime.c      owner-gated (landable + H-5, H-5b S2a, H-6, H-7 + BUG comments)
  pao/source/ai/path_obstacle_avoidance.c      landable (PAO-1..PAO-5)
  pao_opt/source/ai/path_obstacle_avoidance.c  pao + PAO-6 (optional)
"""
import os
import re

A = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
NL = '\r\n'


def load(*p):
    return open(os.path.join(A, *p), 'rb').read().decode('latin-1')


def save(text, *p):
    path = os.path.join(A, 'final', *p)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    assert '\n' not in text.replace(NL, '')
    open(path, 'wb').write(text.encode('latin-1'))
    print('wrote', os.path.relpath(path, A))


def hs_style(t):
    # hs_runtime.c uses 'x = y'; my probe edits used 'x= y'. The TU had zero 'x= y' before the edits.
    return re.sub(r'([^ =!<>+*/&|^\-\r\n])= ', r'\1 = ', t)


def rep(t, old, new):
    o = old.replace('\n', NL)
    assert t.count(o) == 1, (old, t.count(o))
    return t.replace(o, new.replace('\n', NL))


_PAD_OLD = '\tboolean initialized;\n\tbyte pad;\n\tshort executing_thread_index;\n'
_PAD_NEW = '\tboolean initialized;\n\tshort executing_thread_index;\n'   # card H-8

hs_l = rep(hs_style(load('copies', 'hs_L2', 'source', 'hs', 'hs_runtime.c')), _PAD_OLD, _PAD_NEW)
save(hs_l, 'hs_runtime', 'source', 'hs', 'hs_runtime.c')

hs_o = rep(hs_style(load('copies', 'hs_H7', 'source', 'hs', 'hs_runtime.c')), _PAD_OLD, _PAD_NEW)
hs_o = rep(hs_o,
           '\tlong result;\n\n\t*(boolean *)&result = csstrlen((char const *)n)==0;\n',
           '\tlong result;\n\n'
           '\t/* BUG: only the low byte of result is written. January returns the other three bytes of its\n'
           '\t   uninitialised stack slot (push ecx / mov [ebp-4],al / mov eax,[ebp-4]); callers read a boolean. */\n'
           '\t*(boolean *)&result = csstrlen((char const *)n)==0;\n')
hs_o = rep(hs_o,
           '\tlong result;\n\n\t*(short *)&result = (short)l;\n',
           '\tlong result;\n\n'
           '\t/* BUG: only the low word of result is written. January homes result in the argument slot and\n'
           '\t   returns the caller\'s upper word unchanged; callers read a short. */\n'
           '\t*(short *)&result = (short)l;\n')
_PACK = ('\t/* BUG: only the low byte of result_long is written; the other three bytes are indeterminate\n'
         '\t   (January stores the boolean into a 4-byte stack slot and passes the whole dword). */\n')
hs_o = rep(hs_o, '\t\t*(boolean *)&result_long = equal;\n',
           '\t' + _PACK.replace('\n\t   ', '\n\t\t   ') + '\t\t*(boolean *)&result_long = equal;\n')
hs_o = rep(hs_o, '\t\t*(boolean *)&result_long = comparison;\n',
           '\t' + _PACK.replace('\n\t   ', '\n\t\t   ') + '\t\t*(boolean *)&result_long = comparison;\n')
hs_o = rep(hs_o, '\t\t*(boolean *)&result_long = *result;\n',
           '\t' + _PACK.replace('\n\t   ', '\n\t\t   ') + '\t\t*(boolean *)&result_long = *result;\n')
save(hs_o, 'hs_runtime_owner', 'source', 'hs', 'hs_runtime.c')

_NAMES = (('_code_0004e8d0', '_error_heap'), ('_code_0004e9a0', '_heap_verify'), ('_code_0004eb50', '_heap_up'),
          ('_code_0004ece0', '_heap_down'), ('_code_0004ef80', '_heap_insert'), ('_code_0004efe0', '_heap_remove'),
          ('_code_0004f2f0', '_path_add_step'), ('_code_0004f510', '_path_new'), ('_code_0004f6f0', '_path_test_pill2d'),
          ('_code_0004f8f0', '_path_add_steps'), ('_code_0004fc20', '_path_iterate'), ('_code_0004fd50', '_path_find'))


def pao_comment(t):   # card PAO-7: comment-only, same line count
    for old, new in _NAMES:
        t = rep(t, '\t%s (0000)\n' % old, '\t%s (0000)\n' % new)
    return t


save(pao_comment(load('copies', 'pao5', 'source', 'ai', 'path_obstacle_avoidance.c')), 'pao', 'source', 'ai', 'path_obstacle_avoidance.c')
save(pao_comment(load('copies', 'pao6', 'source', 'ai', 'path_obstacle_avoidance.c')), 'pao_opt', 'source', 'ai', 'path_obstacle_avoidance.c')
