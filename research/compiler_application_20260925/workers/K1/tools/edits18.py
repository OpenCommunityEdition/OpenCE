"""K1-18 code-neutral /Od statement forms. usage: edits18.py IN OUT letters (subset of abcde)"""
import sys
src, out, letters = sys.argv[1], sys.argv[2], sys.argv[3]
N = '\r\n'
t = open(src, newline='').read()
def rep(old, new):
    global t
    assert t.count(old) == 1, (t.count(old), old[:70])
    t = t.replace(old, new)
if 'a' in letters:
    rep('\t\t\tcsstrcat(debug_string, "DISABLED");' + N + '\t\t\terror(2, debug_string);' + N,
        '\t\t\tcsstrcat(debug_string, "DISABLED");' + N + '\t\t\tif (!suppress_output)' + N + '\t\t\t{' + N +
        '\t\t\t\terror(2, debug_string);' + N + '\t\t\t}' + N)
if 'b' in letters:
    rep('\t\t\t\t\t\t"still holds",' + N, '\t\t\t\t\t\tbroken ? "broken" : "still holds",' + N)
if 'c' in letters:
    rep('\t\t\treal original_weight = total_possibility_weight;' + N,
        '\t\t\treal original_weight = total_possibility_weight;' + N +
        '\t\t\tshort original_count = possibility_count;' + N)
    rep('\t\t\t\t\t\t"[%d/%.1f force %d/%.1f] ",' + N + '\t\t\t\t\t\tpossibility_count,' + N,
        '\t\t\t\t\t\t"[%d/%.1f force %d/%.1f] ",' + N + '\t\t\t\t\t\toriginal_count,' + N)
if 'd' in letters:
    rep('\t\t\t\t\t\t\t\ttotal_possibility_weight += weight;' + N + '\t\t\t\t\t\t\t\tany_protagonist_considered = TRUE;' + N,
        '\t\t\t\t\t\t\t\tany_protagonist_considered = TRUE;' + N + '\t\t\t\t\t\t\t\ttotal_possibility_weight += weight;' + N)
if 'e' in letters:
    rep('\t\tinformation.target_unit_index = selected_possibility->recipient_unit_index;' + N +
        '\t\tinformation.damage_category = damage_type;' + N,
        '\t\tinformation.damage_category = damage_type;' + N +
        '\t\tinformation.target_unit_index = selected_possibility->recipient_unit_index;' + N)
open(out, 'w', newline='').write(t)
