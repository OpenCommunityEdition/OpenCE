#include "cseries.h"
#include "interface/hud_definitions.h"
#include "native_hud_layout.h"
#include "native_hud_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef malloc
#undef free
#undef memcpy
#undef memset
#undef memcmp
static unsigned checks,failures;
#define CHECK(t) do{++checks;if(!(t)){++failures;fprintf(stderr,"FAIL %s\n",#t);}}while(0)
static uint32_t rd(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void reject(struct mac_retail_cache*c,uint32_t h,unsigned char*p,uint32_t v){
 unsigned char saved[4];unsigned i;const char*e;struct mac_native_object_tag o,z;
 memcpy(saved,p,4);for(i=0;i<4;++i)p[i]=(unsigned char)(v>>(i*8));memset(&z,0,sizeof(z));
 CHECK(!mac_native_hud_globals_decode(c,h,&o,&e));CHECK(!memcmp(&o,&z,sizeof(z)));
 mac_native_object_tag_free(&o);memcpy(p,saved,4);
}
static int visit(void*ctx,uint32_t g,uint32_t h){unsigned*n=ctx;(void)g;(void)h;++*n;return 1;}
int main(int argc,char**argv){
 FILE*f;long length;unsigned char*bytes,*root,*control;uint32_t i,h=0;unsigned refs=0;
 struct mac_retail_cache cache;struct mac_retail_tag tag;struct mac_native_object_tag o;
 struct hud_globals_definition*g;struct icon_hud_element_definition*c;const char*error;
 if(argc!=2 || !(f=fopen(argv[1],"rb")))return 2;
 if(fseek(f,0,SEEK_END)||(length=ftell(f))<2048||length>0x30000000||fseek(f,0,SEEK_SET))return 2;
 bytes=malloc(length);if(!bytes||fread(bytes,1,length,f)!=(size_t)length)return 2;fclose(f);
 if(!mac_retail_open(bytes,length,&cache,&error))return 2;
 for(i=0;i<cache.tag_count;++i)if(mac_retail_tag_get(&cache,i,&tag)&&tag.group==UINT32_C(0x68756467)){h=tag.handle;break;}
 if(!h||!mac_native_hud_globals_decode(&cache,h,&o,&error)){fprintf(stderr,"decode: %s\n",h?error:"missing");return 1;}
 g=o.root;c=g->messaging.button_icons.address;
 CHECK(sizeof(*g)==1104);CHECK(g->messaging.button_icons.count>0);CHECK(g->waypoint.arrows.count>0);
 CHECK(g->messaging.up_time>=0);CHECK(g->messaging.fade_time>=0);
 CHECK(mac_native_object_visit_references(&o,visit,&refs));CHECK(refs>5);
 root=(unsigned char*)mac_retail_tag_span(&cache,tag.address,1104);
 control=(unsigned char*)mac_retail_tag_span(&cache,rd(root+offsetof(struct hud_globals_definition,messaging.button_icons)+4),16);
 reject(&cache,h,root+offsetof(struct hud_globals_definition,messaging.button_icons),19);
 reject(&cache,h,root+offsetof(struct hud_globals_definition,waypoint.arrows),17);
 reject(&cache,h,root+offsetof(struct hud_globals_definition,messaging.up_time),UINT32_C(0x7fc00000));
 reject(&cache,h,root+offsetof(struct hud_globals_definition,messaging.button_icons)+4,0);
 reject(&cache,h,root+offsetof(struct hud_globals_definition,messaging.single_player_font)+12,0);
 reject(&cache,h,root+offsetof(struct hud_globals_definition,messaging.absolute_placement),99);
 printf("native HUD handle=%08lx owned=%lu values=%lu digest=%08lx refs=%u icons=%ld arrows=%ld time=%g/%g\n",(unsigned long)h,(unsigned long)o.bytes_owned,(unsigned long)o.converted_values,(unsigned long)o.field_digest,refs,g->messaging.button_icons.count,g->waypoint.arrows.count,g->messaging.up_time,g->messaging.fade_time);
 free(bytes);CHECK(g->messaging.up_time>=0);CHECK(c->sequence_index>=0);CHECK(g->messaging.single_player_font.name && g->messaging.single_player_font.name[0]);
 mac_native_object_tag_free(&o);CHECK(!o.root&&!o.owner);
 printf("native HUD checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
