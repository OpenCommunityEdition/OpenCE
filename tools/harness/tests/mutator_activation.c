#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mutator_hs.h"
static long state[32], reals[32], buttons[3], time_now, down, hit, models=1, rope_missing;
#define HEAD ((long)0xa3450003)
#define ROPE ((long)0xb4560004)
static int live[2], deleted, placed[2], endpoints[2], fallback;
static float pose[2][3], node_pose[2][3];
static float as_real(long v) { float f; memcpy(&f,&v,4); return f; }
static int model_slot(long v) { return v==HEAD?0:v==ROPE?1:-1; }
static float result[3];
static long real_bits(float f) { long v; memcpy(&v,&f,4); return v; }
#define UNIT ((long)0x81230001)
#define TARGET ((long)0x92340002)
static int host(const char *name, const long *a, long *v)
{
    *v = 0;
    if (!strcmp(name,"math_sqrt")) { float f; memcpy(&f,a,4); *v=real_bits(sqrtf(f)); }
    else if (!strcmp(name,"math_divide")) { float x,y; memcpy(&x,a,4); memcpy(&y,a+1,4); *v=real_bits(y?x/y:0); }
    else if (!strcmp(name,"math_equals_string")) *v=!strcmp((char*)a[0],(char*)a[1]);
    else if (!strcmp(name,"object_acceleration_scale")) *v=real_bits(1);
    else if (!strcmp(name,"model_create")) {
        int rope=!strcmp((char*)a[0],"altis\\weapons\\grappling_hook\\grappling_hook");
        *v=models?(rope?ROPE:HEAD):-1;
        if (models) live[rope]=1;
    }
    else if (!strcmp(name,"model_exists")) { int n=model_slot(a[0]); *v=n>=0 && live[n] && models && !(n==1 && rope_missing); }
    else if (!strcmp(name,"model_delete")) { int n=model_slot(a[0]); if(n>=0 && live[n]) { live[n]=0; deleted++; } }
    else if (!strcmp(name,"model_place")) {
        int n=model_slot(a[0]); if(n<0 || !live[n]) return 0;
        placed[n]++; for(int i=0;i<3;i++) pose[n][i]=as_real(a[i+1]);
    }
    else if (!strcmp(name,"model_node") || !strcmp(name,"model_node_from_view")) {
        if(a[0]!=ROPE || a[1]<0 || a[1]>1 || !live[1]) return 0;
        endpoints[a[1]]++; for(int i=0;i<3;i++) node_pose[a[1]][i]=as_real(a[i+2]);
    }
    else if (!strcmp(name,"draw_line") || !strcmp(name,"draw_line_from_view")) fallback++;
    else if (!strcmp(name,"state_get_long")) *v=state[a[1]];
    else if (!strcmp(name,"state_set_long")) state[a[1]]=a[2];
    else if (!strcmp(name,"state_get_real")) *v=reals[a[1]];
    else if (!strcmp(name,"state_set_real")) reals[a[1]]=a[2];
    else if (!strcmp(name,"result_x")) *v=real_bits(result[0]);
    else if (!strcmp(name,"result_y")) *v=real_bits(result[1]);
    else if (!strcmp(name,"result_z")) *v=real_bits(result[2]);
    else if (!strcmp(name,"result_object")) *v=TARGET;
    else if (!strcmp(name,"world_ray")) { result[0]=10; result[1]=0; result[2]=1; *v=hit; }
    else if (!strcmp(name,"object_root")) *v=a[0];
    else if (!strcmp(name,"object_kind")) *v=(long)"biped";
    else if (!strcmp(name,"object_tag")) *v=(long)"test";
    else if (!strcmp(name,"game_host") || !strcmp(name,"object_alive")) *v=1;
    else if (!strcmp(name,"game_time")) *v=time_now;
    else if (!strcmp(name,"settings_string")) *v=(long)(!strcmp((char*)a[0],"keyboard")?"Left Shift":"DPad Down");
    else if (!strcmp(name,"input_held")) *v=down && !strcmp((char*)a[0],"Left Shift");
    else if (!strcmp(name,"input_button")) buttons[a[0]]=a[1];
    else if (!strcmp(name,"player_button")) *v=buttons[a[1]];
    else if (!strcmp(name,"player_unit")) *v=UNIT;
    else if (!strcmp(name,"object_parent")) *v=-1;
    else if (!strcmp(name,"player_aim") || !strcmp(name,"object_position") || !strcmp(name,"object_velocity")) {
        result[0]=!strcmp(name,"player_aim")?1:(a[0]==TARGET?10:0);
        result[1]=0; result[2]=0; *v=1;
    }
    else if (!strcmp(name,"settings_real")) {
        const char *key=(char*)a[0];
        *v=real_bits(!strcmp(key,"range")?150:!strcmp(key,"projectile_velocity")?2:
            !strcmp(key,"min_distance")?0.5:!strcmp(key,"speed")?0.2:1);
    }
    return 1;
}
#include "activation_api.h"
static int call(struct mutator_hs_program *p,const char *name)
{
    struct mutator_hs_result r;
    int ok=mutator_hs_program_call(p,name,32768,&r);
    fprintf(stderr,"%s ok=%d steps=%ld active=%ld armed=%ld button=%ld %s\n",name,ok,r.steps,state[0],state[2],buttons[0],r.error);
    return ok;
}
int __wrap_main(int argc, char **argv)
{
    char text[32768]; long size;
    struct mutator_hs_result r;
    struct mutator_hs_program *p;
    FILE *f=fopen(argv[1],"rb");
    if (!f) return 2;
    size=fread(text,1,sizeof(text),f); fclose(f);
    p=mutator_hs_program_compile(text,size,api,sizeof(api)/sizeof(api[0]),NULL,&r);
    if (!p) { fprintf(stderr,"compile %s\n",r.error); return 3; }
    if (!call(p,"startup") || !call(p,"frame") || !call(p,"tick") || !call(p,"player_tick")) return 4;
    if (state[2]!=1) return 5;
    time_now++; down=1;
    if (!call(p,"frame") || buttons[0]!=1 || !call(p,"tick") || !call(p,"player_tick")) return 6;
    if (state[0]!=1 || state[2]!=0) return 7;
    if (!call(p,"player_frame")) return 8;
    if(state[6]!=HEAD || state[7]!=ROPE || !placed[0] || !placed[1] ||
       !endpoints[0] || !endpoints[1] || fallback) return 17;
    for(int i=0;i<3;i++) if(fabsf(pose[0][i]-as_real(reals[i]))>0.0001f ||
        fabsf(node_pose[1][i]-pose[0][i])>0.0001f) return 18;
    /* A missing rope must not strand the valid head at its previous endpoint. */
    { int before=placed[0];
      rope_missing=1; reals[0]=real_bits(as_real(reals[0])+1.0f);
      if (!call(p,"player_frame") || placed[0]!=before+1 || !fallback ||
          fabsf(pose[0][0]-as_real(reals[0]))>0.0001f) return 20;
      rope_missing=0;
    }
    models=0;
    if (!call(p,"player_frame") || !fallback) return 13;
    models=1;
    time_now++;
    if (!call(p,"tick") || !call(p,"player_tick")) return 9;
    hit=1; time_now++;
    if (!call(p,"tick") || !call(p,"player_tick") || state[1]!=1 || state[4]!=TARGET) return 10;
    buttons[1]=1;
    if (!call(p,"player_tick") || !call(p,"player_frame")) return 11;
    buttons[0]=0;
    if (!call(p,"player_tick") || state[0]!=0 || !call(p,"player_frame") || !call(p,"player_stop") || !call(p,"stop")) return 12;
    if(live[0] || live[1] || deleted!=2 || state[6]!=-1 || state[7]!=-1) return 19;
    mutator_hs_program_free(p);
    {
        /* A boolean argument fills only the low byte of its 32-bit slot.
         * Evaluate a large long first so the argument stack holds high
         * bytes, then pass false/true: the host must see exactly 0 and 1. */
        const char flags[]="(script static long dirty (state_get_long 0 31)) "
            "(script static void off (begin (dirty) (input_button 0 false))) "
            "(script static void on (begin (dirty) (input_button 0 true)))";
        state[31]=(long)0xdeadbe00; buttons[0]=7;
        p=mutator_hs_program_compile(flags,sizeof(flags)-1,api,sizeof(api)/sizeof(api[0]),NULL,&r);
        if (!p) { fprintf(stderr,"compile flags %s\n",r.error); return 21; }
        if (!mutator_hs_program_call(p,"off",32768,&r) || buttons[0]!=0) {
            fprintf(stderr,"false reached the host as %ld (%s)\n",buttons[0],r.error); return 23; }
        if (!mutator_hs_program_call(p,"on",32768,&r) || buttons[0]!=1) {
            fprintf(stderr,"true reached the host as %ld (%s)\n",buttons[0],r.error); return 24; }
        if (!mutator_hs_program_call(p,"off",32768,&r) || buttons[0]!=0) return 25;
        mutator_hs_program_free(p);
    }
    {
        const char overflow[]="(script static void recurse (recurse)) (script static void tick (recurse)) (script static long healthy (+ 20 22))";
        p=mutator_hs_program_compile(overflow,sizeof(overflow)-1,api,sizeof(api)/sizeof(api[0]),NULL,&r);
        if (!p) return 14;
        if (mutator_hs_program_call(p,"tick",32768,&r) || !strstr(r.error,"callback tick:") || !strstr(r.error,"stack overflow")) {
            fprintf(stderr,"expected isolated overflow: %s\n",r.error); return 15;
        }
        if (!mutator_hs_program_call(p,"healthy",32768,&r) || r.value!=42) return 16;
        mutator_hs_program_free(p);
    }
    return 0;
}
