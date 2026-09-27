"""K2: generate edits E4..E12 (appended to edits.py). <T> = tab."""
def t(s):
    return s.replace('<T>', '\t')

E = {}
E['E4'] = [(t("""<T><T><T><T><T>{
<T><T><T><T><T><T>real_argb_color const *color;

<T><T><T><T><T><T>if (prop->ignore)
<T><T><T><T><T><T>{
<T><T><T><T><T><T><T>color = global_real_argb_blue;
<T><T><T><T><T><T>}
<T><T><T><T><T><T>else
<T><T><T><T><T><T>{
<T><T><T><T><T><T><T>color = prop->preferred_target ? global_real_argb_pink : global_real_argb_red;
<T><T><T><T><T><T>}

<T><T><T><T><T><T>render_debug_string_at_point(TRUE, &string_point, csprintf(temporary, "%.2f", prop->target_weight), color);"""),
t("""<T><T><T><T><T>{
<T><T><T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T><T><T>TRUE,
<T><T><T><T><T><T><T>&string_point,
<T><T><T><T><T><T><T>csprintf(temporary, "%.2f", prop->target_weight),
<T><T><T><T><T><T><T>prop->ignore ? global_real_argb_blue : (prop->preferred_target ? global_real_argb_pink : global_real_argb_red));"""))]

E['E5'] = [(t("""<T><T><T><T><T><T>long time = prop->last_unreachable_time!=NONE ? game_time_get()-prop->last_unreachable_time : NONE;

<T><T><T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T><T><T>TRUE,
<T><T><T><T><T><T><T>&string_point,
<T><T><T><T><T><T><T>csprintf(temporary, "unr %d %d", prop->unreachable_ticks, time),"""),
t("""<T><T><T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T><T><T>TRUE,
<T><T><T><T><T><T><T>&string_point,
<T><T><T><T><T><T><T>csprintf(
<T><T><T><T><T><T><T><T>temporary,
<T><T><T><T><T><T><T><T>"unr %d %d",
<T><T><T><T><T><T><T><T>prop->unreachable_ticks,
<T><T><T><T><T><T><T><T>prop->last_unreachable_time==NONE ? NONE : game_time_get()-prop->last_unreachable_time),"""))]

E['E6'] = [(t("""<T><T><T><T>long time = prop->last_unreachable_time!=NONE ? game_time_get()-prop->last_unreachable_time : NONE;

<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(temporary, "unr %d %d", prop->unreachable_ticks, time),"""),
t("""<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(
<T><T><T><T><T><T>temporary,
<T><T><T><T><T><T>"unr %d %d",
<T><T><T><T><T><T>prop->unreachable_ticks,
<T><T><T><T><T><T>prop->last_unreachable_time==NONE ? NONE : game_time_get()-prop->last_unreachable_time),"""))]

E['E7'] = [(t("""<T><T><T><T><T><T>char const *string = actor->state.action_data.guard.cower_panicked ? "panic" : "hide";
<T><T><T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T><T><T>TRUE,
<T><T><T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T><T><T>csprintf(temporary, "%s %d", string, actor->state.action_data.guard.cower_ticks),"""),
t("""<T><T><T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T><T><T>TRUE,
<T><T><T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T><T><T>csprintf(
<T><T><T><T><T><T><T><T>temporary,
<T><T><T><T><T><T><T><T>"%s %d",
<T><T><T><T><T><T><T><T>actor->state.action_data.guard.cower_panicked ? "panic" : "hide",
<T><T><T><T><T><T><T><T>actor->state.action_data.guard.cower_ticks),"""))]

E['E8'] = [(t("""<T><T><T><T>char const *string = actor_move_animation_busy(actor_index) ? "busy " : "";
<T><T><T><T>
<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(
<T><T><T><T><T><T>temporary,
<T><T><T><T><T><T>"%srof %.1f err %.1f dmg %.1f blk %d",
<T><T><T><T><T><T>string,"""),
t("""<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(
<T><T><T><T><T><T>temporary,
<T><T><T><T><T><T>"%srof %.1f err %.1f dmg %.1f blk %d",
<T><T><T><T><T><T>actor_move_animation_busy(actor_index) ? "busy " : "","""))]

