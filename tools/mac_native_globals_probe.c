#include "cseries.h"
#include "game/game_globals.h"
#include "native_globals_tags.h"
#include "native_globals_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
static unsigned checks,failures,falling_visits;
static uint32_t falling_handle;
#define CHECK(t) do{++checks;if(!(t)){++failures;fprintf(stderr,"FAIL %s\n",#t);}}while(0)
static uint32_t rd(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void reject(struct mac_retail_cache*c,uint32_t h,unsigned char*p,uint32_t v){
 unsigned char saved[4];unsigned i;const char*e;struct mac_native_object_tag o,z;
 memcpy(saved,p,4);for(i=0;i<4;++i)p[i]=(unsigned char)(v>>(i*8));memset(&z,0,sizeof(z));
 CHECK(!mac_native_globals_decode(c,h,&o,&e));CHECK(!memcmp(&o,&z,sizeof(z)));
 mac_native_object_tag_free(&o);memcpy(p,saved,4);
}
static int visit(void*ctx,uint32_t g,uint32_t h){unsigned*n=ctx;if(g==UINT32_C(0x6a707421)&&h==falling_handle)++falling_visits;++*n;return 1;}
int main(int argc,char**argv){
 FILE*f;long length;unsigned char*bytes,*root,*control,*falling_raw;uint32_t i,h=0;unsigned refs=0;
 struct mac_retail_cache cache;struct mac_retail_tag tag;struct mac_native_object_tag o;
 struct game_globals_falling_damage*falling;
 struct game_globals*g;struct game_globals_player_control*c;const char*error;
 if(argc!=2 || !(f=fopen(argv[1],"rb")))return 2;
 if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000||fseek(f,0,SEEK_SET))return 2;
 bytes=malloc(length);if(!bytes||fread(bytes,1,length,f)!=(size_t)length)return 2;fclose(f);
 if(!mac_retail_open(bytes,length,&cache,&error))return 2;
 for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag)&&tag.group==UINT32_C(0x6d617467)){h=tag.handle;break;}
 if(!h||!mac_native_globals_decode(&cache,h,&o,&error)){fprintf(stderr,"decode: %s\n",h?error:"missing");return 1;}
 g=o.root;c=g->player_control.address;
 CHECK(sizeof(*g)==0x1ac);CHECK(g->player_control.count==1);CHECK(c->look_function.count>0);
 CHECK(c->look_default_pitch_rate>0);CHECK(c->look_default_yaw_rate>0);CHECK(g->player_information.count==1);
 falling=g->falling_damage.address;
 CHECK(g->falling_damage.count==1 && falling!=NULL);
 if(!falling)return 1;
 falling_handle=falling->falling_damage.index;
 CHECK(falling->falling_damage.group_tag==UINT32_C(0x6a707421));
 CHECK(falling_handle!=UINT32_MAX);
 CHECK(g->materials.count>0 && g->materials.count<=33);CHECK(g->playlist.count>0);CHECK(mac_native_object_visit_references(&o,visit,&refs));CHECK(refs>20);
 CHECK(falling_visits==1);
 root=(unsigned char*)mac_retail_tag_span(&cache,tag.address,0x1ac);
 control=(unsigned char*)mac_retail_tag_span(&cache,rd(root+offsetof(struct game_globals,player_control)+4),128);
 falling_raw=(unsigned char*)mac_retail_tag_span(&cache,rd(root+offsetof(struct game_globals,falling_damage)+4),sizeof(*falling));
 CHECK(falling_raw!=NULL);
 if(!falling_raw)return 1;
 {
  uint32_t bits;memcpy(&bits,&falling->falling_distance_lower_bound,4);
  CHECK(bits==rd(falling_raw+offsetof(struct game_globals_falling_damage,falling_distance_lower_bound)));
  memcpy(&bits,&falling->falling_distance_upper_bound,4);
  CHECK(bits==rd(falling_raw+offsetof(struct game_globals_falling_damage,falling_distance_upper_bound)));
  memcpy(&bits,&falling->maximum_distance,4);
  CHECK(bits==rd(falling_raw+offsetof(struct game_globals_falling_damage,maximum_distance)));
  CHECK(falling_handle==rd(falling_raw+offsetof(struct game_globals_falling_damage,falling_damage)+12));
 }
 reject(&cache,h,falling_raw+offsetof(struct game_globals_falling_damage,falling_distance_lower_bound),UINT32_C(0x7fc00000));
 reject(&cache,h,falling_raw+offsetof(struct game_globals_falling_damage,falling_damage)+12,0);
 reject(&cache,h,root+offsetof(struct game_globals,player_control),2);
 reject(&cache,h,root+offsetof(struct game_globals,playlist),21);
 reject(&cache,h,control,UINT32_C(0x7fc00000));
 reject(&cache,h,control+116,17);reject(&cache,h,control+120,0);
 reject(&cache,h,root+offsetof(struct game_globals,materials)+4,0);
 printf("native globals handle=%08lx owned=%lu values=%lu digest=%08lx refs=%u materials=%ld playlist=%ld look=%ld pitch=%g yaw=%g\n",(unsigned long)h,(unsigned long)o.bytes_owned,(unsigned long)o.converted_values,(unsigned long)o.field_digest,refs,g->materials.count,g->playlist.count,c->look_function.count,c->look_default_pitch_rate,c->look_default_yaw_rate);
 free(bytes);CHECK(c->look_default_pitch_rate>0);CHECK(((float*)c->look_function.address)[0]>=0);
 mac_native_object_tag_free(&o);CHECK(!o.root&&!o.owner);
 printf("native globals checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
