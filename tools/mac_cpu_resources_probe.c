/* This probe exercises shared allocation providers, below device wrappers. */
#undef D3DDevice_CreateTexture
#undef D3DDevice_CreateCubeTexture
#undef D3DDevice_CreateVolumeTexture
#include "native_resource_memory.h"
#include "build/mac-audit/mac_resource_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static unsigned checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %s line%d\n",#x,__LINE__);}}while(0)
void platform_log(const char *format,...){va_list a;va_start(a,format);vfprintf(stderr,format,a);va_end(a);fputc('\n',stderr);}
int main(void)
{
    D3DVertexBuffer *vertex=NULL,*second_vertex=NULL;D3DIndexBuffer *indices=NULL;D3DTexture *texture=NULL,owned;
    D3DLOCKED_RECT lock;D3DSURFACE_DESC desc;struct xgpu_texture_description info;
    unsigned char *p;void *fixed,*certificate_base,*second_certificate_base;unsigned long before,serial,certificate_generation,certificate_offset,certificate_remaining;unsigned long long certificate_token,certificate_token2,certificate_token3,certificate_token4,first_owner_token,second_owner_token,reissued_owner_token;unsigned i;
    CHECK(mac_resource_allocation_count()==0);
    CHECK(!platform_contiguous_alloc(0,128,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE));
    CHECK(!platform_contiguous_alloc(32,3,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE));
    CHECK(!platform_contiguous_alloc(0x08000000,128,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE));
    CHECK(!platform_contiguous_alloc(32,128,0,PAGE_READWRITE));
    CHECK(!platform_contiguous_alloc(32,128,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READONLY));
    fixed=platform_contiguous_alloc(64,16384,0x100000,PAGE_READWRITE);CHECK(fixed==(void*)0x80100000UL);
    ((unsigned char*)fixed)[0]=0x75;
    CHECK(!platform_contiguous_alloc(64,16384,0x100000,PAGE_READWRITE));CHECK(((unsigned char*)fixed)[0]==0x75);
    CHECK(mac_resource_allocation_count()==1&&mac_resource_allocation_bytes()==4096);
    serial=mac_resource_write_serial();
    memory_watch_prepare_write((unsigned char*)fixed+64,0);
    CHECK(mac_resource_write_serial()==serial); /* Requested end, not VM end. */
    memory_watch_prepare_write((unsigned char*)fixed+128,0);
    CHECK(mac_resource_write_serial()==serial); /* Page padding is not owned. */
    platform_contiguous_free(fixed);CHECK(mac_resource_allocation_count()==0);
    CHECK(D3DDevice_CreateVertexBuffer(8192,0,0,D3DPOOL_DEFAULT,&vertex)==S_OK&&vertex);
    CHECK(vertex->Common==(D3DCOMMON_TYPE_VERTEXBUFFER|D3DCOMMON_D3DCREATED|1));
    certificate_base=PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data);
    CHECK(platform_is_contiguous(certificate_base));
    serial=mac_resource_write_serial();D3DVertexBuffer_Lock(vertex,4090,20,&p,0);
    CHECK(p==(unsigned char*)PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data)+4090);
    CHECK(mac_resource_write_serial()>serial);
    CHECK(mac_resource_range_generation(p,20)==mac_resource_write_serial());
    CHECK(mac_resource_certificate_register(vertex,
        PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data),8192,&certificate_token));
    CHECK(certificate_token!=0);
    first_owner_token=certificate_token;
    /* A fresh handle still cannot certify an interior or shortened span. */
    CHECK(!mac_resource_certificate_register(&owned,
        (unsigned char*)certificate_base+1,8191,&certificate_token2));
    CHECK(!mac_resource_certificate_register(&indices,certificate_base,8191,&certificate_token2));
    /* Probe the exact allocation endpoint before allocating an adjacent
       buffer, so the one-past pointer cannot belong to another resource. */
    {
        unsigned char *interior=p;
        serial=mac_resource_write_serial();
        D3DVertexBuffer_Lock(vertex,8192,0,&p,0);
        CHECK(mac_resource_write_serial()==serial);
        p=interior;
    }
    CHECK(D3DDevice_CreateVertexBuffer(128,0,0,D3DPOOL_DEFAULT,&second_vertex)==S_OK&&second_vertex);
    second_certificate_base=PLATFORM_PHYSICAL_TO_VIRTUAL(second_vertex->Data);
    /* Duplicate-owner and duplicate-base failures are tested against a second
       exact live allocation; the second resource then certifies successfully. */
    CHECK(!mac_resource_certificate_register(vertex,second_certificate_base,128,&certificate_token2));
    CHECK(!mac_resource_certificate_register(&owned,certificate_base,8192,&certificate_token2));
    CHECK(mac_resource_certificate_register(second_vertex,second_certificate_base,128,&certificate_token2));
    second_owner_token=certificate_token2;
    CHECK(second_owner_token>first_owner_token);
    CHECK(mac_resource_certificate_query(second_certificate_base,128,&certificate_token3,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token3==certificate_token2&&certificate_offset==0&&certificate_remaining==128);
    CHECK(mac_resource_certificate_query(p,20,&certificate_token2,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token2==certificate_token&&certificate_offset==4090&&certificate_remaining==4102);
    CHECK(certificate_generation==mac_resource_range_generation(
        PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data),8192));
    CHECK(!mac_resource_certificate_query(p+4096,16,&certificate_token2,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token2==0&&certificate_generation==0&&certificate_offset==0&&certificate_remaining==0);
    serial=certificate_generation=mac_resource_range_generation(
        PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data),8192);
    memory_watch_prepare_write((unsigned char*)PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data)+5000,1);
    CHECK(mac_resource_certificate_query(p,4,&certificate_token2,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_generation>serial&&certificate_offset==4090&&certificate_remaining==4102);
    CHECK(mac_resource_certificate_revoke(vertex));
    CHECK(!mac_resource_certificate_query(p,4,&certificate_token2,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(!mac_resource_certificate_revoke(vertex));
    for(i=0;i<20;++i)p[i]=(unsigned char)(i+7);
    CHECK(p[0]==7&&p[19]==26);
    serial=mac_resource_write_serial();D3DVertexBuffer_Lock(vertex,0,0,&p,0);
    CHECK(mac_resource_write_serial()>serial);
    CHECK(mac_resource_range_generation(p,1)==mac_resource_write_serial());
    CHECK(mac_resource_range_generation(p+8191,1)==mac_resource_write_serial());
    serial=mac_resource_write_serial();
    D3DVertexBuffer_Lock(vertex,4096,0,&p,D3DLOCK_READONLY);
    CHECK(mac_resource_write_serial()>serial);
    CHECK(mac_resource_range_generation(p,4096)==mac_resource_write_serial());
    CHECK(mac_resource_range_generation(p-4096,4096)==serial);
    CHECK(mac_resource_certificate_register(vertex,
        PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data),8192,&certificate_token3));
    reissued_owner_token=certificate_token3;
    CHECK(reissued_owner_token>second_owner_token);
    CHECK(mac_resource_certificate_query(PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data),1,
        &certificate_token4,&certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token4==certificate_token3&&certificate_offset==0&&certificate_remaining==8192);
    /* Resource release automatically removes any remaining certificate. */
    CHECK(D3DResource_Release((D3DResource*)vertex)==0);CHECK(mac_resource_allocation_count()==1);
    CHECK(!mac_resource_certificate_query(certificate_base,1,&certificate_token,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token==0);
    CHECK(D3DResource_Release((D3DResource*)second_vertex)==0);CHECK(mac_resource_allocation_count()==0);
    CHECK(!mac_resource_certificate_query(second_certificate_base,1,&certificate_token,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token==0);
    vertex=NULL;
    CHECK(D3DDevice_CreateVertexBuffer(256,0,0,D3DPOOL_DEFAULT,&vertex)==S_OK&&vertex);
    certificate_base=PLATFORM_PHYSICAL_TO_VIRTUAL(vertex->Data);
    D3DVertexBuffer_Lock(vertex,0,256,&p,0);
    CHECK(mac_resource_certificate_register(vertex,certificate_base,256,&certificate_token));
    CHECK(certificate_token>reissued_owner_token);
    CHECK(mac_resource_certificate_query(certificate_base,256,&certificate_token4,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token4==certificate_token&&certificate_offset==0&&certificate_remaining==256);
    CHECK(D3DResource_Release((D3DResource*)vertex)==0);
    CHECK(!mac_resource_certificate_query(certificate_base,1,&certificate_token,
        &certificate_generation,&certificate_offset,&certificate_remaining));
    CHECK(certificate_token==0);
    CHECK(D3DDevice_CreateIndexBuffer(12,0,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&indices)==S_OK&&indices);
    CHECK(indices->Common==(D3DCOMMON_TYPE_INDEXBUFFER|D3DCOMMON_D3DCREATED|1));
    CHECK(mac_resource_range_is_allocated((const void *)indices->Data,12));
    CHECK(!mac_resource_range_is_allocated((const void *)indices->Data,13));
    CHECK(IDirect3DIndexBuffer8_Lock(indices,2,4,&p,0)==S_OK);p[0]=0x12;p[3]=0x34;
    CHECK(p==(unsigned char*)indices->Data+2&&p[3]==0x34);
    indices->Common++;
    CHECK(D3DResource_Release((D3DResource*)indices)==1);
    CHECK(mac_resource_range_is_allocated((const void *)indices->Data,12));
    CHECK(D3DResource_Release((D3DResource*)indices)==0);
    CHECK(!mac_resource_range_is_allocated((const void *)indices->Data,12));
    CHECK(D3DDevice_CreateTexture(32,32,6,0,D3DFMT_DXT1,D3DPOOL_DEFAULT,&texture)==S_OK&&texture);
    xgpu_texture_describe(texture->Format,texture->Size,&info);CHECK(info.width==32&&info.height==32&&info.levels==6&&info.compressed);
    CHECK(xgpu_texture_face_size(&info)==696);CHECK(xgpu_texture_level_offset(&info,5)==688);
    D3DTexture_LockRect(texture,5,&lock,NULL,0);CHECK(lock.Pitch==8);CHECK(lock.pBits==(unsigned char*)PLATFORM_PHYSICAL_TO_VIRTUAL(texture->Data)+688);
    memset(lock.pBits,0xa6,8);D3DTexture_GetLevelDesc(texture,5,&desc);CHECK(desc.Width==1&&desc.Height==1&&desc.Size==8);
    CHECK(((unsigned char*)lock.pBits)[7]==0xa6);CHECK(D3DResource_Release((D3DResource*)texture)==0);CHECK(mac_resource_allocation_count()==0);
    CHECK(D3DDevice_CreateTexture(17,9,0,0,D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&texture)==S_OK&&texture);
    D3DTexture_LockRect(texture,0,&lock,NULL,0);CHECK(lock.Pitch==128);
    D3DTexture_GetLevelDesc(texture,0,&desc);CHECK(desc.Width==17&&desc.Height==9&&desc.Size==1152);
    CHECK(D3DResource_Release((D3DResource*)texture)==0);
    memset(&owned,0,sizeof(owned));fixed=platform_contiguous_alloc(16,128,PLATFORM_ANY_PHYSICAL_ADDRESS,PAGE_READWRITE);CHECK(fixed!=NULL);
    owned.Common=D3DCOMMON_TYPE_TEXTURE|1;owned.Data=PLATFORM_VIRTUAL_TO_PHYSICAL(fixed);before=mac_resource_allocation_count();
    CHECK(D3DResource_Release((D3DResource*)&owned)==0);CHECK(mac_resource_allocation_count()==before);((unsigned char*)fixed)[0]=0x6d;CHECK(((unsigned char*)fixed)[0]==0x6d);
    platform_contiguous_free(fixed);CHECK(mac_resource_allocation_count()==0&&mac_resource_allocation_bytes()==0);
    printf("shared CPU resources: %u checks, %u failures; real buffers/textures/release and explicit write generations\n",checks,failures);return failures?1:0;
}
