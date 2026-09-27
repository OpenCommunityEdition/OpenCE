"""Revert HELD forms in a K1 lab copy. usage: revert_held.py IN OUT letters
  f = protagonist_invalid FALSE-init flag -> TRUE-init "valid" (same block/continue structure)
  l = uninitialised recipient_look_data aggregate -> "long look_unit_index = NONE" + canonical struct field
  p = uninitialised play_type -> "short play_type = _unit_play_speech_none;" (same declaration position)
  n = header normalize2d at the alignment site -> canonical hand-written magnitude/epsilon/divide
  v = vector_from_points2d((real_point2d const *)...) -> canonical hand-written subtraction"""
import sys, re
src, out, letters = sys.argv[1], sys.argv[2], sys.argv[3]
N = '\r\n'
t = open(src, newline='').read()
def rep(old, new, count=1):
    global t
    assert t.count(old) == count, (t.count(old), old[:80])
    t = t.replace(old, new)
if 'f' in letters:
    rep('boolean protagonist_invalid = FALSE;', 'boolean valid = TRUE;')
    n = t.count('protagonist_invalid = TRUE;')
    assert n == 6, n
    t = t.replace('protagonist_invalid = TRUE;', 'valid = FALSE;')
    rep('if (protagonist_invalid)' + N, 'if (!valid)' + N)
    assert 'protagonist_invalid' not in t
if 'l' in letters:
    rep('\tword pad2A;' + N + '\tstruct ai_information_look_data look_data;' + N, '\tword pad2A;' + N + '\tlong look_unit_index;' + N)
    rep('\t\t\t\tstruct ai_information_look_data recipient_look_data;' + N, '\t\t\t\tlong look_unit_index = NONE;' + N)
    t = re.sub(r'recipient_look_data\.(unit\.unit_index|object\.object_index) = ', 'look_unit_index = ', t)
    rep('possibilities[possibility_count].look_data = recipient_look_data;', 'possibilities[possibility_count].look_unit_index = look_unit_index;')
    rep('information.look_data = selected_possibility->look_data;', 'information.look_data.unit.unit_index = selected_possibility->look_unit_index;')
    assert 'recipient_look_data' not in t
if 'p' in letters:
    rep('\t\t\t\t\t\tshort play_type;' + N, '\t\t\t\t\t\tshort play_type = _unit_play_speech_none;' + N)
if 'n' in letters:
    rep('\t\t\t\t\treal_point3d target_head;' + N + N, '\t\t\t\t\treal_point3d target_head;' + N + '\t\t\t\t\treal magnitude;' + N + N)
    rep('\t\t\t\t\tif (normalize2d(&alignment) == 0.0f)' + N + '\t\t\t\t\t{' + N,
        '\t\t\t\t\tmagnitude = square_root(' + N + '\t\t\t\t\t\talignment.i * alignment.i + alignment.j * alignment.j);' + N +
        '\t\t\t\t\tif (fabs(magnitude) >= _real_epsilon && magnitude != 0.0f)' + N + '\t\t\t\t\t{' + N +
        '\t\t\t\t\t\talignment.i /= magnitude;' + N + '\t\t\t\t\t\talignment.j /= magnitude;' + N + '\t\t\t\t\t}' + N +
        '\t\t\t\t\telse' + N + '\t\t\t\t\t{' + N)
if 'v' in letters:
    rep('\t\t\t\t\tvector_from_points2d(' + N + '\t\t\t\t\t\t(real_point2d const *)&speaker_head,' + N +
        '\t\t\t\t\t\t(real_point2d const *)&target_head,' + N + '\t\t\t\t\t\t&alignment);' + N,
        '\t\t\t\t\talignment.i = target_head.x - speaker_head.x;' + N + '\t\t\t\t\talignment.j = target_head.y - speaker_head.y;' + N)
open(out, 'w', newline='').write(t)
print('wrote', out)
