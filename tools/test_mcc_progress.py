"""Real completion journal: profile/mission isolation and interrupted writes."""
import json
import re
from pathlib import Path

from test_hud_button_style import run_c

ROOT = Path(__file__).resolve().parents[1]


def test_completion_journal(tmp_path):
    for i in range(3): (tmp_path/str(i)).mkdir()
    code = re.sub(r'^#include[^\n]*\n', '', (ROOT/'port/linux/game/mcc_progress.c').read_text(), flags=re.M)
    run_c(tmp_path, r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define _error_silent 0
typedef unsigned long DWORD;
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 16
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
static DWORD GetFileAttributesA(const char *path){FILE *f=fopen(path,"rb");if(!f)return INVALID_FILE_ATTRIBUTES;fclose(f);return 0;}
static DWORD GetLastError(void){return ERROR_FILE_NOT_FOUND;}
static int CopyFileA(const char *from,const char *to,int exclusive){
 FILE *in,*out;int c;(void)exclusive;out=fopen(to,"rb");if(out){fclose(out);return 0;}
 in=fopen(from,"rb");if(!in)return 0;out=fopen(to,"wb");if(!out){fclose(in);return 0;}
 while((c=fgetc(in))!=EOF)fputc(c,out);fclose(in);return fclose(out)==0;
}
static short player_spawn_count=2;
static long profiles[4]={0,1,-1,-1};
static int warnings;
static long player_ui_get_active_player_profile_index(short player) {return profiles[player];}
static int player_profile_get_enclosing_directory_path(long profile,char *path) {
 if(profile<0||profile>1)return 0;
 snprintf(path,256,"%s/%ld/",TEST_ROOT,profile);return 1;
}
static int csstrcasecmp(const char *a,const char *b) {
 while(*a&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}
 return tolower((unsigned char)*a)-tolower((unsigned char)*b);
}
static void error(int severity,const char *format,...) {(void)severity;(void)format;warnings++;}
'''.replace('TEST_ROOT', json.dumps(tmp_path.as_posix())) + code + r'''
static long size(const char *path) {FILE *f=fopen(path,"rb");long n;assert(f);fseek(f,0,SEEK_END);n=ftell(f);fclose(f);return n;}
int main(void) {
 const char *a="mcc_maps\\one\\a10",*b="mcc_maps\\two\\a10";
 char path[300],renamed[300],destination[300],original[300];FILE *f;unsigned char record[76]={0};unsigned checksum;int i;
 snprintf(original,sizeof(original),"%s/0/blam.sav",TEST_ROOT);
 snprintf(destination,sizeof(destination),"%s/2/",TEST_ROOT);
 assert(mcc_progress_copy(original,destination)); /* no companion leaves native rename untouched */
 assert(!mcc_progress_flags(0,a));
 mcc_progress_complete(a,1);
 assert(mcc_progress_flags(0,a)==2&&mcc_progress_flags(1,a)==2&&!mcc_progress_flags(0,b));
 assert(mcc_progress_flags(0,"mcc_maps\\ONE\\A10")==2);
 assert(mcc_progress_path(0,path)&&size(path)==76);
 mcc_progress_complete(a,1);assert(size(path)==76); /* replay is idempotent */
 profiles[1]=-1;mcc_progress_complete(a,3);
 assert(mcc_progress_flags(0,a)==10&&mcc_progress_flags(1,a)==2);
 mcc_progress_complete(b,2);assert(mcc_progress_flags(0,b)==4&&mcc_progress_flags(0,a)==10);
 assert(!mcc_progress_flags(0,"levels\\a10\\a10"));
 assert(!mcc_progress_save(0,"custom_maps\\a10",1));
 assert(!mcc_progress_save(0,a,-1)&&!mcc_progress_save(0,a,4));
 assert(!mcc_progress_save(-1,a,1));
 f=fopen(path,"ab");assert(f);fwrite("MCC1bad",1,7,f);fclose(f);
 assert(mcc_progress_flags(0,a)==10); /* prior good records survive a torn append */
 assert(!mcc_progress_save(0,a,2)&&size(path)==235);
 mcc_progress_complete(a,2);assert(warnings==1&&size(path)==235);
 strcpy(original,path);strcpy(strrchr(original,'/')+1,"blam.sav");
 snprintf(renamed,sizeof(renamed),"%s/2/",TEST_ROOT);
 assert(mcc_progress_copy(original,renamed)); /* rename preserves even a damaged tail */
 strcat(renamed,"mcc_completed.dat");assert(size(renamed)==235&&size(path)==235);
 assert(!mcc_progress_copy(original,destination)); /* exclusive copy refuses an existing companion */
 assert(size(path)==235); /* failure cannot delete the original */
 /* A checksum mismatch and unsupported version cannot manufacture badges. */
 assert(mcc_progress_path(1,path));f=fopen(path,"r+b");assert(f);fputc('X',f);fclose(f);
 assert(!mcc_progress_flags(1,a)&&!mcc_progress_save(1,a,1));
 f=fopen(path,"wb");assert(f);
 memcpy(record,"MCC1",4);strcpy((char*)record+4,a);record[68]=1;
 checksum=mcc_progress_checksum(record);for(i=0;i<4;i++)record[72+i]=(unsigned char)(checksum>>(8*i));
 for(i=0;i<4096;i++)assert(fwrite(record,1,76,f)==76);fclose(f);
 assert(mcc_progress_flags(1,a)==1&&!mcc_progress_save(1,a,2));
 assert(size(path)==4096*76);
 return 0;
}
'''.replace('TEST_ROOT', json.dumps(tmp_path.as_posix())))
