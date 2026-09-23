#ifndef HALO_D3D8_MINIPORT_CONSTANTS_H
#define HALO_D3D8_MINIPORT_CONSTANTS_H
namespace D3D
{
/* NV2A object classes and memory-space values used by the target. Values are
 * checked against the supplied binary; names follow the hardware interfaces. */
/* PLL register offsets and lock bits, as accessed by the January miniport. */
enum PllRegister
{
    NV_PRAMDAC_NVPLL_COEFF = 0x680500,
    NV_PRAMDAC_MPLL_COEFF = 0x680504,
    NV_PRAMDAC_VPLL_COEFF = 0x680508,
    NV_PRAMDAC_PLL_TEST_COUNTER = 0x680514,
    NV_PFB_EMRS = 0x1002c4,
    NV_PFB_MRS = 0x1002c0,
    NV_PFB_TIMING2 = 0x100228,
    NV_PBUS_DEBUG_3 = 0x108c
};
enum AgpClockRate
{
    AGP_CLOCK_66_MHZ = 66000000,
    AGP_CLOCK_73_MHZ = 73000000,
    AGP_CLOCK_80_MHZ = 80000000,
    AGP_CLOCK_87_MHZ = 87000000,
    AGP_CLOCK_94_MHZ = 94000000,
    AGP_CLOCK_100_MHZ = 100000000,
    AGP_CLOCK_106_MHZ = 106000000,
    AGP_CLOCK_113_MHZ = 113000000,
    AGP_CLOCK_120_MHZ = 120000000,
    AGP_CLOCK_126_MHZ = 126000000,
    AGP_CLOCK_133_MHZ = 133000000
};
/* January software-method values; names describe the original dispatch.
 * The later SDK's packed 5-bit IDs are not interchangeable with these IDs. */
enum MiniportSoftwareMethod
{
    SoftwareFlipImmediate = 0x300,
    SoftwareFlipQueued = 0x304,
    SoftwareRunPushBuffer = 0x308,
    SoftwareFixupPushBuffer = 0x30c,
    SoftwareFence = 0x310,
    SoftwareReadCallback = 0x314,
    SoftwareWriteCallback = 0x318,
    SoftwareDxt1NoiseEnable = 0x31c,
    SoftwareWriteRegister = 0x320,
    SoftwareSetSurfacePitchAndOffset = 0x324
};
enum DmaObjectClass
{
    NV01_CONTEXT_DMA_FROM_MEMORY = 0x02,
    NV01_CONTEXT_DMA_TO_MEMORY = 0x03,
    NV01_CONTEXT_DMA_IN_MEMORY = 0x3d
};
enum GraphicsObjectClass
{
    NV03_NULL = 0x30,
    NV03_MEMORY_TO_MEMORY_FORMAT = 0x39,
    NV10_CONTEXT_SURFACES_2D = 0x62,
    NV20_KELVIN_PRIMITIVE = 0x97,
    NV12_IMAGE_BLIT = 0x9f
};
enum DmaAddressSpace
{
    ADDR_SYSMEM = 1,
    ADDR_FBMEM = 2,
    ADDR_AGPMEM = 3
};
}
#endif
