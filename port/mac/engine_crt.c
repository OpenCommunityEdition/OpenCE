/* The engine's labels are ASCII. Keep unsigned byte comparisons independent
   of the host locale, as in the isolated animation selection boundary. */
#include <stddef.h>
int _stricmp(const char *a,const char *b)
{
    unsigned x,y;
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A' && x<='Z')x+='a'-'A';
        if(y>='A' && y<='Z')y+='a'-'A';
        if(x!=y)return x<y?-1:1;
    } while(x);
    return 0;
}

int _strnicmp(const char *a,const char *b,size_t count)
{
    unsigned x,y;
    while(count--) {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='A' && x<='Z')x+='a'-'A';
        if(y>='A' && y<='Z')y+='a'-'A';
        if(x!=y)return x<y?-1:1;
        if(!x)break;
    }
    return 0;
}
#include <stdlib.h>
#include <string.h>
char *_strdup(const char *text)
{
    size_t length=strlen(text)+1;
    char *result=(char *)malloc(length);
    if(result)memcpy(result,text,length);
    return result;
}
