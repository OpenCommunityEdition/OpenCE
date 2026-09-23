#ifndef HALO_D3D8_DEVICE_LAYOUT_H
#define HALO_D3D8_DEVICE_LAYOUT_H
/* Original field names, types and offsets are decoded from cachebeta.pdb.
 * Opaque kernel-owned records remain reserved pending their own reconstruction. */
#include <xtl.h>
#include <stddef.h>
#include "kernel_records.h"
struct _KINTERRUPT;
struct _KDPC;
struct _HAL_SHUTDOWN_REGISTRATION;
struct Nv206eControl;
struct NvNotification;
struct _XMETAL_PushBuffer
{
    DWORD *m_pPut;
    DWORD *m_pThreshold;
};
namespace D3D
{
class CDevice;
struct _HWREG;
struct FenceEncoding;
struct VertexShader;
struct PixelShader;
struct Light;

struct Fence
{
    unsigned long Time; /* +0x0 */
    D3D::FenceEncoding *pEncoding; /* +0x4 */
    unsigned long Distance; /* +0x8 */
};
struct InlineAttributeData
{
    unsigned long UP_Count; /* +0x0 */
    unsigned long UP_Delta; /* +0x4 */
};
struct PixelShader
{
    unsigned long RefCount; /* +0x0 */
    unsigned long D3DOwned; /* +0x4 */
    _D3DPixelShaderDef *pPSDef; /* +0x8 */
};
struct Light
{
    _D3DLIGHT8 Light8; /* +0x0 */
    unsigned long Flags; /* +0x68 */
    _D3DVECTOR Direction; /* +0x6c */
    float Falloff_L; /* +0x78 */
    float Falloff_M; /* +0x7c */
    float Falloff_N; /* +0x80 */
    float Scale; /* +0x84 */
    float W; /* +0x88 */
    D3D::Light *pNext; /* +0x8c */
};
class CMiniport
{
public:
    struct PUSHBUFFERFIXUPINFO
    {
    unsigned long *pFixup; /* +0x0 */
    unsigned char *pStart; /* +0x4 */
    unsigned long ReturnOffset; /* +0x8 */
    unsigned long *ReturnAddress; /* +0xc */
    };
    struct VBLANKFLIPS
    {
    int Pending; /* +0x0 */
    unsigned long Offset; /* +0x4 */
    };
    struct HW_HASHENTRY
    {
    unsigned long ht_ObjectHandle; /* +0x0 */
    unsigned long ht_Context; /* +0x4 */
    };
    struct VIDEOMODETIMING
    {
    unsigned long HorizontalVisible; /* +0x0 */
    unsigned long VerticalVisible; /* +0x4 */
    unsigned long Refresh; /* +0x8 */
    unsigned long HorizontalTotal; /* +0xc */
    unsigned long HorizontalBlankStart; /* +0x10 */
    unsigned long HorizontalRetraceStart; /* +0x14 */
    unsigned long HorizontalRetraceEnd; /* +0x18 */
    unsigned long HorizontalBlankEnd; /* +0x1c */
    unsigned long VerticalTotal; /* +0x20 */
    unsigned long VerticalBlankStart; /* +0x24 */
    unsigned long VerticalRetraceStart; /* +0x28 */
    unsigned long VerticalRetraceEnd; /* +0x2c */
    unsigned long VerticalBlankEnd; /* +0x30 */
    unsigned long PixelClock; /* +0x34 */
    unsigned long HorizontalSyncPolarity; /* +0x38 */
    unsigned long VerticalSyncPolarity; /* +0x3c */
    };
    struct HALINFO
    {
    long FifoChID; /* +0x0 */
    unsigned long FifoMode; /* +0x4 */
    int FifoInUse; /* +0x8 */
    unsigned long FifoInstance; /* +0xc */
    unsigned long FifoAllocCount; /* +0x10 */
    long FifoCacheDepth; /* +0x14 */
    unsigned long FifoObjectCount; /* +0x18 */
    unsigned long FifoIntrEn0; /* +0x1c */
    unsigned long FifoRetryCount; /* +0x20 */
    unsigned long FifoUserBase; /* +0x24 */
    unsigned long FifoContextAddr1; /* +0x28 */
    unsigned long FifoContextAddr2; /* +0x2c */
    unsigned long HashTableAddr; /* +0x30 */
    unsigned long GrChID; /* +0x34 */
    unsigned long GrCtxTable[2]; /* +0x38 */
    unsigned long GrCtxTableBase; /* +0x40 */
    unsigned long GrCurrentObjects3d[2]; /* +0x44 */
    unsigned long FbSave0; /* +0x4c */
    unsigned long FbSave1; /* +0x50 */
    unsigned long McSave; /* +0x54 */
    unsigned long McSaveIntrEn0; /* +0x58 */
    };
    struct DACINFO
    {
    unsigned long MClk; /* +0x0 */
    unsigned long VClk; /* +0x4 */
    unsigned long NVClk; /* +0x8 */
    unsigned long MPllM; /* +0xc */
    unsigned long MPllN; /* +0x10 */
    unsigned long MPllO; /* +0x14 */
    unsigned long MPllP; /* +0x18 */
    unsigned long VPllM; /* +0x1c */
    unsigned long VPllN; /* +0x20 */
    unsigned long VPllO; /* +0x24 */
    unsigned long VPllP; /* +0x28 */
    unsigned long NVPllM; /* +0x2c */
    unsigned long NVPllN; /* +0x30 */
    unsigned long NVPllO; /* +0x34 */
    unsigned long NVPllP; /* +0x38 */
    };
    struct GENERALINFO
    {
    unsigned long ChipId; /* +0x0 */
    unsigned long VideoRamSize; /* +0x4 */
    unsigned long VideoRamType; /* +0x8 */
    unsigned long ChipIntrEn0; /* +0xc */
    unsigned long MpVIPSlavePresent; /* +0x10 */
    unsigned long CrystalFreq; /* +0x14 */
    unsigned long MaskRevision; /* +0x18 */
    };
    struct OBJECTINFO
    {
        DWORD Handle;
        WORD SubChannel;
        WORD Engine;
        DWORD ClassNum;
        DWORD Instance;
        void Init(
            void)
        {
            memset(this, 0, sizeof(*this));
            return;
        }
    };
    void *m_RegisterBase; /* +0x0 */
    unsigned long m_PresentationInterval; /* +0x4 */
    unsigned long m_SurfacePitch; /* +0x8 */
    unsigned long m_DisplayMode; /* +0xc */
    unsigned long m_Format; /* +0x10 */
    void *m_InstMem; /* +0x14 */
    _KINTERRUPT m_InterruptObject; /* +0x18 */
    _KDPC m_Dpc; /* +0x88 */
    int m_InterruptsEnabled; /* +0xa4 */
    D3D::CMiniport::GENERALINFO m_GenInfo; /* +0xa8 */
    D3D::CMiniport::DACINFO m_DacInfo; /* +0xc4 */
    D3D::CMiniport::HALINFO m_HalInfo; /* +0x100 */
    D3D::CMiniport::VIDEOMODETIMING m_VideoModeTiming; /* +0x15c */
    unsigned long m_VideoModeDepth; /* +0x19c */
    unsigned long m_FreeInstAddr; /* +0x1a0 */
    _HAL_SHUTDOWN_REGISTRATION m_ShutdownRegistration; /* +0x1a4 */
    D3D::CMiniport::VBLANKFLIPS m_VBlankFlips[5]; /* +0x1b4 */
    unsigned long m_VBlankFlipNext; /* +0x1dc */
    unsigned long m_VBlankQueueNext; /* +0x1e0 */
    D3DCALLBACK m_pVerticalBlankCallback; /* +0x1e4 */
    _KEVENT m_VerticalBlankEvent; /* +0x1e8 */
    _KEVENT m_BusyBlockEvent; /* +0x1f8 */
    unsigned long m_CurrentAvInfo; /* +0x208 */
    int m_FirstFlip; /* +0x20c */
    volatile unsigned long m_VBlankFlipCount; /* +0x210 */
    _D3DGAMMARAMP m_GammaRamp[2]; /* +0x214 */
    int m_GammaUpdated[2]; /* +0x814 */
    unsigned long m_GammaCurrentIndex; /* +0x81c */
    volatile unsigned long m_VBlankCount; /* +0x820 */
    unsigned long m_OverlayVBlank; /* +0x824 */
    unsigned long m_DebugRegister[11]; /* +0x828 */
    unsigned long m_PusherGetRunSize; /* +0x854 */

