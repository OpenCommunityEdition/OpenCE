/* Compile the entire shared CPU resource implementation. The generated clean
   declaration section replaces only xgpu.h's GL-dependent include surface. */
#include "resource_c_runtime.h"
/* This TU defines the allocation providers called by the native wrappers. */
#undef D3DDevice_CreateTexture
#undef D3DDevice_CreateCubeTexture
#undef D3DDevice_CreateVolumeTexture
#include "native_resource_memory.h"
#include "build/mac-audit/mac_resource_declarations.h"
#define __HALO_LINUX_XGPU_H
#define D3DDevice_CreateIndexBuffer mac_shared_create_index_buffer
#define D3DResource_Release mac_shared_resource_release
#include "../../port/linux/src/d3d8_resources.c"
#undef D3DDevice_CreateIndexBuffer
#undef D3DResource_Release

/* Index buffers are the shared resource implementation's one malloc-backed
   GPU input allocation. Track that exact Data range so native indexed draws
   can validate it, and retire the range immediately before the shared final
   release frees it. Other resource storage remains owned by the shared code. */
extern HRESULT WINAPI mac_shared_create_index_buffer(UINT,DWORD,D3DFORMAT,
	D3DPOOL,D3DIndexBuffer **);
extern ULONG WINAPI mac_shared_resource_release(D3DResource *);

HRESULT WINAPI D3DDevice_CreateIndexBuffer(UINT length,DWORD usage,
	D3DFORMAT format,D3DPOOL pool,D3DIndexBuffer **result)
{
	HRESULT status;
	status=mac_shared_create_index_buffer(length,usage,format,pool,result);
	if(status!=S_OK||!result||!*result)
		return status;
	if(!mac_resource_register_owned_range((const void *)(*result)->Data,
		length?length:1)) {
		mac_shared_resource_release((D3DResource *)*result);
		*result=NULL;
		return E_OUTOFMEMORY;
	}
	return status;
}

ULONG WINAPI D3DResource_Release(D3DResource *resource)
{
	DWORD *fields=(DWORD *)resource;
	ULONG count;
	if(!resource)
		return 0;
	count=fields[0]&D3DCOMMON_REFCOUNT_MASK;
	if(count==1&&(fields[0]&D3DCOMMON_D3DCREATED)&&
		(fields[0]&D3DCOMMON_TYPE_MASK)==D3DCOMMON_TYPE_INDEXBUFFER)
		mac_resource_unregister_owned_range((const void *)fields[1]);
	return mac_shared_resource_release(resource);
}
