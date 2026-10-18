#include "mapped_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);}}while(0)
int main(void)
{
    char path[]="/tmp/opence-mapped-XXXXXX";const char*error;
    struct mac_mapped_file map={0};int fd=mkstemp(path);
    CHECK(fd>=0);if(fd<0)return 1;
    CHECK(!mac_mapped_file_open(path,16,&map,&error));CHECK(!map.data&&!map.size);
    CHECK(write(fd,"0123456789abcdef",16)==16);close(fd);
    CHECK(!mac_mapped_file_open(path,15,&map,&error));CHECK(!map.data&&!map.size);
    CHECK(mac_mapped_file_open(path,16,&map,&error));CHECK(map.size==16);
    CHECK(unlink(path)==0);
    CHECK(mac_mapped_file_span(&map,4,4)&&!memcmp(mac_mapped_file_span(&map,4,4),"4567",4));
    CHECK(!mac_mapped_file_span(&map,17,0));CHECK(!mac_mapped_file_span(&map,15,2));
    CHECK(!mac_mapped_file_span(&map,(size_t)-1,4));CHECK(!mac_mapped_file_span(&map,1,(size_t)-1));
    CHECK(mac_mapped_file_span(&map,16,0)==map.data+16);
    mac_mapped_file_close(&map);CHECK(!map.data&&!map.size);CHECK(!mac_mapped_file_span(&map,0,0));
    mac_mapped_file_close(&map);CHECK(!mac_mapped_file_open("/tmp",16,&map,&error));
    CHECK(!mac_mapped_file_open(path,16,&map,&error));
    printf("native mapped file: %u checks %u failures\n",checks,failures);return failures?1:0;
}