E['E9'] = [(t("""<T><T><T><T>real damage = unit==NULL ? 0.f : unit->object.current_body_damage;
<T><T><T><T>char const *string = actor->meta.unit_index==NONE || !unit_is_busy(actor->meta.unit_index) ? "not-" : "";

<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(temporary, "trying: %sbusy dmg %.1f", string, damage),"""),
t("""<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>ai_debug_drawstack(),
<T><T><T><T><T>csprintf(
<T><T><T><T><T><T>temporary,
<T><T><T><T><T><T>"trying: %sbusy dmg %.1f",
<T><T><T><T><T><T>actor->meta.unit_index!=NONE && unit_is_busy(actor->meta.unit_index) ? "" : "not-",
<T><T><T><T><T><T>unit==NULL ? 0.f : unit->object.current_body_damage),"""))]

old10 = t("""<T><T><T><T>char const *string = actor->danger_zone.projectile.time_until_explosion==NONE ?
<T><T><T><T><T>"NONE" :
<T><T><T><T><T>csprintf(temporary, "%d", actor->danger_zone.projectile.time_until_explosion);

<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>&actor->danger_zone.position,
<T><T><T><T><T>string,
<T><T><T><T><T>color);""")
new10 = t("""<T><T><T><T>render_debug_string_at_point(
<T><T><T><T><T>TRUE,
<T><T><T><T><T>&actor->danger_zone.position,
<T><T><T><T><T>actor->danger_zone.projectile.time_until_explosion==NONE ?
<T><T><T><T><T><T>"NONE" :
<T><T><T><T><T><T>csprintf(temporary, "%d", actor->danger_zone.projectile.time_until_explosion),
<T><T><T><T><T>color);""")
E['E10'] = [('@@TWICE@@' + old10, new10)]

E['E11'] = [(t("""<T><T><T>real_argb_color const *color;

<T><T><T>if (actor->danger_zone.currently_perceived)
<T><T><T>{
<T><T><T><T>if (actor->danger_zone.acknowledgement_timer>0)
<T><T><T><T>{
<T><T><T><T><T>color = global_real_argb_white;
<T><T><T><T>}
<T><T><T><T>else
<T><T><T><T>{
<T><T><T><T><T>if (actor->danger_zone.noticed_danger)
<T><T><T><T><T>{
<T><T><T><T><T><T>color = global_real_argb_yellow;
<T><T><T><T><T>}
<T><T><T><T><T>else
<T><T><T><T><T>{
<T><T><T><T><T><T>color = global_real_argb_blue;
<T><T><T><T><T>}
<T><T><T><T>}
<T><T><T>}
<T><T><T>else
<T><T><T>{
<T><T><T><T>color = global_real_argb_darkgreen;
<T><T><T>}"""),
t("""<T><T><T>real_argb_color const *color = actor->danger_zone.currently_perceived ?
<T><T><T><T>(actor->danger_zone.acknowledgement_timer>0 ? global_real_argb_white : (actor->danger_zone.noticed_danger ? global_real_argb_yellow : global_real_argb_blue)) :
<T><T><T><T>global_real_argb_darkgreen;"""))]

E['E12'] = [(t("""<T><T><T><T>real_argb_color const *color;
<T><T><T><T>real_point3d offset_point;

<T><T><T><T>if (actor->control.path.at_destination)
<T><T><T><T>{
<T><T><T><T><T>color = global_real_argb_yellow;
<T><T><T><T>}
<T><T><T><T>else
<T><T><T><T>{
<T><T><T><T><T>color = actor->control.path.path.steps_finish_path ? global_real_argb_pink : global_real_argb_purple;
<T><T><T><T>}
"""),
t("""<T><T><T><T>real_argb_color const *color = actor->control.path.at_destination ? global_real_argb_yellow : (actor->control.path.path.steps_finish_path ? global_real_argb_pink : global_real_argb_purple);
<T><T><T><T>real_point3d offset_point;
"""))]

if __name__ == '__main__':
    import os
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'edits.py')
    with open(p, 'a', encoding='latin-1') as f:
        f.write('\n# E4..E12: /Od-attested conditional spellings (cards C4..C12); generated by gen_edits_e4_e12.py.\n')
        for k, v in E.items():
            f.write('EDITS[%r] = %r\n' % (k, v))
    print('appended', list(E))
