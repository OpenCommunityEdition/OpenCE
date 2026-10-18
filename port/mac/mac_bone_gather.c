#include "mac_bone_gather.h"

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error,size_t size,const char *message)
{
    if(error&&size){snprintf(error,size,"%s",message);error[size-1]='\0';}
}

static unsigned count_text(const char *source,const char *needle)
{
    unsigned count=0;size_t length=strlen(needle);const char *cursor=source;
    if(!length)return 0;
    while((cursor=strstr(cursor,needle))!=NULL){++count;cursor+=length;}
    return count;
}

static unsigned count_identifier(const char *source,const char *identifier)
{
    size_t length=strlen(identifier);const char *cursor=source;unsigned count=0;
    while((cursor=strstr(cursor,identifier))!=NULL){
        int left=(cursor==source)||!((cursor[-1]>='a'&&cursor[-1]<='z')||
            (cursor[-1]>='A'&&cursor[-1]<='Z')||(cursor[-1]>='0'&&cursor[-1]<='9')||cursor[-1]=='_');
        int right=!((cursor[length]>='a'&&cursor[length]<='z')||
            (cursor[length]>='A'&&cursor[length]<='Z')||(cursor[length]>='0'&&cursor[length]<='9')||cursor[length]=='_');
        if(left&&right)++count;
        cursor+=length;
    }
    return count;
}

static int rewrite_rows(const char *source,char **rewritten)
{
    static const unsigned offsets[6]={60,61,62,60,61,62};
    const char *cursor=source,*match;size_t source_size=strlen(source),used=0;
    char *result;unsigned read=0;
    result=(char *)malloc(source_size+1);
    if(!result)return -1;
    while((match=strstr(cursor,"c[clamp(a0 + "))!=NULL){
        unsigned offset;int consumed=0,length;char replacement[32];size_t prefix=(size_t)(match-cursor);
        if(read>=6||sscanf(match,"c[clamp(a0 + %u, 0, 191)].xyzw%n",&offset,&consumed)!=1||
            !consumed||offset!=offsets[read]){free(result);return 0;}
        memcpy(result+used,cursor,prefix);used+=prefix;
        length=sprintf(replacement,"v%u.xyzw",10+read);
        memcpy(result+used,replacement,(size_t)length);used+=(size_t)length;
        cursor=match+consumed;++read;
    }
    if(read!=6){free(result);return 0;}
    strcpy(result+used,cursor);*rewritten=result;return 1;
}

static int register_xy_overwritten(const char *begin,const char *end,unsigned reg);

