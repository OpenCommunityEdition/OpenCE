"""Replace the animation-impulse else-if block with the /Od boolean-local form. usage: edit_impulse.py IN OUT"""
import sys
src, out = sys.argv[1], sys.argv[2]
t = open(src, newline='').read()
crlf = '\r\n' in t
t = t.replace('\r\n', '\n')
start = t.index('\t\t\t\t\t\t\telse if (animation_type != NONE &&\n')
end_marker = '\t\t\t\t\t\t\t\telse\n\t\t\t\t\t\t\t\t{\n\t\t\t\t\t\t\t\t\tanimation_weight = 2.0f;\n\t\t\t\t\t\t\t\t}\n\t\t\t\t\t\t\t}\n'
end = t.index(end_marker, start) + len(end_marker)
old = t[start:end]
assert old.count('animation_weight = 2.0f;') == 2
B = chr(92) * 2
fname = '"c:' + B + 'halo' + B + 'SOURCE' + B + 'ai' + B + 'ai_communication.c"'
assert fname in old, fname
T = '\t'
new = (T*7 + 'else if (animation_type != NONE)\n' +
       T*7 + '{\n' +
       T*8 + 'boolean animation_impulse = unit_test_animation_impulse(\n' +
       T*9 + 'protagonist_unit_index,\n' +
       T*9 + 'animation_type);\n\n' +
       T*8 + 'if (animation_impulse && protagonist_actor_index != NONE)\n' +
       T*8 + '{\n' +
       T*9 + 'match_assert(\n' +
       T*10 + fname + ',\n' +
       T*10 + '0x6F2,\n' +
       T*10 + 'protagonist_actor);\n' +
       T*9 + 'if (actor_action_class(protagonist_actor_index) == _action_class_transitory)\n' +
       T*9 + '{\n' + T*10 + 'animation_impulse = FALSE;\n' + T*9 + '}\n' +
       T*9 + 'else if (protagonist_actor->state.mode == _actor_mode_asleep)\n' +
       T*9 + '{\n' + T*10 + 'animation_impulse = FALSE;\n' + T*9 + '}\n' +
       T*8 + '}\n\n' +
       T*8 + 'if (animation_impulse)\n' +
       T*8 + '{\n' + T*9 + 'animation_weight = 2.0f;\n' + T*8 + '}\n' +
       T*8 + 'else\n' +
       T*8 + '{\n' + T*9 + 'animation_type = NONE;\n' + T*8 + '}\n' +
       T*7 + '}\n')
t = t[:start] + new + t[end:]
if crlf:
    t = t.replace('\n', '\r\n')
open(out, 'w', newline='').write(t)
print('wrote', out)
