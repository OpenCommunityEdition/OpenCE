#include "cseries.h"
#include "models/model_animation_definitions.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "memory/data.h"
#include "engine_animation_selection.h"
#undef memset
#undef memcpy
#undef strcmp
#undef malloc
#undef free
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef char selection_graph_size[sizeof(struct animation_graph)==0x80?1:-1];
typedef char selection_animations_offset[offsetof(struct animation_graph,animations)==0x74?1:-1];
typedef char selection_animation_size[sizeof(struct animation)==0xb4?1:-1];

struct selection_state {
    struct animation_graph graph;
    struct animation *animations;
    const struct mac_retail_animation_graph *decoded;
    unsigned long checksum;
    unsigned node_count;
    unsigned long diagnostic_definition_handle;
    struct unit_definition diagnostic_definition;
};
static struct selection_state *active_selection;
/* Real native tag service for this isolated registry, never a fabricated
   return for an unknown tag/group. Consolidate with the full native registry
   before linking engine cache_files.c. */
void *tag_get(long group,long handle)
{
    if(active_selection && group==UNIT_DEFINITION_TAG &&
        (unsigned long)handle==active_selection->diagnostic_definition_handle)
        return &active_selection->diagnostic_definition;
    if(!active_selection || group!=ANIMATION_GRAPH_TAG ||
        (unsigned long)handle!=active_selection->decoded->handle) {
        fprintf(stderr,"invalid native selection tag lookup\n");abort();
    }
    return &active_selection->graph;
}
long csstrcmp(const char *a,const char *b) { return strcmp(a,b); }
/* ASCII case-insensitive CRT semantics for Halo's tag/animation labels. */
int _stricmp(const char *a,const char *b)
{
    unsigned x,y;
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A' && x<='Z')x+='a'-'A';
        if(y>='A' && y<='Z')y+='a'-'A';
        if(x!=y)return x<y?-1:1;
    } while(x);
    return 0;
}
static int native_indices(const struct mac_animation_indices *source,struct tag_block *target)
{
    struct animation_graph_animation_index *indices;
    unsigned i;
    if(source->count>256 || (source->count && !source->indices))return 0;
    if(!source->count)return 1;
    indices=(struct animation_graph_animation_index *)calloc(source->count,sizeof(*indices));
    if(!indices)return 0;
    target->address=indices;target->count=source->count;
    for(i=0;i<source->count;++i)indices[i].animation_index=source->indices[i];
    return 1;
}
static int native_seats(struct selection_state *state)
{
    unsigned i,j,k;
    struct animation_graph_unit_seat *seats;
    if(state->decoded->seat_count>64 || (state->decoded->seat_count && !state->decoded->seats))return 0;
    if(!state->decoded->seat_count)return 1;
    seats=(struct animation_graph_unit_seat *)calloc(state->decoded->seat_count,sizeof(*seats));
    if(!seats)return 0;
    state->graph.unit_seats.count=state->decoded->seat_count;state->graph.unit_seats.address=seats;
    for(i=0;i<state->decoded->seat_count;++i) {
        const struct mac_animation_seat *source=&state->decoded->seats[i];
        struct animation_graph_weapon_class *classes;
        memcpy(seats[i].label,source->label,32);
        if(!native_indices(&source->animations,&seats[i].animations))return 0;
        if(source->class_count>64 || (source->class_count && !source->classes))return 0;
        if(!source->class_count)continue;
        classes=(struct animation_graph_weapon_class *)calloc(source->class_count,sizeof(*classes));
        if(!classes)return 0;
        seats[i].weapon_classes.count=source->class_count;seats[i].weapon_classes.address=classes;
        for(j=0;j<source->class_count;++j) {
            const struct mac_animation_class *sc=&source->classes[j];
            struct animation_graph_weapon_type *types;
            memcpy(classes[j].label,sc->label,32);
            if(!native_indices(&sc->animations,&classes[j].animations))return 0;
            if(sc->type_count>64 || (sc->type_count && !sc->types))return 0;
            if(!sc->type_count)continue;
            types=(struct animation_graph_weapon_type *)calloc(sc->type_count,sizeof(*types));
            if(!types)return 0;
            classes[j].weapon_types.count=sc->type_count;classes[j].weapon_types.address=types;
            for(k=0;k<sc->type_count;++k) {
                memcpy(types[k].label,sc->types[k].label,32);
                if(!native_indices(&sc->types[k].animations,&types[k].animations))return 0;
            }
        }
    }
    return 1;
}
int mac_engine_animation_selection_create(const struct mac_retail_animation_graph *decoded,
    const struct mac_retail_skeleton *skeleton,struct mac_engine_animation_selection *out,const char **error)
{
    struct selection_state *state;
    unsigned i;
    if(error)*error=NULL;
    if(out)out->state=NULL;
    if(!out || !decoded || !skeleton || decoded->animation_count>MAXIMUM_ANIMATIONS_PER_GRAPH ||
        !decoded->animation_count || !decoded->animations || decoded->node_count!=skeleton->count ||
        !skeleton->count || skeleton->count>MAXIMUM_NODES_PER_ANIMATION) {
        if(error)*error="invalid native selection graph/model";return 0;
    }
    state=(struct selection_state *)calloc(1,sizeof(*state));
    if(!state) { if(error)*error="selection allocation failed";return 0; }
    state->animations=(struct animation *)calloc(decoded->animation_count,sizeof(struct animation));
    if(!state->animations) { free(state);if(error)*error="selection allocation failed";return 0; }
    state->decoded=decoded;state->checksum=skeleton->checksum;state->node_count=skeleton->count;
    state->graph.animations.count=decoded->animation_count;
    state->graph.animations.address=state->animations;
    for(i=0;i<decoded->animation_count;++i) {
        const struct mac_retail_animation *source=&decoded->animations[i];
        struct animation *target=&state->animations[i];
        /* Only name and compatibility metadata are consumed by these APIs.
           Never use this selection view to sample a pose or drive a unit. */
        memcpy(target->name,source->name,sizeof(target->name));
        target->type=source->type;target->frame_count=source->frame_count;
        target->node_count=source->node_count;target->node_list_checksum=source->node_list_checksum;
    }
    out->state=state;
    if(!native_seats(state)) {
        mac_engine_animation_selection_free(out);
        if(error)*error="native selection descriptors are invalid or allocation failed";
        return 0;
    }
    return 1;
}
void mac_engine_animation_selection_free(struct mac_engine_animation_selection *view)
{
    struct selection_state *state;
    if(!view || !view->state)return;
    state=(struct selection_state *)view->state;
    {
        unsigned i,j,k;
        struct animation_graph_unit_seat *seats=(struct animation_graph_unit_seat *)state->graph.unit_seats.address;
        for(i=0;i<(unsigned)state->graph.unit_seats.count;++i) {
            struct animation_graph_weapon_class *classes=(struct animation_graph_weapon_class *)seats[i].weapon_classes.address;
            for(j=0;j<(unsigned)seats[i].weapon_classes.count;++j) {
                struct animation_graph_weapon_type *types=(struct animation_graph_weapon_type *)classes[j].weapon_types.address;
                for(k=0;k<(unsigned)classes[j].weapon_types.count;++k)free(types[k].animations.address);
                free(types);free(classes[j].animations.address);
            }
            free(classes);free(seats[i].animations.address);
        }
        free(seats);
    }
    free(state->animations);free(state);view->state=NULL;
}
int mac_engine_animation_selection_find(const struct mac_engine_animation_selection *view,const char *name)
{
    struct selection_state *state;
    struct animation *animation;
    short index;
    if(!view || !view->state || !name || active_selection)return -1;
    state=(struct selection_state *)view->state;
    active_selection=state;
    index=animation_graph_get_animation_by_name((long)state->decoded->handle,name);
    active_selection=NULL;
    if(index<0)return -1;
    animation=unit_animation_get_fitting(&state->graph,index,(short)state->node_count);
    if(!animation)return -1;
    if(animation->frame_count<1 || animation->node_count!=(int)state->node_count ||
        (animation->node_list_checksum && state->checksum && animation->node_list_checksum!=state->checksum))return -1;
    return index;
}
int mac_engine_animation_selection_slot(const struct mac_engine_animation_selection *view,
    const char *seat_label,const char *class_label,int slot)
{
    struct selection_state *state;
    unsigned i,j;
    if(!view || !view->state || !seat_label || !class_label || slot<0)return -1;
    state=(struct selection_state *)view->state;
    for(i=0;i<state->decoded->seat_count;++i) {
        const struct mac_animation_seat *seat=&state->decoded->seats[i];
        if(_stricmp(seat->label,seat_label))continue;
        for(j=0;j<seat->class_count;++j) {
            const struct mac_animation_class *weapon_class=&seat->classes[j];
            int index,found;
            if(_stricmp(weapon_class->label,class_label))continue;
            if((unsigned)slot>=weapon_class->animations.count)return -1;
            index=weapon_class->animations.indices[slot];
            if(index<0 || (unsigned)index>=state->decoded->animation_count)return -1;
            found=mac_engine_animation_selection_find(view,state->decoded->animations[index].name);
            return found==index?found:-1;
        }
    }
    return -1;
}
int mac_engine_animation_selection_idle_slot(void) { return _unit_weapon_class_animation_idle; }
int mac_engine_animation_selection_move_front_slot(void) { return _unit_weapon_class_animation_moving_front; }

