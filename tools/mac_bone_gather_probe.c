#include "../port/mac/mac_bone_gather.h"
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,failures;
#define CHECK(value) do { ++checks; if(!(value)){++failures;printf("FAIL line %d: %s\n",__LINE__,#value);} } while(0)

static void check_address_case(const struct mac_bone_gather_plan *plan,
    const float constants[192][4],float address,unsigned expected0,unsigned expected1,unsigned expected2)
{
    float records[16*4],before[16*4];char error[128];int result;unsigned attribute;
    memset(records,0,sizeof(records));records[5*4]=address;records[5*4+1]=-0.0f;
    records[3*4+2]=17.0f;records[9*4+1]=-23.0f;
    memcpy(before,records,sizeof(records));
    result=mac_bone_gather_apply(plan,records,1,16,constants,error,sizeof(error));
    CHECK(result==1);
    CHECK(records[10*4]==constants[expected0][0]);
    CHECK(records[11*4+1]==constants[expected1][1]);
    CHECK(records[12*4+2]==constants[expected2][2]);
    CHECK(records[13*4]==constants[60][0]&&records[14*4+1]==constants[61][1]&&
        records[15*4+2]==constants[62][2]);
    for(attribute=0;attribute<16;++attribute)if(attribute<10||attribute>15)
        CHECK(!memcmp(records+attribute*4,before+attribute*4,4*sizeof(float)));
}

int main(void)
{
    struct mac_bone_gather_plan plan;
    float constants[192][4],records[2*16*4],before[2*16*4];
    char error[128];unsigned i;int result;
    union { float value; uint32_t bits; } threshold;
    memset(constants,0,sizeof(constants));memset(records,0,sizeof(records));
    for(i=0;i<192;++i){constants[i][0]=(float)i;constants[i][1]=(float)i+0.25f;
        constants[i][2]=(float)i+0.5f;constants[i][3]=(float)i+0.75f;}
    constants[7][3]=1.0f;
    records[5*4]=-61.0f;records[5*4+1]=0.0f;
    records[16*4+5*4]=131.0f;records[16*4+5*4+1]=189.0f;
    memset(&plan,0,sizeof(plan));plan.version=1;plan.address_attribute=5;plan.address_constant=7;
    plan.first_gather_attribute=10;plan.gather_attribute_count=6;plan.row_base=60;plan.row_limit=191;
    result=mac_bone_gather_apply(&plan,records,2,16,(const float (*)[4])constants,error,sizeof(error));
    CHECK(result==1);
    CHECK(records[10*4+0]==constants[0][0]&&records[11*4+1]==constants[0][1]&&
        records[12*4+2]==constants[1][2]);
    CHECK(records[16*4+10*4+0]==constants[191][0]&&
        records[16*4+11*4+1]==constants[191][1]&&records[16*4+12*4+2]==constants[191][2]);
    check_address_case(&plan,(const float (*)[4])constants,-61.0f,0,0,1);
    check_address_case(&plan,(const float (*)[4])constants,-0.0011f,59,60,61);
    check_address_case(&plan,(const float (*)[4])constants,-0.001f,60,61,62);
    check_address_case(&plan,(const float (*)[4])constants,-0.0009f,60,61,62);
    if(sizeof(threshold.value)==sizeof(uint32_t)){
        threshold.value=-0.001f;threshold.bits+=1;
        check_address_case(&plan,(const float (*)[4])constants,threshold.value,59,60,61);
        threshold.value=-0.001f;threshold.bits-=1;
        check_address_case(&plan,(const float (*)[4])constants,threshold.value,60,61,62);
    }else CHECK(0);
    check_address_case(&plan,(const float (*)[4])constants,-0.0f,60,61,62);
    check_address_case(&plan,(const float (*)[4])constants,130.9989f,190,191,191);
    check_address_case(&plan,(const float (*)[4])constants,130.9991f,191,191,191);
    check_address_case(&plan,(const float (*)[4])constants,131.0f,191,191,191);
    check_address_case(&plan,(const float (*)[4])constants,FLT_MAX,191,191,191);
    check_address_case(&plan,(const float (*)[4])constants,-FLT_MAX,0,0,0);
    /* Compare complete records across both clamp boundaries, fractional
     * addresses and positive/negative scales against separate row copies. */
    {
        int sample,axis,row,base,index;unsigned scale_index;
        static const float scales[3]={1.0f,0.5f,-2.0f};
        float expected[16*4];volatile float product,address;
        for(scale_index=0;scale_index<3;++scale_index){
            constants[7][3]=scales[scale_index];
            for(sample=-270;sample<=550;++sample){
                for(i=0;i<16*4;++i)records[i]=(float)i+0.125f;
                records[5*4]=(float)sample*0.25f;
                records[5*4+1]=-(float)sample*0.5f;
                memcpy(expected,records,sizeof(expected));
                for(axis=0;axis<2;++axis){
                    product=records[5*4+axis]*constants[7][3];
                    address=product+0.001f;base=(int)address;
                    if((float)base>address)--base;
                    for(row=0;row<3;++row){
                        index=base+60+row;
                        if(index<0)index=0;else if(index>191)index=191;
                        memcpy(expected+(10+axis*3+row)*4,constants[index],4*sizeof(float));
                    }
                }
                result=mac_bone_gather_apply(&plan,records,1,16,
                    (const float (*)[4])constants,error,sizeof(error));
                CHECK(result==1&&!memcmp(expected,records,sizeof(expected)));
            }
        }
    }
    memcpy(before,records,sizeof(records));
    constants[7][3]=FLT_MAX;records[5*4]=2.0f;
    memcpy(before,records,sizeof(records));
    result=mac_bone_gather_apply(&plan,records,2,16,(const float (*)[4])constants,error,sizeof(error));
    CHECK(result==0&&!memcmp(records,before,sizeof(records)));
    {
        float bad_values[2];unsigned bad;
        bad_values[0]=(float)strtod("nan",NULL);bad_values[1]=(float)strtod("inf",NULL);
        constants[7][3]=1.0f;plan.version=1;
        for(bad=0;bad<2;++bad){memset(records,0,sizeof(records));records[5*4]=bad_values[bad];
            memcpy(before,records,sizeof(records));
            result=mac_bone_gather_apply(&plan,records,1,16,(const float (*)[4])constants,error,sizeof(error));
            CHECK(result==0&&!memcmp(records,before,sizeof(records)));}
        memset(records,0,sizeof(records));records[5*4]=0.0f;
        records[16*4+5*4]=(float)strtod("inf",NULL);memcpy(before,records,sizeof(records));
        result=mac_bone_gather_apply(&plan,records,2,16,(const float (*)[4])constants,error,sizeof(error));
        CHECK(result==0&&!memcmp(records,before,sizeof(records)));
    }
    constants[7][3]=1.0f;plan.version=0;memcpy(before,records,sizeof(records));
    result=mac_bone_gather_apply(&plan,records,2,16,(const float (*)[4])constants,error,sizeof(error));
    CHECK(result==0&&!memcmp(records,before,sizeof(records)));
    printf("native bone gather apply probe: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