    static unsigned long WINAPI GetPresentFlagsFromAvInfo(
        unsigned long avInfo);

    static unsigned long WINAPI GetDisplayCapabilities(
        void);

    void SetVideoMode(
        unsigned long arg0,
        unsigned long arg1,
        unsigned long arg2,
        unsigned long arg3,
        _D3DFORMAT arg4,
        unsigned long arg5,
        unsigned long arg6);

    void DacProgramGammaRamp(
        _D3DGAMMARAMP *arg0);

    unsigned long GetRefreshRate(
        void);

    int IsOddField(
        void);
    int IsFlipPending(
        void);
/* Additional original-PDB method declarations. */
public:
    void DacProgramVideoStart(
        unsigned long arg0);
private:
    int MapRegisters(
        void);
private:
    void InitGammaRamp(
        unsigned long arg0);
private:
    unsigned long ReserveInstMem(
        unsigned long arg0);
private:
    void TmrDelay(
        unsigned long arg0);
/* Remaining methods decoded from the supplied January PDB. */
public:
    int InitHardware(
        void);
public:
    int InitDMAChannel(
        unsigned long arg0,
        D3D::CMiniport::OBJECTINFO *arg1,
        D3D::CMiniport::OBJECTINFO *arg2,
        unsigned long arg3,
        void **arg4);
public:
    int BindToChannel(
        D3D::CMiniport::OBJECTINFO *arg0);
public:
    int CreateGrObject(
        unsigned long arg0,
        unsigned long arg1,
        D3D::CMiniport::OBJECTINFO *arg2);
public:
    int CreateCtxDmaObject(
        unsigned long arg0,
        unsigned long arg1,
        void *arg2,
        unsigned long arg3,
        D3D::CMiniport::OBJECTINFO *arg4);
public:
    unsigned long SetDmaRange(
        unsigned long arg0,
        D3DSurface *arg1);
public:
    int CreateTile(
        unsigned long arg0,
        unsigned long arg1,
        unsigned long arg2,
        unsigned long arg3,
        unsigned long arg4,
        unsigned long arg5,
        unsigned long arg6);
public:
    int DestroyTile(
        unsigned long arg0,
        unsigned long arg1);
public:
    void ShutdownEngines(
        void);
public:
    void DisableInterrupts(
        void);
public:
    void EnableInterrupts(
        void);
public:
    static unsigned char WINAPI Isr(
        _KINTERRUPT *arg0,
        void *arg1);
public:
    static void WINAPI Dpc(
        _KDPC *arg0,
        void *arg1,
        void *arg2,
        void *arg3);
public:
    static void WINAPI ShutdownNotification(
        _HAL_SHUTDOWN_REGISTRATION *arg0);
public:
    void TilingUpdateIdle(
        unsigned long *arg0);
public:
    void FixupPushBuffer(
        D3D::CMiniport::PUSHBUFFERFIXUPINFO *arg0,
        unsigned long arg1);
private:
    int InitEngines(
        void);
private:
    int LoadEngines(
        void);
private:
    int GetGeneralInfo(
        void);
private:
    void SoftwareMethod(
        unsigned long arg0,
        unsigned long arg1);
private:
    void SetupPaletteAndGamma(
        void);
private:
    void GetAddressInfo(
        void *arg0,
        void **arg1,
        unsigned long *arg2,
        int arg3);
private:
    unsigned long ServiceGrInterrupt(
        void);
private:
    unsigned long VBlank(
        void);
private:
    unsigned long ServiceFifoInterrupt(
        void);
private:
    unsigned long ServiceMediaPortInterrupt(
        void);
private:
    unsigned long ServiceVideoInterrupt(
        void);
private:
    void HalMcControlInit(
        void);
private:
    void HalFbControlInit(
        void);
private:
    void HalVideoControlInit(
        void);
private:
    void HalMpControlInit(
        void);
private:
    void HalGrControlInit(
        void);
private:
    void HalGrControlLoad(
        void);
private:
    void HalGrIdle(
        void);
private:
    void HalGrLoadChannelContext(
        unsigned long arg0);
private:
    void HalGrUnloadChannelContext(
        unsigned long arg0);
private:
    void HalGrInitObjectContext(
        unsigned long arg0,
        unsigned long arg1);
private:
    void HalGrInit3d(
        void);
private:
    void HalFifoControlInit(
        void);
private:
    void HalFifoControlLoad(
        void);
private:
    void HalFifoContextSwitch(
        unsigned long arg0);
private:
    void HalFifoAllocDMA(
        unsigned long arg0,
        unsigned long arg1,
        unsigned long arg2,
        D3D::CMiniport::OBJECTINFO *arg3);
private:
    void HalFifoHashAdd(
        unsigned long arg0,
        unsigned long arg1,
        unsigned long arg2,
        unsigned long arg3,
        unsigned long arg4);
private:
    void HalDacControlInit(
        void);
private:
    void HalDacLoad(
        void);
private:
    void HalDacUnload(
        void);
private:
    void HalDacProgramMClk(
        void);
private:
    void HalDacProgramNVClk(
        void);
private:
    void HalDacProgramPClk(
        void);
private:
    void DumpClocks(
        void);
private:
    void GrDone(
        void);
private:
    unsigned char UnlockCRTC(
        void);
private:
    void RestoreCRTCLock(
        unsigned char arg0);
private:
    unsigned char ReadCRTCLock(
        void);
};
class CDevice : public D3DDevice
{
public:
    static DWORD m_PushBufferSize;
    static DWORD m_PushSegmentSize;
    _XMETAL_PushBuffer m_Pusher; /* +0x0 */
    unsigned long m_DirtyFlags; /* +0x8 */
    unsigned long m_StateFlags; /* +0xc */
    unsigned long *m_pPushBase; /* +0x10 */
    unsigned long *m_pPushLimit; /* +0x14 */
    unsigned long *m_pKickOff; /* +0x18 */
    unsigned long m_CpuTime; /* +0x1c */
    unsigned long m_PusherLastSegment; /* +0x20 */
    D3D::Fence m_PusherSegment[16]; /* +0x24 */
    D3D::Fence m_PusherFence[64]; /* +0xe4 */
    unsigned long m_PusherPutRunSize; /* +0x3e4 */
    unsigned long m_PusherLastSize; /* +0x3e8 */
    unsigned long m_LastRunPushBufferTime; /* +0x3ec */
    volatile unsigned long *m_pGpuTime; /* +0x3f0 */
    D3DPushBuffer *m_pPushBufferRecordResource; /* +0x3f4 */
    unsigned long m_PushBufferRecordWrapSize; /* +0x3f8 */
    unsigned long *m_pPushBufferRecordSavedThreshold; /* +0x3fc */
    unsigned long *m_pPushBufferRecordSavedPut; /* +0x400 */
    D3D::_HWREG *m_NvBase; /* +0x404 */
    unsigned long m_cRefs; /* +0x408 */
    D3DSurface *m_pRenderTarget; /* +0x40c */
    D3DSurface *m_pZBuffer; /* +0x410 */
    D3D::PixelShader *m_pPixelShader; /* +0x414 */
    unsigned long m_ShaderUsesSpecFog; /* +0x418 */
    unsigned long m_ShaderAdjustsTexMode; /* +0x41c */
    unsigned long m_PSShaderStageProgram; /* +0x420 */
    unsigned long m_pPixelShaderConstants[16]; /* +0x424 */
    D3D::PixelShader m_UserPixelShader; /* +0x464 */
    D3D::VertexShader *m_pVertexShader; /* +0x470 */
    unsigned long m_VertexShaderHandle; /* +0x474 */
    unsigned int m_IndexBase; /* +0x478 */
    D3DIndexBuffer *m_pIndexBuffer; /* +0x47c */
    D3D::Light *m_pLights; /* +0x480 */
    unsigned long m_LightCount; /* +0x484 */
    D3D::Light *m_pActiveLights; /* +0x488 */
    void *m_ReportAllocations[16]; /* +0x48c */
    unsigned long m_TextureControl0Enabled[4]; /* +0x4cc */
    unsigned long m_TextureCubemapAndDimension[4]; /* +0x4dc */
    float m_WNear; /* +0x4ec */
    float m_WFar; /* +0x4f0 */
    float m_InverseWFar; /* +0x4f4 */
    float m_ZScale; /* +0x4f8 */
    unsigned long m_TexGenInverseNeeded; /* +0x4fc */
    float m_SuperSampleScaleX; /* +0x500 */
    float m_SuperSampleScaleY; /* +0x504 */
    float m_SuperSampleScale; /* +0x508 */
    float m_SuperSampleLODBias; /* +0x50c */
    unsigned long m_AlphaICW[8]; /* +0x510 */
    unsigned long m_AlphaOCW[8]; /* +0x530 */
    unsigned long m_ColorICW[8]; /* +0x550 */
    unsigned long m_ColorOCW[8]; /* +0x570 */
    unsigned long m_CombinerFlags; /* +0x590 */
    unsigned long m_ActiveCombinerStagesCount; /* +0x594 */
    unsigned long m_CachedIndexBase; /* +0x598 */
    BYTE reserved_059c[4]; /* Original layout gap. */
    _D3DMATRIX m_ProjectionViewportTransform; /* +0x5a0 */
    _D3DMATRIX m_ModelViewTransform[4]; /* +0x5e0 */
    _D3DMATRIX m_ProjectionViewport; /* +0x6e0 */
    unsigned long m_VertexShaderInputHandle; /* +0x720 */
    unsigned long m_VertexShaderInputCount; /* +0x724 */
    _D3DSTREAM_INPUT m_VertexShaderInputStream[16]; /* +0x728 */
    unsigned long m_InlineVertexDwords; /* +0x7e8 */
    unsigned long m_InlineStartOffset; /* +0x7ec */
    unsigned long m_InlineDelta; /* +0x7f0 */
    D3D::InlineAttributeData m_InlineAttributeData[16]; /* +0x7f4 */
    unsigned long m_InlineAttributeCount; /* +0x874 */
    BYTE reserved_0878[8]; /* Original layout gap. */
    _D3DMATRIX m_Transform[10]; /* +0x880 */
    _D3DVIEWPORT8 m_Viewport; /* +0xb00 */
    _D3DMATERIAL8 m_Material; /* +0xb18 */
    _D3DMATERIAL8 m_BackMaterial; /* +0xb5c */
    D3DBaseTexture *m_Textures[4]; /* +0xba0 */
    D3DPalette *m_Palettes[4]; /* +0xbb0 */
    float m_PixelShaderConstants[16][4]; /* +0xbc0 */
    float m_VertexShaderConstants[192][4]; /* +0xcc0 */
    unsigned long m_VertexShaderProgramSlots[136][4]; /* +0x18c0 */
    unsigned long m_ConstantMode; /* +0x2140 */
    void *m_pFrameBufferBase; /* +0x2144 */
    void *m_pAntiAliasBufferBase; /* +0x2148 */
    unsigned long m_FrameBufferCount; /* +0x214c */
    D3DSurface m_FrameBufferSurfaces[3]; /* +0x2150 */
    void *m_pAutoDepthBuffer; /* +0x2198 */
    D3DSurface m_AutoDepthSurface; /* +0x219c */
    unsigned long m_DisplayWidth; /* +0x21b4 */
    unsigned long m_DisplayHeight; /* +0x21b8 */
    _D3DTILE m_Tile[8]; /* +0x21bc */
    _D3DRECT m_ScissorsRects[8]; /* +0x227c */
    unsigned long m_ScissorsCount; /* +0x22fc */
    int m_ScissorsExclusive; /* +0x2300 */
    volatile Nv206eControl *m_pControlDma; /* +0x2304 */
    D3D::CMiniport m_Miniport; /* +0x2308 */
    unsigned long m_PresentCount; /* +0x2b60 */
    unsigned long m_PresentationInterval; /* +0x2b64 */
    unsigned long m_dwOpcode; /* +0x2b68 */
    unsigned long m_dwSnapshot; /* +0x2b6c */
    unsigned long *m_pShaderCaptureBuffer; /* +0x2b70 */
    unsigned long *m_pShaderCapturePtr; /* +0x2b74 */
    unsigned long m_ColorContextDmaInstance; /* +0x2b78 */
    unsigned long m_ZetaContextDmaInstance; /* +0x2b7c */
    unsigned long m_CopyContextDmaInstance; /* +0x2b80 */
    unsigned char *m_pCachedContiguousMemoryBase; /* +0x2b84 */
    volatile NvNotification *m_pMemCopyNotifiers; /* +0x2b88 */
    volatile NvNotification *m_pKelvinNotifiers; /* +0x2b8c */
    long Init(
        _D3DPRESENT_PARAMETERS_ *arg0);
    void UnInit(
        void);
    long InitializeFrameBuffers(
        _D3DPRESENT_PARAMETERS_ *arg0);
    void FreeFrameBuffers(
        void);
    void SetStateUP(
        void);
    void SetStateVB(
        unsigned long arg0);
    unsigned long *StartPush(
        unsigned long arg0);
    unsigned long *StartPush(
        void);
    void StartBeginEndBracket(
        void);
    void EndBeginEndBracket(
        void);
    void EndPush(
        unsigned long *arg0);
    void KickOff(
        void);
    void InitializePushBuffer(
        void);
    unsigned long *MakeSpace(
        void);
    void HwPut(
        unsigned long *arg0);
    unsigned long *HwGet(
        void);
    unsigned long *GpuGet(
        void);
    unsigned long GpuTime(
        void);
    unsigned long Age(
        unsigned long arg0);
    int IsTimePending(
        unsigned long arg0);
    void RecordResourceReadPush(
        D3DResource *arg0);
    void RecordSurfaceWritePush(
        D3DResource *arg0);
};
extern CDevice *g_pDevice;
extern CDevice g_Device;
extern const D3DCAPS8 g_DeviceCaps;
extern const D3DADAPTER_IDENTIFIER8 g_AdapterIdentifier;
}
typedef char D3D_CDevice_size_check[sizeof(D3D::CDevice) == 11152 ? 1 : -1];
typedef char D3D_CDevice_m_Pusher_offset_check[offsetof(D3D::CDevice, m_Pusher) == 0x0 ? 1 : -1];
typedef char D3D_CDevice_m_DirtyFlags_offset_check[offsetof(D3D::CDevice, m_DirtyFlags) == 0x8 ? 1 : -1];
typedef char D3D_CDevice_m_StateFlags_offset_check[offsetof(D3D::CDevice, m_StateFlags) == 0xc ? 1 : -1];
typedef char D3D_CDevice_m_pPushBase_offset_check[offsetof(D3D::CDevice, m_pPushBase) == 0x10 ? 1 : -1];
typedef char D3D_CDevice_m_pPushLimit_offset_check[offsetof(D3D::CDevice, m_pPushLimit) == 0x14 ? 1 : -1];
typedef char D3D_CDevice_m_pKickOff_offset_check[offsetof(D3D::CDevice, m_pKickOff) == 0x18 ? 1 : -1];
typedef char D3D_CDevice_m_CpuTime_offset_check[offsetof(D3D::CDevice, m_CpuTime) == 0x1c ? 1 : -1];
typedef char D3D_CDevice_m_PusherLastSegment_offset_check[offsetof(D3D::CDevice, m_PusherLastSegment) == 0x20 ? 1 : -1];
typedef char D3D_CDevice_m_PusherSegment_offset_check[offsetof(D3D::CDevice, m_PusherSegment) == 0x24 ? 1 : -1];
typedef char D3D_CDevice_m_PusherFence_offset_check[offsetof(D3D::CDevice, m_PusherFence) == 0xe4 ? 1 : -1];
typedef char D3D_CDevice_m_PusherPutRunSize_offset_check[offsetof(D3D::CDevice, m_PusherPutRunSize) == 0x3e4 ? 1 : -1];
typedef char D3D_CDevice_m_PusherLastSize_offset_check[offsetof(D3D::CDevice, m_PusherLastSize) == 0x3e8 ? 1 : -1];
typedef char D3D_CDevice_m_LastRunPushBufferTime_offset_check[offsetof(D3D::CDevice, m_LastRunPushBufferTime) == 0x3ec ? 1 : -1];
typedef char D3D_CDevice_m_pGpuTime_offset_check[offsetof(D3D::CDevice, m_pGpuTime) == 0x3f0 ? 1 : -1];
typedef char D3D_CDevice_m_pPushBufferRecordResource_offset_check[offsetof(D3D::CDevice, m_pPushBufferRecordResource) == 0x3f4 ? 1 : -1];
typedef char D3D_CDevice_m_PushBufferRecordWrapSize_offset_check[offsetof(D3D::CDevice, m_PushBufferRecordWrapSize) == 0x3f8 ? 1 : -1];
typedef char D3D_CDevice_m_pPushBufferRecordSavedThreshold_offset_check[offsetof(D3D::CDevice, m_pPushBufferRecordSavedThreshold) == 0x3fc ? 1 : -1];
typedef char D3D_CDevice_m_pPushBufferRecordSavedPut_offset_check[offsetof(D3D::CDevice, m_pPushBufferRecordSavedPut) == 0x400 ? 1 : -1];
typedef char D3D_CDevice_m_NvBase_offset_check[offsetof(D3D::CDevice, m_NvBase) == 0x404 ? 1 : -1];
typedef char D3D_CDevice_m_cRefs_offset_check[offsetof(D3D::CDevice, m_cRefs) == 0x408 ? 1 : -1];
typedef char D3D_CDevice_m_pRenderTarget_offset_check[offsetof(D3D::CDevice, m_pRenderTarget) == 0x40c ? 1 : -1];
typedef char D3D_CDevice_m_pZBuffer_offset_check[offsetof(D3D::CDevice, m_pZBuffer) == 0x410 ? 1 : -1];
typedef char D3D_CDevice_m_pPixelShader_offset_check[offsetof(D3D::CDevice, m_pPixelShader) == 0x414 ? 1 : -1];
typedef char D3D_CDevice_m_ShaderUsesSpecFog_offset_check[offsetof(D3D::CDevice, m_ShaderUsesSpecFog) == 0x418 ? 1 : -1];
typedef char D3D_CDevice_m_ShaderAdjustsTexMode_offset_check[offsetof(D3D::CDevice, m_ShaderAdjustsTexMode) == 0x41c ? 1 : -1];
typedef char D3D_CDevice_m_PSShaderStageProgram_offset_check[offsetof(D3D::CDevice, m_PSShaderStageProgram) == 0x420 ? 1 : -1];
typedef char D3D_CDevice_m_pPixelShaderConstants_offset_check[offsetof(D3D::CDevice, m_pPixelShaderConstants) == 0x424 ? 1 : -1];
typedef char D3D_CDevice_m_UserPixelShader_offset_check[offsetof(D3D::CDevice, m_UserPixelShader) == 0x464 ? 1 : -1];
typedef char D3D_CDevice_m_pVertexShader_offset_check[offsetof(D3D::CDevice, m_pVertexShader) == 0x470 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderHandle_offset_check[offsetof(D3D::CDevice, m_VertexShaderHandle) == 0x474 ? 1 : -1];
typedef char D3D_CDevice_m_IndexBase_offset_check[offsetof(D3D::CDevice, m_IndexBase) == 0x478 ? 1 : -1];
typedef char D3D_CDevice_m_pIndexBuffer_offset_check[offsetof(D3D::CDevice, m_pIndexBuffer) == 0x47c ? 1 : -1];
typedef char D3D_CDevice_m_pLights_offset_check[offsetof(D3D::CDevice, m_pLights) == 0x480 ? 1 : -1];
typedef char D3D_CDevice_m_LightCount_offset_check[offsetof(D3D::CDevice, m_LightCount) == 0x484 ? 1 : -1];
typedef char D3D_CDevice_m_pActiveLights_offset_check[offsetof(D3D::CDevice, m_pActiveLights) == 0x488 ? 1 : -1];
typedef char D3D_CDevice_m_ReportAllocations_offset_check[offsetof(D3D::CDevice, m_ReportAllocations) == 0x48c ? 1 : -1];
typedef char D3D_CDevice_m_TextureControl0Enabled_offset_check[offsetof(D3D::CDevice, m_TextureControl0Enabled) == 0x4cc ? 1 : -1];
typedef char D3D_CDevice_m_TextureCubemapAndDimension_offset_check[offsetof(D3D::CDevice, m_TextureCubemapAndDimension) == 0x4dc ? 1 : -1];
typedef char D3D_CDevice_m_WNear_offset_check[offsetof(D3D::CDevice, m_WNear) == 0x4ec ? 1 : -1];
typedef char D3D_CDevice_m_WFar_offset_check[offsetof(D3D::CDevice, m_WFar) == 0x4f0 ? 1 : -1];
typedef char D3D_CDevice_m_InverseWFar_offset_check[offsetof(D3D::CDevice, m_InverseWFar) == 0x4f4 ? 1 : -1];
typedef char D3D_CDevice_m_ZScale_offset_check[offsetof(D3D::CDevice, m_ZScale) == 0x4f8 ? 1 : -1];
typedef char D3D_CDevice_m_TexGenInverseNeeded_offset_check[offsetof(D3D::CDevice, m_TexGenInverseNeeded) == 0x4fc ? 1 : -1];
typedef char D3D_CDevice_m_SuperSampleScaleX_offset_check[offsetof(D3D::CDevice, m_SuperSampleScaleX) == 0x500 ? 1 : -1];
typedef char D3D_CDevice_m_SuperSampleScaleY_offset_check[offsetof(D3D::CDevice, m_SuperSampleScaleY) == 0x504 ? 1 : -1];
typedef char D3D_CDevice_m_SuperSampleScale_offset_check[offsetof(D3D::CDevice, m_SuperSampleScale) == 0x508 ? 1 : -1];
typedef char D3D_CDevice_m_SuperSampleLODBias_offset_check[offsetof(D3D::CDevice, m_SuperSampleLODBias) == 0x50c ? 1 : -1];
typedef char D3D_CDevice_m_AlphaICW_offset_check[offsetof(D3D::CDevice, m_AlphaICW) == 0x510 ? 1 : -1];
typedef char D3D_CDevice_m_AlphaOCW_offset_check[offsetof(D3D::CDevice, m_AlphaOCW) == 0x530 ? 1 : -1];
typedef char D3D_CDevice_m_ColorICW_offset_check[offsetof(D3D::CDevice, m_ColorICW) == 0x550 ? 1 : -1];
typedef char D3D_CDevice_m_ColorOCW_offset_check[offsetof(D3D::CDevice, m_ColorOCW) == 0x570 ? 1 : -1];
typedef char D3D_CDevice_m_CombinerFlags_offset_check[offsetof(D3D::CDevice, m_CombinerFlags) == 0x590 ? 1 : -1];
typedef char D3D_CDevice_m_ActiveCombinerStagesCount_offset_check[offsetof(D3D::CDevice, m_ActiveCombinerStagesCount) == 0x594 ? 1 : -1];
typedef char D3D_CDevice_m_CachedIndexBase_offset_check[offsetof(D3D::CDevice, m_CachedIndexBase) == 0x598 ? 1 : -1];
typedef char D3D_CDevice_m_ProjectionViewportTransform_offset_check[offsetof(D3D::CDevice, m_ProjectionViewportTransform) == 0x5a0 ? 1 : -1];
typedef char D3D_CDevice_m_ModelViewTransform_offset_check[offsetof(D3D::CDevice, m_ModelViewTransform) == 0x5e0 ? 1 : -1];
typedef char D3D_CDevice_m_ProjectionViewport_offset_check[offsetof(D3D::CDevice, m_ProjectionViewport) == 0x6e0 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderInputHandle_offset_check[offsetof(D3D::CDevice, m_VertexShaderInputHandle) == 0x720 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderInputCount_offset_check[offsetof(D3D::CDevice, m_VertexShaderInputCount) == 0x724 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderInputStream_offset_check[offsetof(D3D::CDevice, m_VertexShaderInputStream) == 0x728 ? 1 : -1];
typedef char D3D_CDevice_m_InlineVertexDwords_offset_check[offsetof(D3D::CDevice, m_InlineVertexDwords) == 0x7e8 ? 1 : -1];
typedef char D3D_CDevice_m_InlineStartOffset_offset_check[offsetof(D3D::CDevice, m_InlineStartOffset) == 0x7ec ? 1 : -1];
typedef char D3D_CDevice_m_InlineDelta_offset_check[offsetof(D3D::CDevice, m_InlineDelta) == 0x7f0 ? 1 : -1];
typedef char D3D_CDevice_m_InlineAttributeData_offset_check[offsetof(D3D::CDevice, m_InlineAttributeData) == 0x7f4 ? 1 : -1];
typedef char D3D_CDevice_m_InlineAttributeCount_offset_check[offsetof(D3D::CDevice, m_InlineAttributeCount) == 0x874 ? 1 : -1];
typedef char D3D_CDevice_m_Transform_offset_check[offsetof(D3D::CDevice, m_Transform) == 0x880 ? 1 : -1];
typedef char D3D_CDevice_m_Viewport_offset_check[offsetof(D3D::CDevice, m_Viewport) == 0xb00 ? 1 : -1];
typedef char D3D_CDevice_m_Material_offset_check[offsetof(D3D::CDevice, m_Material) == 0xb18 ? 1 : -1];
typedef char D3D_CDevice_m_BackMaterial_offset_check[offsetof(D3D::CDevice, m_BackMaterial) == 0xb5c ? 1 : -1];
typedef char D3D_CDevice_m_Textures_offset_check[offsetof(D3D::CDevice, m_Textures) == 0xba0 ? 1 : -1];
typedef char D3D_CDevice_m_Palettes_offset_check[offsetof(D3D::CDevice, m_Palettes) == 0xbb0 ? 1 : -1];
typedef char D3D_CDevice_m_PixelShaderConstants_offset_check[offsetof(D3D::CDevice, m_PixelShaderConstants) == 0xbc0 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderConstants_offset_check[offsetof(D3D::CDevice, m_VertexShaderConstants) == 0xcc0 ? 1 : -1];
typedef char D3D_CDevice_m_VertexShaderProgramSlots_offset_check[offsetof(D3D::CDevice, m_VertexShaderProgramSlots) == 0x18c0 ? 1 : -1];
typedef char D3D_CDevice_m_ConstantMode_offset_check[offsetof(D3D::CDevice, m_ConstantMode) == 0x2140 ? 1 : -1];
typedef char D3D_CDevice_m_pFrameBufferBase_offset_check[offsetof(D3D::CDevice, m_pFrameBufferBase) == 0x2144 ? 1 : -1];
typedef char D3D_CDevice_m_pAntiAliasBufferBase_offset_check[offsetof(D3D::CDevice, m_pAntiAliasBufferBase) == 0x2148 ? 1 : -1];
typedef char D3D_CDevice_m_FrameBufferCount_offset_check[offsetof(D3D::CDevice, m_FrameBufferCount) == 0x214c ? 1 : -1];
typedef char D3D_CDevice_m_FrameBufferSurfaces_offset_check[offsetof(D3D::CDevice, m_FrameBufferSurfaces) == 0x2150 ? 1 : -1];
typedef char D3D_CDevice_m_pAutoDepthBuffer_offset_check[offsetof(D3D::CDevice, m_pAutoDepthBuffer) == 0x2198 ? 1 : -1];
typedef char D3D_CDevice_m_AutoDepthSurface_offset_check[offsetof(D3D::CDevice, m_AutoDepthSurface) == 0x219c ? 1 : -1];
typedef char D3D_CDevice_m_DisplayWidth_offset_check[offsetof(D3D::CDevice, m_DisplayWidth) == 0x21b4 ? 1 : -1];
typedef char D3D_CDevice_m_DisplayHeight_offset_check[offsetof(D3D::CDevice, m_DisplayHeight) == 0x21b8 ? 1 : -1];
typedef char D3D_CDevice_m_Tile_offset_check[offsetof(D3D::CDevice, m_Tile) == 0x21bc ? 1 : -1];
typedef char D3D_CDevice_m_ScissorsRects_offset_check[offsetof(D3D::CDevice, m_ScissorsRects) == 0x227c ? 1 : -1];
typedef char D3D_CDevice_m_ScissorsCount_offset_check[offsetof(D3D::CDevice, m_ScissorsCount) == 0x22fc ? 1 : -1];
typedef char D3D_CDevice_m_ScissorsExclusive_offset_check[offsetof(D3D::CDevice, m_ScissorsExclusive) == 0x2300 ? 1 : -1];
typedef char D3D_CDevice_m_pControlDma_offset_check[offsetof(D3D::CDevice, m_pControlDma) == 0x2304 ? 1 : -1];
typedef char D3D_CDevice_m_Miniport_offset_check[offsetof(D3D::CDevice, m_Miniport) == 0x2308 ? 1 : -1];
typedef char D3D_CDevice_m_PresentCount_offset_check[offsetof(D3D::CDevice, m_PresentCount) == 0x2b60 ? 1 : -1];
typedef char D3D_CDevice_m_PresentationInterval_offset_check[offsetof(D3D::CDevice, m_PresentationInterval) == 0x2b64 ? 1 : -1];
typedef char D3D_CDevice_m_dwOpcode_offset_check[offsetof(D3D::CDevice, m_dwOpcode) == 0x2b68 ? 1 : -1];
typedef char D3D_CDevice_m_dwSnapshot_offset_check[offsetof(D3D::CDevice, m_dwSnapshot) == 0x2b6c ? 1 : -1];
typedef char D3D_CDevice_m_pShaderCaptureBuffer_offset_check[offsetof(D3D::CDevice, m_pShaderCaptureBuffer) == 0x2b70 ? 1 : -1];
typedef char D3D_CDevice_m_pShaderCapturePtr_offset_check[offsetof(D3D::CDevice, m_pShaderCapturePtr) == 0x2b74 ? 1 : -1];
typedef char D3D_CDevice_m_ColorContextDmaInstance_offset_check[offsetof(D3D::CDevice, m_ColorContextDmaInstance) == 0x2b78 ? 1 : -1];
typedef char D3D_CDevice_m_ZetaContextDmaInstance_offset_check[offsetof(D3D::CDevice, m_ZetaContextDmaInstance) == 0x2b7c ? 1 : -1];
typedef char D3D_CDevice_m_CopyContextDmaInstance_offset_check[offsetof(D3D::CDevice, m_CopyContextDmaInstance) == 0x2b80 ? 1 : -1];
typedef char D3D_CDevice_m_pCachedContiguousMemoryBase_offset_check[offsetof(D3D::CDevice, m_pCachedContiguousMemoryBase) == 0x2b84 ? 1 : -1];
typedef char D3D_CDevice_m_pMemCopyNotifiers_offset_check[offsetof(D3D::CDevice, m_pMemCopyNotifiers) == 0x2b88 ? 1 : -1];
typedef char D3D_CDevice_m_pKelvinNotifiers_offset_check[offsetof(D3D::CDevice, m_pKelvinNotifiers) == 0x2b8c ? 1 : -1];
typedef char D3D_CMiniport_size_check[sizeof(D3D::CMiniport) == 2136 ? 1 : -1];
typedef char D3D_CMiniport_m_RegisterBase_offset_check[offsetof(D3D::CMiniport, m_RegisterBase) == 0x0 ? 1 : -1];
typedef char D3D_CMiniport_m_PresentationInterval_offset_check[offsetof(D3D::CMiniport, m_PresentationInterval) == 0x4 ? 1 : -1];
typedef char D3D_CMiniport_m_SurfacePitch_offset_check[offsetof(D3D::CMiniport, m_SurfacePitch) == 0x8 ? 1 : -1];
typedef char D3D_CMiniport_m_DisplayMode_offset_check[offsetof(D3D::CMiniport, m_DisplayMode) == 0xc ? 1 : -1];
typedef char D3D_CMiniport_m_Format_offset_check[offsetof(D3D::CMiniport, m_Format) == 0x10 ? 1 : -1];
typedef char D3D_CMiniport_m_InstMem_offset_check[offsetof(D3D::CMiniport, m_InstMem) == 0x14 ? 1 : -1];
typedef char D3D_CMiniport_m_InterruptsEnabled_offset_check[offsetof(D3D::CMiniport, m_InterruptsEnabled) == 0xa4 ? 1 : -1];
typedef char D3D_CMiniport_m_GenInfo_offset_check[offsetof(D3D::CMiniport, m_GenInfo) == 0xa8 ? 1 : -1];
typedef char D3D_CMiniport_m_DacInfo_offset_check[offsetof(D3D::CMiniport, m_DacInfo) == 0xc4 ? 1 : -1];
typedef char D3D_CMiniport_m_HalInfo_offset_check[offsetof(D3D::CMiniport, m_HalInfo) == 0x100 ? 1 : -1];
typedef char D3D_CMiniport_m_VideoModeTiming_offset_check[offsetof(D3D::CMiniport, m_VideoModeTiming) == 0x15c ? 1 : -1];
typedef char D3D_CMiniport_m_VideoModeDepth_offset_check[offsetof(D3D::CMiniport, m_VideoModeDepth) == 0x19c ? 1 : -1];
typedef char D3D_CMiniport_m_FreeInstAddr_offset_check[offsetof(D3D::CMiniport, m_FreeInstAddr) == 0x1a0 ? 1 : -1];
typedef char D3D_CMiniport_m_VBlankFlips_offset_check[offsetof(D3D::CMiniport, m_VBlankFlips) == 0x1b4 ? 1 : -1];
typedef char D3D_CMiniport_m_VBlankFlipNext_offset_check[offsetof(D3D::CMiniport, m_VBlankFlipNext) == 0x1dc ? 1 : -1];
typedef char D3D_CMiniport_m_VBlankQueueNext_offset_check[offsetof(D3D::CMiniport, m_VBlankQueueNext) == 0x1e0 ? 1 : -1];
typedef char D3D_CMiniport_m_pVerticalBlankCallback_offset_check[offsetof(D3D::CMiniport, m_pVerticalBlankCallback) == 0x1e4 ? 1 : -1];
typedef char D3D_CMiniport_m_CurrentAvInfo_offset_check[offsetof(D3D::CMiniport, m_CurrentAvInfo) == 0x208 ? 1 : -1];
typedef char D3D_CMiniport_m_FirstFlip_offset_check[offsetof(D3D::CMiniport, m_FirstFlip) == 0x20c ? 1 : -1];
typedef char D3D_CMiniport_m_VBlankFlipCount_offset_check[offsetof(D3D::CMiniport, m_VBlankFlipCount) == 0x210 ? 1 : -1];
typedef char D3D_CMiniport_m_GammaRamp_offset_check[offsetof(D3D::CMiniport, m_GammaRamp) == 0x214 ? 1 : -1];
typedef char D3D_CMiniport_m_GammaUpdated_offset_check[offsetof(D3D::CMiniport, m_GammaUpdated) == 0x814 ? 1 : -1];
typedef char D3D_CMiniport_m_GammaCurrentIndex_offset_check[offsetof(D3D::CMiniport, m_GammaCurrentIndex) == 0x81c ? 1 : -1];
typedef char D3D_CMiniport_m_VBlankCount_offset_check[offsetof(D3D::CMiniport, m_VBlankCount) == 0x820 ? 1 : -1];
typedef char D3D_CMiniport_m_OverlayVBlank_offset_check[offsetof(D3D::CMiniport, m_OverlayVBlank) == 0x824 ? 1 : -1];
typedef char D3D_CMiniport_m_DebugRegister_offset_check[offsetof(D3D::CMiniport, m_DebugRegister) == 0x828 ? 1 : -1];
typedef char D3D_CMiniport_m_PusherGetRunSize_offset_check[offsetof(D3D::CMiniport, m_PusherGetRunSize) == 0x854 ? 1 : -1];
typedef char D3D_Light_size_check[sizeof(D3D::Light) == 144 ? 1 : -1];
typedef char D3D_Light_Light8_offset_check[offsetof(D3D::Light, Light8) == 0x0 ? 1 : -1];
typedef char D3D_Light_Flags_offset_check[offsetof(D3D::Light, Flags) == 0x68 ? 1 : -1];
typedef char D3D_Light_Direction_offset_check[offsetof(D3D::Light, Direction) == 0x6c ? 1 : -1];
typedef char D3D_Light_Falloff_L_offset_check[offsetof(D3D::Light, Falloff_L) == 0x78 ? 1 : -1];
typedef char D3D_Light_Falloff_M_offset_check[offsetof(D3D::Light, Falloff_M) == 0x7c ? 1 : -1];
typedef char D3D_Light_Falloff_N_offset_check[offsetof(D3D::Light, Falloff_N) == 0x80 ? 1 : -1];
typedef char D3D_Light_Scale_offset_check[offsetof(D3D::Light, Scale) == 0x84 ? 1 : -1];
typedef char D3D_Light_W_offset_check[offsetof(D3D::Light, W) == 0x88 ? 1 : -1];
typedef char D3D_Light_pNext_offset_check[offsetof(D3D::Light, pNext) == 0x8c ? 1 : -1];
#endif
