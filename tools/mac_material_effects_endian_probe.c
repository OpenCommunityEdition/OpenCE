#include <stdio.h>

long mac_native_material_effect_collision_type(const void *collision);

static unsigned checks;
static unsigned failures;

static void check(int condition,const char *name)
{
    ++checks;
    if(!condition) {
        ++failures;
        fprintf(stderr,"FAIL: %s\n",name);
    }
}

int main(void)
{
    unsigned char collision[0x50];
    union { unsigned short value; unsigned char bytes[2]; } endian;
    short values[4];
    unsigned pads[3];
    unsigned i,j;

    endian.value=1;
    values[0]=0;values[1]=1;values[2]=31;values[3]=(short)-1;
    pads[0]=0; pads[1]=0x8482U; pads[2]=0xffffU;
    for(i=0;i<4;++i)for(j=0;j<3;++j) {
        unsigned short value=(unsigned short)values[i];
        unsigned short pad=(unsigned short)pads[j];
        collision[0x34]=endian.bytes[0] ? (unsigned char)value : (unsigned char)(value>>8);
        collision[0x35]=endian.bytes[0] ? (unsigned char)(value>>8) : (unsigned char)value;
        collision[0x36]=endian.bytes[0] ? (unsigned char)pad : (unsigned char)(pad>>8);
        collision[0x37]=endian.bytes[0] ? (unsigned char)(pad>>8) : (unsigned char)pad;
        check(mac_native_material_effect_collision_type(collision)==(long)values[i],
            "material type is independent of adjacent padding");
    }
    printf("native material-effect endian probe: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