int mac_bone_gather_rewrite(const char *source,struct mac_bone_gather_plan *plan,
    char **rewritten_source,char *error,size_t error_size)
{
    static const char product_prefix[]="A = v5.xyzw;\n\tB = c[7].wwww;\n\tC = v5.xyzw;\n\tmac = A * B;\n\tr";
    static const char address_int[]="a0 = int(floor(mac.x + 0.001));";
    char product[128],arl_x[256],arl_x_legacy[256],arl_y[256],target[32];
    unsigned reg,i;char *rewritten=NULL;int result,consumed=0;
    const char *product_start,*product_end,*first_arl,*first_arl_end,*second_arl,*second_arl_end,*cursor,*read;
    size_t first_arl_size;
    if(plan)memset(plan,0,sizeof(*plan));
    if(rewritten_source)*rewritten_source=NULL;
    if(!source||!plan||!rewritten_source){set_error(error,error_size,"invalid gather rewrite argument");return -1;}
    set_error(error,error_size,"shader does not match supported bone gather pattern");
    if(count_text(source,product_prefix)!=1||count_text(source,address_int)!=2||
        count_identifier(source,"a0")!=9||count_text(source,"c[clamp(a0 + ")!=6)return 0;
    product_start=strstr(source,product_prefix);
    if(!product_start||sscanf(product_start,
        "A = v5.xyzw;\n\tB = c[7].wwww;\n\tC = v5.xyzw;\n\tmac = A * B;\n\tr%u.xy = mac.xy;%n",
        &reg,&consumed)!=1||!consumed||reg>11)return 0;
    product_end=product_start+consumed;
    sprintf(product,"r%u.xy =",reg);
    if(count_text(product_end,product)!=0)return 0;
    sprintf(arl_x,"A = r%u.xyzw;\n\tB = v4.xyzw;\n\tC = v4.zzzz;\n\tmac = A;\n\tilu = C;\n\ta0 = int(floor(mac.x + 0.001));",reg);
    sprintf(arl_x_legacy,"A = r%u.xxxx;\n\tB = v0.xyzw;\n\tC = v0.xyzw;\n\tmac = A;\n\ta0 = int(floor(mac.x + 0.001));",reg);
    sprintf(arl_y,"A = r%u.yyyy;\n\tB = v0.xyzw;\n\tC = v0.xyzw;\n\tmac = A;\n\ta0 = int(floor(mac.x + 0.001));",reg);
    if(count_text(source,arl_x_legacy)+count_text(source,arl_x)!=1||count_text(source,arl_y)!=1)return 0;
    first_arl=strstr(product_end,arl_x_legacy);
    first_arl_size=strlen(arl_x_legacy);
    if(!first_arl){first_arl=strstr(product_end,arl_x);first_arl_size=strlen(arl_x);}
    if(!first_arl)return 0;
    first_arl_end=first_arl+first_arl_size;
    second_arl=strstr(first_arl+1,arl_y);
    if(!second_arl||strstr(first_arl_end,address_int)!=second_arl+strlen(arl_y)-strlen(address_int))return 0;
    if(register_xy_overwritten(product_end,second_arl,reg))return 0;
    second_arl_end=second_arl+strlen(arl_y);
    if(count_text(source,"A = r")<2)return 0;
    /* Exactly three ordered reads must use each ARL value. No relative read
     * may be moved across the second address-register write. */
    cursor=source;
    for(i=0;i<6;++i){
        char row_needle[64];unsigned offset=(i<3?60+i:57+i);
        sprintf(row_needle,"c[clamp(a0 + %u, 0, 191)].xyzw",offset);
        read=strstr(cursor,row_needle);
        if(!read||(i<3&&(read<first_arl_end||read>=second_arl))||
            (i>=3&&read<second_arl_end))return 0;
        cursor=read+strlen(row_needle);
    }
    sprintf(target,"r%u.xy =",reg);
    if(count_text(source,target)!=1)return 0;
    for(i=0;i<6;++i){char name[16];sprintf(name,"v%u",10+i);if(count_identifier(source,name)!=1)return 0;
        sprintf(name,"v%u_in",10+i);if(count_identifier(source,name)!=2)return 0;}
    result=rewrite_rows(source,&rewritten);
    if(result<0){set_error(error,error_size,"out of memory rewriting bone gather shader");return -1;}
    if(!result)return 0;
    plan->version=1;plan->address_attribute=5;plan->address_constant=7;
    plan->first_gather_attribute=10;plan->gather_attribute_count=6;
    plan->row_base=60;plan->row_limit=191;
    *rewritten_source=rewritten;set_error(error,error_size,"");return 1;
}

static int finite_float(float value)
{
    return value==value&&value<=FLT_MAX&&value>=-FLT_MAX;
}

static int register_xy_overwritten(const char *begin,const char *end,unsigned reg)
{
    const char *line=begin;char name[16];sprintf(name,"r%u",reg);
    while(line<end){
        const char *line_end=line,*cursor;size_t length;
        while(line_end<end&&*line_end!='\n')++line_end;
        cursor=line;while(cursor<line_end&&(*cursor==' '||*cursor=='\t'))++cursor;
        length=strlen(name);
        if((size_t)(line_end-cursor)>=length&&!memcmp(cursor,name,length)){
            const char *mask=cursor+length;char swizzle[8];size_t used=0;
            if(mask<line_end&&*mask=='.'){
                ++mask;
                while(mask<line_end&&(*mask=='x'||*mask=='y'||*mask=='z'||*mask=='w')){
                    if(used+1<sizeof(swizzle))swizzle[used++]=*mask;
                    ++mask;
                }
            }
            while(mask<line_end&&(*mask==' '||*mask=='\t'))++mask;
            swizzle[used]='\0';
            if(mask<line_end&&*mask=='='&&strcmp(swizzle,"z")&&
                strcmp(swizzle,"w")&&strcmp(swizzle,"zw"))return 1;
        }
        line=line_end<end?line_end+1:end;
    }
    return 0;
}

static void clamped_rows(float address,unsigned offset,unsigned rows[3])
{
    int base,index;unsigned row;
    /* Large magnitudes can be saturated without an out-of-range float cast. */
    if(address<-1000000.0f){rows[0]=rows[1]=rows[2]=0;return;}
    if(address>1000000.0f){rows[0]=rows[1]=rows[2]=191;return;}
    /* All three rows share the same floored address. */
    base=(int)address;
    if((float)base>address)--base;
    for(row=0;row<3;++row){
        index=base+(int)offset+(int)row;
        if(index<0)index=0;else if(index>191)index=191;
        rows[row]=(unsigned)index;
    }
}

