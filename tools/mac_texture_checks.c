/* Independent pixel fixtures for the portable retail base-level decoder. */
#include "../port/mac/retail_texture.h"
#include <stdio.h>
#include <string.h>

static unsigned int checks, failures;
#define CHECK(v) do { ++checks; if (!(v)) { ++failures; printf("FAIL line %d: %s\n",__LINE__,#v); } } while (0)
static void pixel(const unsigned char *p, unsigned int r, unsigned int g,
    unsigned int b, unsigned int a)
{ CHECK(p[0]==r && p[1]==g && p[2]==b && p[3]==a); }
static void endpoint(unsigned char *p, unsigned int a, unsigned int b)
{ p[0]=(unsigned char)a; p[1]=(unsigned char)(a>>8); p[2]=(unsigned char)b; p[3]=(unsigned char)(b>>8); }
int main(void)
{
    unsigned char source[64], result[256], before[256];
    unsigned int i, j;
    static const unsigned char a7[8]={255,0,218,182,145,109,72,36};
    static const unsigned char a5[8]={0,255,51,102,153,204,0,255};
    /* Intentionally unaligned source address tests the actual byte readers. */
    memset(source,0,sizeof(source));
    source[1]=0x00; source[2]=0xf8; source[3]=0xe0; source[4]=0x07;
    source[5]=0x1f; source[6]=0; source[7]=0xff; source[8]=0xff;
    CHECK(mac_retail_texture_decode(6,4,1,source+1,8,result,16));
    pixel(result,255,0,0,255); pixel(result+4,0,255,0,255);
    pixel(result+8,0,0,255,255); pixel(result+12,255,255,255,255);
    source[1]=0x21; source[2]=0x08;
    CHECK(mac_retail_texture_decode(6,1,1,source+1,2,result,4));
    pixel(result,8,4,8,255);
    source[1]=0x00;source[2]=0xfc;source[3]=0xe0;source[4]=0x03;
    source[5]=0x1f;source[6]=0x80;source[7]=0x21;source[8]=0x84;
    CHECK(mac_retail_texture_decode(8,4,1,source+1,8,result,16));
    pixel(result,255,0,0,255);pixel(result+4,0,255,0,0);
    pixel(result+8,0,0,255,255);pixel(result+12,8,8,8,255);
    /* A4R4G4B4 uses little-endian words in the PC cache on either CPU. */
    source[1]=0x23;source[2]=0xa1;source[3]=0xff;source[4]=0xff;
    source[5]=0;source[6]=0;source[7]=0x87;source[8]=0x09;
    CHECK(mac_retail_texture_decode(9,4,1,source+1,8,result,16));
    pixel(result,17,34,51,170);pixel(result+4,255,255,255,255);
    pixel(result+8,0,0,0,0);pixel(result+12,153,136,119,0);
    CHECK(!mac_retail_texture_decode(9,4,1,source+1,7,result,16));
    CHECK(!mac_retail_texture_decode(9,4,1,source+1,8,result,15));
    source[1]=19; source[2]=83; source[3]=201; source[4]=37;
    CHECK(mac_retail_texture_decode(10,1,1,source+1,4,result,4));
    pixel(result,201,83,19,255);
    CHECK(mac_retail_texture_decode(11,1,1,source+1,4,result,4));
    pixel(result,201,83,19,37);
    memset(source,0,sizeof(source)); endpoint(source,0xf800,0x001f);
    for (i=4;i<8;++i) source[i]=0xe4; /* Indices 0,1,2,3 each row. */
    CHECK(mac_retail_texture_decode(14,4,4,source,8,result,64));
    for (i=0;i<4;++i) {
        pixel(result+i*16,255,0,0,255); pixel(result+i*16+4,0,0,255,255);
        pixel(result+i*16+8,170,0,85,255); pixel(result+i*16+12,85,0,170,255);
    }
    endpoint(source,0x001f,0xf800);
    CHECK(mac_retail_texture_decode(14,4,4,source,8,result,64));
    pixel(result,0,0,255,255); pixel(result+4,255,0,0,255);
    pixel(result+8,127,0,127,255); pixel(result+12,0,0,0,0);
    /* DXT3 must retain four colors even when endpoint0 <= endpoint1. */
    memset(source,0,sizeof(source)); endpoint(source+8,0x001f,0xf800);
    for (i=0;i<8;++i) source[i]=(unsigned char)((i*2)|((i*2+1)<<4));
    for (i=12;i<16;++i) source[i]=0xff;
    CHECK(mac_retail_texture_decode(15,4,4,source,16,result,64));
    for (i=0;i<16;++i) pixel(result+i*4,170,0,85,i*17);
    /* 3-bit selectors 0..7 twice, packed across byte boundaries. */
    for (j=0;j<2;++j) {
        memset(source,0,sizeof(source)); endpoint(source+8,0xffff,0);
        source[0]=(unsigned char)(j?0:255); source[1]=(unsigned char)(j?255:0);
        source[2]=0x88; source[3]=0xc6; source[4]=0xfa;
        source[5]=0x88; source[6]=0xc6; source[7]=0xfa;
        CHECK(mac_retail_texture_decode(16,4,4,source,16,result,64));
        for (i=0;i<16;++i) pixel(result+i*4,255,255,255,j?a5[i%8]:a7[i%8]);
    }
    /* Four different solid blocks, clipped at the right and bottom edges. */
    memset(source,0,sizeof(source));
    endpoint(source,0xf800,0); endpoint(source+8,0x07e0,0);
    endpoint(source+16,0x001f,0); endpoint(source+24,0xffff,0);
    memset(result,0xa5,sizeof(result));
    CHECK(mac_retail_texture_decode(14,5,5,source,32,result,100));
    for (j=0;j<5;++j) for (i=0;i<5;++i)
        pixel(result+(j*5+i)*4, j<4?(i<4?255:0):(i<4?0:255),
            i<4?0:255, j<4?0:255,255);
    for (i=100;i<sizeof(result);++i) CHECK(result[i]==0xa5);
    memset(result,0xa5,sizeof(result)); memcpy(before,result,sizeof(result));
    CHECK(!mac_retail_texture_decode(14,5,5,source,31,result,100));
    CHECK(!mac_retail_texture_decode(14,5,5,source,33,result,100));
    CHECK(!mac_retail_texture_decode(14,5,5,source,32,result,99));
    CHECK(!mac_retail_texture_decode(14,5,5,source,32,result,101));
    CHECK(!mac_retail_texture_decode(14,0,5,source,0,result,0));
    CHECK(!mac_retail_texture_decode(14,2049,1,source,8,result,4));
    CHECK(!mac_retail_texture_decode(17,1,1,source,8,result,4));
    CHECK(!mac_retail_texture_decode(14,1,1,NULL,8,result,4));
    CHECK(!mac_retail_texture_decode(14,1,1,source,8,NULL,4));
    CHECK(!memcmp(result,before,sizeof(result)));
    printf("texture checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
