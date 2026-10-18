#include "retail_object_spawn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,failures;static uint32_t digest=UINT32_C(2166136261);
static void check(int ok){++checks;if(!ok)++failures;}
static void put(unsigned char*p,uint32_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);}
static void put16(unsigned char*p,unsigned n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void putf(unsigned char*p,float f){uint32_t n;memcpy(&n,&f,4);put(p,n);}
static void word(uint32_t n){digest=(digest^n)*UINT32_C(16777619);}
static void realword(float f){uint32_t n;memcpy(&n,&f,4);word(n);}
static void refword(struct mac_spawn_reference r){word(r.group);word(r.handle);}
static void synthetic(void)
{
    unsigned char bytes[8192],saved[8192];struct mac_retail_cache c;struct mac_retail_object_spawn out;const char*e;unsigned i,j;
    static const uint32_t groups[5]={UINT32_C(0x6d617467),UINT32_C(0x62697064),UINT32_C(0x6d6f6432),UINT32_C(0x77656170),UINT32_C(0x616e7472)};
    static const unsigned roots[5]={256,1000,2400,2800,4300},refs[6]={0x28,0x38,0x70,0x80,0x90,0xa0};
    memset(bytes,0,sizeof(bytes));memset(&c,0,sizeof(c));c.tags=bytes;c.tag_size=sizeof(bytes);c.tag_count=5;c.tags_end=160;
    for(i=0;i<5;++i){put(bytes+i*32,groups[i]);put(bytes+i*32+12,UINT32_C(0x12340000)+i);put(bytes+i*32+16,MAC_RETAIL_BASE+200);put(bytes+i*32+20,MAC_RETAIL_BASE+roots[i]);}
    put(bytes+256+0x164,1);put(bytes+256+0x168,MAC_RETAIL_BASE+700);put(bytes+716,groups[1]);put(bytes+728,UINT32_C(0x12340001));
    for(i=1;i<=3;i+=2)for(j=0;j<6;++j)put(bytes+roots[i]+refs[j]+12,UINT32_MAX);
    put(bytes+1000+0x28,groups[2]);put(bytes+1000+0x34,UINT32_C(0x12340002));put(bytes+1000+0x38,groups[4]);put(bytes+1000+0x44,UINT32_C(0x12340004));
    put(bytes+2400+0xb8,2);putf(bytes+1000+0x424,.7f);putf(bytes+1000+0x428,.5f);putf(bytes+1000+0x42c,.2f);
    put(bytes+1000+0x2d8,1);put(bytes+1000+0x2dc,MAC_RETAIL_BASE+5000);put(bytes+5000,groups[3]);put(bytes+5012,UINT32_C(0x12340003));
    put16(bytes+2800,2);put(bytes+2800+0x468,UINT32_MAX);put(bytes+2800+0x478,UINT32_MAX);put(bytes+2800+0x48c,UINT32_MAX);
    put(bytes+2800+0x4f0,1);put(bytes+2800+0x4f4,MAC_RETAIL_BASE+5100);put(bytes+2800+0x4fc,1);put(bytes+2800+0x500,MAC_RETAIL_BASE+5300);
    put16(bytes+5106,12);put16(bytes+5108,100);put16(bytes+5110,5);put(bytes+5100+68,UINT32_MAX);put(bytes+5100+84,UINT32_MAX);
    put(bytes+5300+104,UINT32_MAX);put(bytes+5300+160,UINT32_MAX);memcpy(saved,bytes,sizeof(bytes));
    check(mac_retail_object_spawn_decode(&c,&out,&e));check(out.weapon_count==1&&out.weapons[0].magazines[0].initial_loaded==5&&out.weapons[0].magazines[0].initial_spare==7);
#define BAD32(offset,n) do{put(bytes+(offset),(n));check(!mac_retail_object_spawn_decode(&c,&out,&e));check(!out.weapon_count&&!out.biped.handle);memcpy(bytes,saved,sizeof(bytes));}while(0)
    BAD32(5012,UINT32_C(0x99990003));BAD32(1000+0x2d8,5);BAD32(1000+0x2dc,MAC_RETAIL_BASE+8190);
    BAD32(2800+0x4f0,3);BAD32(2800+0x4fc,3);BAD32(2800+0x4f4,MAC_RETAIL_BASE+8188);
    BAD32(1000+0x42c,UINT32_C(0x7fc00000));BAD32(2800+4,UINT32_C(0x7f800000));BAD32(5100+20,UINT32_C(0x7f800000));
    BAD32(5300+4,UINT32_C(0x7fc00000));BAD32(1000+0x44,UINT32_MAX);
#undef BAD32
#define BAD16(offset,n) do{put16(bytes+(offset),(n));check(!mac_retail_object_spawn_decode(&c,&out,&e));memcpy(bytes,saved,sizeof(bytes));}while(0)
    BAD16(5300+32,1);BAD16(5106,65535);BAD16(1000+0x4e4,2);BAD16(2800,0);
#undef BAD16
    put16(bytes+5106,3);check(mac_retail_object_spawn_decode(&c,&out,&e));check(out.weapons[0].magazines[0].initial_loaded==3&&out.weapons[0].magazines[0].initial_spare==0);
}
static void object_digest(const struct mac_spawn_object*o)
{
    unsigned i;word(o->handle);word(o->runtime_flags);word((uint32_t)(int32_t)o->type);word((uint32_t)(int32_t)o->forced_shader);word(o->flags);
    realword(o->radius);realword(o->acceleration_scale);realword(o->render_radius);for(i=0;i<3;++i){realword(o->bounding_offset[i]);realword(o->origin_offset[i]);}
    for(i=0;i<6;++i)refword(o->refs[i]);word(o->attachment_count);word(o->function_count);word(o->color_count);for(i=0;i<o->attachment_count;++i)refword(o->attachments[i]);
}
int main(int argc,char**argv)
{
    struct mac_retail_cache c;struct mac_retail_object_spawn s;const char*e;FILE*f;long length;unsigned char*bytes;unsigned i,j,k;
    synthetic();printf("object spawn synthetic checks=%u failures=%u\n",checks,failures);if(failures)return 1;if(argc==1)return 0;if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000L||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=malloc((size_t)length);if(!bytes){fclose(f);return 2;}if(fread(bytes,1,(size_t)length,f)!=(size_t)length){fclose(f);free(bytes);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)length,&c,&e)||!mac_retail_object_spawn_decode(&c,&s,&e)){fprintf(stderr,"object spawn: %s\n",e);free(bytes);return 1;}
    object_digest(&s.biped);word(s.unit_flags);word(s.biped_flags);word((uint32_t)(int32_t)s.default_team);word((uint32_t)(int32_t)s.grenade_type);word((uint32_t)(int32_t)s.grenade_count);word((uint32_t)(int32_t)s.pelvis_node);word((uint32_t)(int32_t)s.head_node);
    realword(s.standing_height);realword(s.crouching_height);realword(s.radius);realword(s.standing_camera);realword(s.crouching_camera);word(s.weapon_count);
    printf("biped=%08lx heights=%.9f/%.9f radius=%.9f weapons=%lu cached_nodes=%d/%d\n",(unsigned long)s.biped.handle,s.standing_height,s.crouching_height,s.radius,(unsigned long)s.weapon_count,(int)s.pelvis_node,(int)s.head_node);
    for(i=0;i<s.weapon_count;++i){
        const struct mac_spawn_weapon*w=&s.weapons[i];object_digest(&w->object);word(w->flags);for(k=0;k<32;++k)word((unsigned char)w->label[k]);refword(w->first_person_model);refword(w->first_person_animation);refword(w->hud);word(w->magazine_count);word(w->trigger_count);
        printf("weapon[%u]=%08lx label=%s magazines=%lu triggers=%lu FP_model=%08lx FP_animation=%08lx\n",i,(unsigned long)w->object.handle,w->label,(unsigned long)w->magazine_count,(unsigned long)w->trigger_count,(unsigned long)w->first_person_model.handle,(unsigned long)w->first_person_animation.handle);
        for(j=0;j<w->magazine_count;++j){const struct mac_spawn_magazine*m=&w->magazines[j];word(m->flags);word(m->recharged);word(m->initial);word(m->total_max);word(m->loaded_max);word(m->reloaded);word(m->initial_loaded);word(m->initial_spare);realword(m->reload_time);realword(m->chamber_time);refword(m->reload_effect);refword(m->chamber_effect);word(m->ammunition_count);for(k=0;k<m->ammunition_count;++k){word(m->ammunition_rounds[k]);refword(m->ammunition[k]);}printf(" magazine[%u] initial=%d loaded=%d spare=%d maximum=%d\n",j,(int)m->initial,(int)m->initial_loaded,(int)m->initial_spare,(int)m->total_max);}
        for(j=0;j<w->trigger_count;++j){const struct mac_spawn_trigger*t=&w->triggers[j];word(t->flags);word((uint32_t)(int32_t)t->magazine);word(t->rounds_per_shot);word(t->minimum_loaded);word(t->tracer_rounds);word(t->projectiles_per_shot);realword(t->initial_rate);realword(t->final_rate);refword(t->charging_effect);refword(t->projectile);printf(" trigger[%u] magazine=%d rounds=%d projectile=%08lx\n",j,(int)t->magazine,(int)t->rounds_per_shot,(unsigned long)t->projectile.handle);}
    }
    printf("constructor input digest=%08lx (no objects created)\n",(unsigned long)digest);free(bytes);return 0;
}