/* The rows one vertex's address (x, y) selects; shared by both entry points
   so that they compute exactly the same rows. */
static int gather_rows_for_address(const struct mac_bone_gather_plan *plan,float x,float y,
    float scale,unsigned rows[6])
{
    volatile float product_x,product_y,biased_x,biased_y;
    if(!finite_float(x)||!finite_float(y))return 0;
    product_x=x*scale;
    product_y=y*scale;
    biased_x=product_x+0.001f;biased_y=product_y+0.001f;
    if(!finite_float(product_x)||!finite_float(product_y)||!finite_float(biased_x)||!finite_float(biased_y))return 0;
    clamped_rows(biased_x,plan->row_base,rows);
    clamped_rows(biased_y,plan->row_base,rows+3);
    return 1;
}

static int gather_rows_for_record(const struct mac_bone_gather_plan *plan,const float *record,
    float scale,unsigned rows[6])
{
    return gather_rows_for_address(plan,record[plan->address_attribute*4],
        record[plan->address_attribute*4+1],scale,rows);
}

int mac_bone_gather_apply(const struct mac_bone_gather_plan *plan,float *records,
    size_t vertex_count,size_t stride_vec4,const float constants[192][4],
    char *error,size_t error_size)
{
    size_t vertex;float scale;
    if(!plan||plan->version!=1||!records||!constants||stride_vec4!=16||vertex_count>65535){
        set_error(error,error_size,"invalid gather apply argument or plan");return 0;}
    if(plan->address_attribute!=5||plan->address_constant!=7||plan->first_gather_attribute!=10||
        plan->gather_attribute_count!=6||plan->row_base!=60||plan->row_limit!=191){
        set_error(error,error_size,"unsupported gather plan fields");return 0;}
    if(!finite_float(constants[7][3])){set_error(error,error_size,"non-finite address scale");return 0;}
    scale=constants[7][3];
    for(vertex=0;vertex<vertex_count;++vertex){
        float *record=records+vertex*stride_vec4*4;unsigned rows[6];
        if(!gather_rows_for_record(plan,record,scale,rows)){
            set_error(error,error_size,"non-finite bone address calculation");return 0;}
    }
    for(vertex=0;vertex<vertex_count;++vertex){float *record=records+vertex*stride_vec4*4;
        unsigned rows[6],axis,row;
        /* This pass is deterministic because all stream inputs were validated
         * above and the caller supplies non-overlapping records/constants. */
        (void)gather_rows_for_record(plan,record,scale,rows);
        for(axis=0;axis<2;++axis){
            unsigned first=axis*3;
            float *destination=record+(plan->first_gather_attribute+first)*4;
            if(rows[first+1]==rows[first]+1&&rows[first+2]==rows[first]+2)
                memcpy(destination,constants[rows[first]],12*sizeof(float));
            else for(row=0;row<3;++row)
                memcpy(destination+row*4,constants[rows[first+row]],4*sizeof(float));
        }
    }
    set_error(error,error_size,"");return 1;
}

int mac_bone_gather_rows(const struct mac_bone_gather_plan *plan,const float *addresses,
    size_t vertex_count,const float constants[192][4],float *rows_out,
    char *error,size_t error_size)
{
    size_t vertex;float scale;
    if(!plan||plan->version!=1||!addresses||!constants||!rows_out||vertex_count>65536){
        set_error(error,error_size,"invalid gather rows argument or plan");return 0;}
    if(plan->address_attribute!=5||plan->address_constant!=7||plan->first_gather_attribute!=10||
        plan->gather_attribute_count!=6||plan->row_base!=60||plan->row_limit!=191){
        set_error(error,error_size,"unsupported gather plan fields");return 0;}
    if(!finite_float(constants[7][3])){set_error(error,error_size,"non-finite address scale");return 0;}
    scale=constants[7][3];
    for(vertex=0;vertex<vertex_count;++vertex){
        unsigned rows[6],axis,row;float *destination;
        if(!gather_rows_for_address(plan,addresses[vertex*2],addresses[vertex*2+1],scale,rows)){
            set_error(error,error_size,"non-finite bone address calculation");return 0;}
        destination=rows_out+vertex*24;
        for(axis=0;axis<2;++axis)for(row=0;row<3;++row)
            memcpy(destination+(axis*3+row)*4,constants[rows[axis*3+row]],4*sizeof(float));
    }
    set_error(error,error_size,"");return 1;
}
