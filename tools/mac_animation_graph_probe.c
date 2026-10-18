#include "retail_animation_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t digest=UINT32_C(2166136261),lists,indices_count,ik_count;
static void word(uint32_t n) { digest=(digest^n)*UINT32_C(16777619); }
static void name(const char *s) { unsigned i;for(i=0;i<32;++i)word((unsigned char)s[i]); }
static void list(const struct mac_animation_indices *a)
{ uint32_t i;++lists;indices_count+=a->count;word(a->count);for(i=0;i<a->count;++i)word((uint16_t)a->indices[i]); }
static void aiming(const struct mac_animation_aiming *a)
{
    uint32_t bits;
    memcpy(&bits,&a->negative_yaw_delta,4);word(bits);memcpy(&bits,&a->positive_yaw_delta,4);word(bits);
    memcpy(&bits,&a->negative_pitch_delta,4);word(bits);memcpy(&bits,&a->positive_pitch_delta,4);word(bits);
    word((uint16_t)a->negative_yaw_frame_count);word((uint16_t)a->positive_yaw_frame_count);
    word((uint16_t)a->negative_pitch_frame_count);word((uint16_t)a->positive_pitch_frame_count);
}
static void ik(const struct mac_animation_ik *p,uint32_t count)
{ uint32_t i;ik_count+=count;word(count);for(i=0;i<count;++i){name(p[i].marker_name);name(p[i].attached_to_marker_name);} }
static uint32_t raw32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static unsigned mutation_checks,mutation_failures;
static void reject(const struct mac_retail_cache *cache,uint32_t handle,unsigned char *field,unsigned size,uint32_t value)
{
    unsigned char saved[32];unsigned i;struct mac_retail_animation_graph g,empty;const char *error;int ok;
    memcpy(saved,field,size);
    for(i=0;i<size;++i)field[i]=size==32?'X':(unsigned char)(value>>(8*i));
    memset(&g,0xa5,sizeof(g));memset(&empty,0,sizeof(empty));
    ok=mac_retail_animation_graph_decode(cache,handle,&g,&error);++mutation_checks;
    if(ok || memcmp(&g,&empty,sizeof(g)))++mutation_failures;
    mac_retail_animation_graph_free(&g);memcpy(field,saved,size);
}
static void mutations(const struct mac_retail_cache *cache,uint32_t handle,uint32_t root)
{
    unsigned char *p=(unsigned char *)mac_retail_tag_span(cache,root,128);
    unsigned char *seat=(unsigned char *)mac_retail_tag_span(cache,raw32(p+16),100);
    unsigned char *classes=seat?(unsigned char *)mac_retail_tag_span(cache,raw32(seat+92),188):NULL;
    unsigned char *types=classes?(unsigned char *)mac_retail_tag_span(cache,raw32(classes+180),60):NULL;
    unsigned char *sounds=(unsigned char *)mac_retail_tag_span(cache,raw32(p+88),20);
    unsigned char *clip=(unsigned char *)mac_retail_tag_span(cache,raw32(p+120),180);
    unsigned char *listp=classes?(unsigned char *)mac_retail_tag_span(cache,raw32(classes+156),2):NULL;
    /* This fixture family targets the already validated cyborg graph. */
    if(!seat || !classes || !types || !sounds || !clip || !listp) { ++mutation_failures;return; }
    reject(cache,handle,p+12,4,33);reject(cache,handle,p+16,4,UINT32_MAX);
    reject(cache,handle,seat,32,0);reject(cache,handle,seat+32,4,UINT32_C(0x7fc00000));
    reject(cache,handle,seat+76,4,5);reject(cache,handle,seat+88,4,17);
    reject(cache,handle,classes+176,4,17);reject(cache,handle,types+48,4,11);
    reject(cache,handle,listp,2,32767);reject(cache,handle,sounds+12,4,0);
    reject(cache,handle,clip+56,2,0);reject(cache,handle,p+36,4,1);
}
static void overlay_mutations(const struct mac_retail_cache *cache,uint32_t handle,uint32_t root)
{
    unsigned char *p=(unsigned char *)mac_retail_tag_span(cache,root,128);
    unsigned char *overlay;
    if(!p || raw32(p)!=1 || !(overlay=(unsigned char *)mac_retail_tag_span(cache,raw32(p+4),20))) {
        ++mutation_failures;return;
    }
    reject(cache,handle,p,4,257);              /* overlay count limit */
    reject(cache,handle,p+4,4,UINT32_MAX);     /* bounded cache pointer */
    reject(cache,handle,overlay,2,1);          /* animation index must exist */
    reject(cache,handle,overlay+2,2,4);        /* object function slots are 0..3 or NONE */
    reject(cache,handle,overlay+4,2,2);        /* frame/scale enum */
}
int main(int argc,char **argv)
{
    FILE *f;long size;unsigned char *bytes;struct mac_retail_cache cache;
    struct mac_retail_animation_graph g;struct mac_retail_tag t;const char *error;
    uint32_t h=0,i,j,k,classes=0,types=0;
    if(argc!=2)return 2;
    f=fopen(argv[1],"rb");if(!f)return 2;
    if(fseek(f,0,SEEK_END) || (size=ftell(f))<2048 || size>0x30000000L || fseek(f,0,SEEK_SET)){fclose(f);return 2;}
    bytes=(unsigned char *)malloc((size_t)size);if(!bytes){fclose(f);return 2;}
    if(fread(bytes,1,(size_t)size,f)!=(size_t)size){free(bytes);fclose(f);return 2;}fclose(f);
    if(!mac_retail_open(bytes,(size_t)size,&cache,&error)){fprintf(stderr,"map: %s\n",error);free(bytes);return 1;}
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&t) && t.group==UINT32_C(0x616e7472) &&
        !strcmp(t.name,"characters\\cyborg\\cyborg")){h=t.handle;break;}
    if(!h || !mac_retail_animation_graph_decode(&cache,h,&g,&error)){fprintf(stderr,"selection graph: %s\n",h?error:"missing");free(bytes);return 1;}
    word(g.handle);word(g.node_count);word(g.animation_count);word(g.seat_count);
    list(&g.weapon_animations);list(&g.unit_damage_animations);
    for(i=0;i<g.sound_count;++i)word(g.sound_handles[i]);
    for(i=0;i<g.seat_count;++i) {
        struct mac_animation_seat *s=g.seats+i;classes+=s->class_count;
        name(s->label);aiming(&s->looking);list(&s->animations);ik(s->ik_points,s->ik_count);word(s->class_count);
        for(j=0;j<s->class_count;++j) {
            struct mac_animation_class *c=s->classes+j;types+=c->type_count;
            name(c->label);name(c->grip_marker_name);name(c->hand_marker_name);aiming(&c->aiming);
            list(&c->animations);ik(c->ik_points,c->ik_count);word(c->type_count);
            for(k=0;k<c->type_count;++k){name(c->types[k].label);list(&c->types[k].animations);}
        }
    }
    printf("native selection graph: seats=%lu classes=%lu types=%lu lists=%lu indices=%lu IK=%lu sounds=%lu clips=%lu digest=%08lx\n",
        (unsigned long)g.seat_count,(unsigned long)classes,(unsigned long)types,(unsigned long)lists,
        (unsigned long)indices_count,(unsigned long)ik_count,(unsigned long)g.sound_count,(unsigned long)g.animation_count,(unsigned long)digest);
    mac_retail_animation_graph_free(&g);mac_retail_animation_graph_free(&g);
    mutations(&cache,h,t.address);
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&t) && t.group==UINT32_C(0x616e7472) &&
        !strcmp(t.name,"scenery\\plants\\plant_broadleaf_short\\plant_broadleaf_short"))break;
    if(i==cache.tag_count)++mutation_failures;
    else overlay_mutations(&cache,t.handle,t.address);
    for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&t) &&
        t.group==UINT32_C(0x616e7472) && !strcmp(t.name,"weapons\\flamethrower\\fp\\fp"))break;
    ++mutation_checks;
    if(i==cache.tag_count || !mac_retail_animation_graph_decode(&cache,t.handle,&g,&error)) {
        fprintf(stderr,"flamethrower first-person graph: %s\n",i==cache.tag_count?"missing":error);
        ++mutation_failures;
    } else {
        if(g.seat_count!=1 || g.seats[0].looking.negative_yaw_frame_count!=1912 ||
           g.seats[0].looking.positive_yaw_frame_count!=62 ||
           g.seats[0].looking.negative_pitch_frame_count!=6118 ||
           g.seats[0].looking.positive_pitch_frame_count!=30709 ||
           g.seats[0].animations.count!=12 || g.seats[0].class_count!=1)++mutation_failures;
        mac_retail_animation_graph_free(&g);
    }
    printf("selection graph mutation checks=%u failures=%u\n",mutation_checks,mutation_failures);
    free(bytes);return mutation_failures?1:0;
}
