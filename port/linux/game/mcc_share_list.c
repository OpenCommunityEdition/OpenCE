#include "mcc_share_list.h"
#include <string.h>

static int list_nibble(char c)
{
    if (c>='0' && c<='9') return c-'0';
    if (c>='a' && c<='f') return c-'a'+10;
    if (c>='A' && c<='F') return c-'A'+10;
    return -1;
}

void mcc_share_list_hex(unsigned char const digest[32], char text[65])
{
    static char const digits[]="0123456789abcdef";
    unsigned i;
    for (i=0;i<32;i++) {
        text[i*2]=digits[digest[i]>>4];
        text[i*2+1]=digits[digest[i]&15];
    }
    text[64]=0;
}

static int list_line(char const *line, unsigned long length, unsigned char const digest[32],
    char credit[MCC_SHARE_CREDIT_SIZE])
{
    unsigned char found[32];
    unsigned long i,start,used=0;
    if (length<64 || line[0]=='#') return 0;
    for (i=0;i<32;i++) {
        int high=list_nibble(line[i*2]),low=list_nibble(line[i*2+1]);
        if (high<0 || low<0) return 0;
        found[i]=(unsigned char)(high<<4|low);
    }
    if (memcmp(found,digest,32)) return 0;
    /* the digest alone, or followed by blanks, names the package with no maker */
    if (length>64 && line[64]!=' ' && line[64]!='\t') return 0;
    start=65;
    while (start<length && (line[start]==' ' || line[start]=='\t')) start++;
    for (i=start;i<length && used<MCC_SHARE_CREDIT_SIZE-1;i++) {
        unsigned char c=(unsigned char)line[i];
        if (c>=32 && c<127) credit[used++]=(char)c;
    }
    while (used && credit[used-1]==' ') used--;
    credit[used]=0;
    return used>0 ? 1 : -1;
}

int mcc_share_list_find(char const *text, unsigned long length, unsigned char const digest[32],
    char credit[MCC_SHARE_CREDIT_SIZE])
{
    unsigned long start=0,end;
    int found;
    credit[0]=0;
    if (!text) return 0;
    while (start<length) {
        end=start;
        while (end<length && text[end]!='\n' && text[end]!='\r' && text[end]) end++;
        found=list_line(text+start,end-start,digest,credit);
        if (found) return found;
        credit[0]=0;
        while (end<length && (text[end]=='\n' || text[end]=='\r' || !text[end])) end++;
        start=end;
    }
    return 0;
}