int mac_engine_animation_diagnostic_seat(const struct mac_engine_animation_selection *view,
    uint32_t biped_handle,const char *seat,struct mac_engine_seat_result *out)
{
    struct selection_state *state;
    struct unit_datum unit;
    struct object_header_datum *header;
    long index,size;
    int result;
    if(!view || !view->state || !seat || !out || active_selection || object_header_data)return -1;
    state=(struct selection_state *)view->state;
    memset(&unit,0,sizeof(unit));
    unit.definition_index=biped_handle;unit.object.type=_object_type_biped;
    unit.unit.animation.seat_index=-1;unit.unit.animation.base_seat_index=-1;
    unit.unit.animation.weapon_index=-1;unit.unit.animation.weapon_type_index=-1;
    unit.unit.animation.state=-1;
    size=data_allocation_size(1,sizeof(struct object_header_datum));
    object_header_data=(struct data_array *)malloc((size_t)size);
    if(!object_header_data)return -1;
    data_initialize(object_header_data,"diagnostic object",1,sizeof(struct object_header_datum));
    data_make_valid(object_header_data);index=datum_new(object_header_data);
    if(index==NONE) { free(object_header_data);object_header_data=NULL;return -1; }
    header=object_header_get(index);
    header->type=_object_type_biped;header->data_size=sizeof(unit);
    header->datum=(struct object_datum *)&unit;
    /* Partial definition fixture: only the animation reference consumed by
       unit_set_seat is populated. Never pass this to unit_new/object_new. */
    memset(&state->diagnostic_definition,0,sizeof(state->diagnostic_definition));
    state->diagnostic_definition_handle=biped_handle;
    state->diagnostic_definition.object.animation_graph.index=state->decoded->handle;
    state->diagnostic_definition.object.model.index=NONE;
    active_selection=state;
    result=unit_set_seat(index,seat);
    active_selection=NULL;
    out->seat=unit.unit.animation.seat_index;
    out->base_seat=unit.unit.animation.base_seat_index;
    out->weapon_class=unit.unit.animation.weapon_index;
    out->weapon_type=unit.unit.animation.weapon_type_index;
    out->flags=unit.unit.animation.flags;
    datum_delete(object_header_data,index);
    free(object_header_data);object_header_data=NULL;
    return result;
}

int mac_engine_animation_selection_fits(const struct mac_engine_animation_selection *view,int index,int model_nodes)
{
    struct selection_state *state;
    if(!view || !view->state || index < -32768 || index > 32767 || model_nodes<0 || model_nodes>64)return 0;
    state=(struct selection_state *)view->state;
    return unit_animation_get_fitting(&state->graph,(short)index,(short)model_nodes)!=NULL;
}
