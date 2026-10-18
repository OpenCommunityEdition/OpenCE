/* Opt-in terrain-camera overlay for the live local unit and held weapon. */
#include "mac_live_object_view.h"
#include "units/units.h"
#include "objects/objects.h"
#include <stdlib.h>
#include <string.h>

int mac_live_object_view_draw(struct mac_runtime_view *view,long unit_index,
    const struct mac_bsp_camera *camera,struct mac_live_object_view_result *out,const char **error)
{
    struct mac_live_object_view_result result;
    struct mac_object_render_probe probe;
    struct mac_runtime_view_frame terrain,overlay;
    struct unit_datum *unit;
    long objects[2],weapon_index;
    const char *local_error=NULL;
    int attached=0,ok=0;

    memset(&result,0,sizeof(result));memset(&probe,0,sizeof(probe));
    if(error)*error=NULL;
    if(out)memset(out,0,sizeof(*out));
    if(!view||!camera||unit_index==NONE) { if(error)*error="live terrain overlay arguments";return 0; }
    unit=unit_get(unit_index);
    if(!unit||unit->object.type!=_object_type_biped) { if(error)*error="live unit is not a biped";return 0; }
    if(unit->unit.current_weapon_index<0||unit->unit.current_weapon_index>=MAXIMUM_WEAPONS_PER_UNIT) {
        if(error)*error="live unit weapon slot is invalid";return 0;
    }
    weapon_index=unit->unit.weapon_object_indices[unit->unit.current_weapon_index];
    if(weapon_index==NONE) { if(error)*error="live unit has no held weapon";return 0; }
    objects[0]=unit_index;objects[1]=weapon_index;
    if(!mac_runtime_view_draw(view,camera,&terrain,&local_error))goto done;
    if(!mac_object_render_probe_attach_many(view,&probe,objects,2)) {
        local_error="attach live terrain objects";goto done;
    }
    attached=1;
    if(!mac_runtime_view_draw(view,camera,&overlay,&local_error))goto done;
    result.terrain_digest=terrain.pixel_digest;result.overlay_digest=overlay.pixel_digest;
    result.biped_parts=probe.stats.drawn_parts;result.biped_vertices=(unsigned)probe.stats.submitted_vertices;
    result.biped_textured_parts=probe.stats.textured_parts;result.biped_unsupported_shaders=probe.stats.unsupported_shaders;
    result.biped_unsupported_semantics=probe.stats.unsupported_shader_semantics;
    result.weapon_parts=probe.additional_stats[0].drawn_parts;
    result.weapon_vertices=(unsigned)probe.additional_stats[0].submitted_vertices;
    result.weapon_textured_parts=probe.additional_stats[0].textured_parts;
    result.weapon_unsupported_shaders=probe.additional_stats[0].unsupported_shaders;
    result.weapon_unsupported_semantics=probe.additional_stats[0].unsupported_shader_semantics;
    if(!mac_object_render_probe_object_valid(&probe,0)||!mac_object_render_probe_object_valid(&probe,1)) {
        local_error="live object geometry overlay rejected a model part";goto done;
    }
    if(terrain.pixel_digest==overlay.pixel_digest) {
        local_error="live model overlay produced no terrain-frame pixel change";goto done;
    }
    {
        const char *capture=getenv("HALO_MAC_TERRAIN_OBJECT_OVERLAY_PPM");
        if(capture&&*capture&&!mac_runtime_view_write_ppm(view,capture)) {
            local_error="save terrain-camera model overlay capture";goto done;
        }
    }
    ok=1;
done:
    if(attached&&!mac_object_render_probe_detach(view,&probe)) {
        ok=0;local_error="detach live terrain object overlay";
    }
    if(out)*out=result;
    if(error)*error=local_error;
    return ok;
}
