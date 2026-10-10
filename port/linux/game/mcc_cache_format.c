/* Halo 1 MCC cache reader. All file integers are explicitly little-endian.
 * Public format references and observed-fixture details: docs/mcc_maps.md. */
#include "mcc_cache_format.h"
#include <stdlib.h>
#include <string.h>

#define MCC_FOURCC(a,b,c,d) (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(uint32_t)(d))
#define MCC_SCENARIO MCC_FOURCC('s','c','n','r')
#define MCC_BSP MCC_FOURCC('s','b','s','p')

static uint16_t mcc_u16(void const *p) {
    uint8_t const *b = (uint8_t const *)p;
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}
static uint32_t mcc_u32(void const *p) {
    uint8_t const *b = (uint8_t const *)p;
    return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
static int mcc_range(uint64_t offset, uint64_t bytes, uint64_t limit) {
    return offset <= limit && bytes <= limit - offset;
}
static enum mcc_cache_status mcc_fail(struct mcc_cache_report *r,
    enum mcc_cache_status status, uint32_t location) {
    r->status = status;
    r->problem_location = location;
    return status;
}

enum mcc_cache_status mcc_cache_identify(void const *header, size_t header_bytes,
    uint64_t file_size, struct mcc_cache_identity *out) {
    uint8_t const *h = (uint8_t const *)header;
    struct mcc_cache_identity v;
    if (!h || !out) return MCC_CACHE_BAD_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (header_bytes < MCC_CACHE_HEADER_BYTES) return MCC_CACHE_BAD_HEADER;
    if (mcc_u32(h) != MCC_FOURCC('h','e','a','d') ||
        mcc_u32(h+0x7FC) != MCC_FOURCC('f','o','o','t')) return MCC_CACHE_BAD_HEADER;
    if (mcc_u32(h+4) != MCC_CACHE_VERSION) return MCC_CACHE_WRONG_VERSION;
    if (file_size < MCC_CACHE_HEADER_BYTES || file_size > 0x7FFFFFFFu) return MCC_CACHE_BAD_FILE_SIZE;
    memset(&v, 0, sizeof(v));
    v.file_size = file_size;
    v.file_length = mcc_u32(h+8);
    if (v.file_length && (v.file_length < MCC_CACHE_HEADER_BYTES || v.file_length > file_size))
        return MCC_CACHE_BAD_FILE_SIZE;
    if (!v.file_length) v.file_length = (uint32_t)file_size;
    if (!memchr(h+0x20, 0, 32) || !memchr(h+0x40, 0, 32)) return MCC_CACHE_BAD_STRING;
    memcpy(v.name, h+0x20, 32);
    memcpy(v.build, h+0x40, 32);
    v.scenario_type = mcc_u16(h+0x60);
    if (v.scenario_type > 1) return MCC_CACHE_BAD_SCENARIO_TYPE;
    v.flags = mcc_u16(h+0x68);
    v.checksum = mcc_u32(h+0x64);
    v.tag_data_offset = mcc_u32(h+0x10);
    v.tag_data_size = mcc_u32(h+0x14);
    if (v.tag_data_offset < MCC_CACHE_HEADER_BYTES || v.tag_data_size < 0x28 ||
        v.tag_data_size > MCC_CACHE_TAG_CAPACITY ||
        !mcc_range(v.tag_data_offset, v.tag_data_size, v.file_length)) return MCC_CACHE_BAD_TAG_RANGE;
    *out = v;
    return MCC_CACHE_OK;
}

void *mcc_cache_pointer(void *data, struct mcc_cache_report const *r,
    uint32_t address, uint32_t bytes) {
    if (!data || !r || address < r->tag_base ||
        !mcc_range(address-r->tag_base, bytes, r->identity.tag_data_size)) return NULL;
    return (uint8_t *)data + (address-r->tag_base);
}

static void *mcc_block(void *data, struct mcc_cache_report const *r,
    uint8_t const *block, uint32_t stride) {
    uint32_t count = mcc_u32(block);
    if (count > UINT32_MAX / stride) return NULL;
    if (!count) return data;
    return mcc_cache_pointer(data, r, mcc_u32(block+4), count*stride);
}

static uint8_t *mcc_bsp_pointer(uint8_t *data, struct mcc_cache_bsp const *b,
    uint32_t address, uint32_t bytes) {
    if (address < b->address || !mcc_range(address-b->address, bytes, b->size)) return NULL;
    return data + (address-b->address);
}

static enum mcc_cache_status mcc_check_bsp(struct mcc_cache_source const *source,
    struct mcc_cache_report *r, struct mcc_cache_bsp *b) {
    uint8_t *data, *structure, *lightmaps;
    uint32_t i, count;
    enum mcc_cache_status status = MCC_CACHE_BAD_BSP;
    if (b->size < 0x18 || b->size > MCC_CACHE_TAG_CAPACITY || b->file_offset < MCC_CACHE_HEADER_BYTES ||
        !mcc_range(b->file_offset, b->size, r->identity.file_length) ||
        b->address < r->tag_base ||
        !mcc_range(b->address-r->tag_base, b->size, MCC_CACHE_TAG_CAPACITY) ||
        b->address-r->tag_base < r->identity.tag_data_size) return MCC_CACHE_BAD_BSP;
    data = (uint8_t *)malloc(b->size);
    if (!data) return MCC_CACHE_NO_MEMORY;
    if (!source->read(source->context,b->file_offset,b->size,data)) {
        status = MCC_CACHE_READ_FAILED;
        goto done;
    }
    b->structure_address = mcc_u32(data);
    /* Version 13 stores uncompressed environment/lightmap vertices in a
     * separate file section, named by the first pair after the BSP pointer. */
    b->vertex_bytes = mcc_u32(data+4);
    b->vertex_file_offset = mcc_u32(data+8);
    if (mcc_u32(data+0x14) != MCC_BSP ||
        !mcc_range(b->vertex_file_offset,b->vertex_bytes,r->identity.file_length)) goto done;
    structure = mcc_bsp_pointer(data,b,b->structure_address,0x288);
    if (!structure) goto done;
    count = mcc_u32(structure+0x104);
    if (count > b->size/0x20) goto done;
    lightmaps = mcc_bsp_pointer(data,b,mcc_u32(structure+0x108),count*0x20);
    if (count && !lightmaps) goto done;
    for (i=0;i<count;i++) {
        uint8_t *lm = lightmaps+i*0x20, *materials;
        uint32_t j, material_count = mcc_u32(lm+0x14);
        if (material_count > b->size/0x100) goto done;
        materials = mcc_bsp_pointer(data,b,mcc_u32(lm+0x18),material_count*0x100);
        if (material_count && !materials) goto done;
        for (j=0;j<material_count;j++) {
            uint8_t const *m = materials+j*0x100;
            uint32_t rendered = mcc_u32(m+0xB4), lit = mcc_u32(m+0xC8);
            if (!mcc_range(mcc_u32(m+0xB8),(uint64_t)rendered*56,b->vertex_bytes) ||
                !mcc_range(mcc_u32(m+0xCC),(uint64_t)lit*20,b->vertex_bytes) ||
                (lit && lit != rendered)) goto done;
            r->bsp_material_count++;
        }
    }
    status = MCC_CACHE_OK;
done:
    free(data);
    return status;
}

static enum mcc_cache_status mcc_check_assets(void *data, struct mcc_cache_report *r,
    uint32_t group, uint32_t address) {
    uint8_t *tag;
    uint32_t i, count;
    if (group == MCC_FOURCC('b','i','t','m')) {
        uint8_t *bitmaps;
        tag = mcc_cache_pointer(data,r,address,0x6C);
        if (!tag) return MCC_CACHE_BAD_BITMAP;
        count = mcc_u32(tag+0x60);
        bitmaps = mcc_block(data,r,tag+0x60,0x30);
        if (!bitmaps) return MCC_CACHE_BAD_BITMAP;
        for (i=0;i<count;i++) {
            uint8_t *b = bitmaps+i*0x30;
            uint16_t format = mcc_u16(b+0xC), flags = mcc_u16(b+0xE);
            if (!mcc_u16(b+4) || !mcc_u16(b+6) || !mcc_u16(b+8) || format>18)
                return MCC_CACHE_BAD_BITMAP;
            if (flags & 0x100) r->external_bitmap_count++;
            else if (!mcc_range(mcc_u32(b+0x18),mcc_u32(b+0x1C),r->identity.file_length))
                return MCC_CACHE_BAD_BITMAP;
            r->bitmap_count++;
            if (format==18) r->bc7_bitmap_count++;
        }
    }
    if (group == MCC_FOURCC('s','n','d','!')) {
        uint8_t *ranges;
        tag = mcc_cache_pointer(data,r,address,0xA4);
        if (!tag) return MCC_CACHE_BAD_SOUND;
        count = mcc_u32(tag+0x98);
        ranges = mcc_block(data,r,tag+0x98,0x48);
        if (!ranges) return MCC_CACHE_BAD_SOUND;
        for (i=0;i<count;i++) {
            uint8_t *pr = ranges+i*0x48, *permutations;
            uint32_t j, n = mcc_u32(pr+0x3C);
            permutations = mcc_block(data,r,pr+0x3C,0x7C);
            if (!permutations) return MCC_CACHE_BAD_SOUND;
            for (j=0;j<n;j++) {
                uint8_t *p = permutations+j*0x7C;
                /* tag_data flags bit zero selects the external sounds map. */
                if (mcc_u32(p+0x44)&1) r->external_sound_count++;
                else if (!mcc_range(mcc_u32(p+0x48),mcc_u32(p+0x40),r->identity.file_length))
                    return MCC_CACHE_BAD_SOUND;
                r->sound_permutation_count++;
            }
        }
    }
    return MCC_CACHE_OK;
}

enum mcc_cache_status mcc_cache_load(struct mcc_cache_source const *source,
    void *data, size_t capacity, struct mcc_cache_report *r) {
    uint8_t header[MCC_CACHE_HEADER_BYTES], *tags = (uint8_t *)data, *scenario, *bsp_refs;
    uint32_t i, index_address;
    enum mcc_cache_status status;
    if (!source || !source->read || !data || !r) return MCC_CACHE_BAD_ARGUMENT;
    memset(r,0,sizeof(*r));
    r->problem_tag = UINT32_MAX;
    if (source->size<MCC_CACHE_HEADER_BYTES || !source->read(source->context,0,sizeof(header),header))
        return mcc_fail(r,MCC_CACHE_READ_FAILED,0);
    status = mcc_cache_identify(header,sizeof(header),source->size,&r->identity);
    if (status) return mcc_fail(r,status,0);
    if (capacity<r->identity.tag_data_size) return mcc_fail(r,MCC_CACHE_NO_MEMORY,0);
    if (!source->read(source->context,r->identity.tag_data_offset,r->identity.tag_data_size,data))
        return mcc_fail(r,MCC_CACHE_READ_FAILED,r->identity.tag_data_offset);
    index_address = mcc_u32(tags);
    if (index_address<0x28 || (index_address&3) || mcc_u32(tags+0x24)!=MCC_FOURCC('t','a','g','s'))
        return mcc_fail(r,MCC_CACHE_BAD_TAG_INDEX,r->identity.tag_data_offset);
    r->tag_base = index_address-0x28;
    r->tag_count = mcc_u32(tags+0xC);
    r->scenario_handle = mcc_u32(tags+4);
    if (!r->tag_count || r->tag_count>65535 ||
        !mcc_range(0x28,(uint64_t)r->tag_count*0x20,r->identity.tag_data_size) ||
        !mcc_range(r->tag_base,MCC_CACHE_TAG_CAPACITY,UINT64_C(0x100000000)) ||
        (r->scenario_handle&0xFFFF)>=r->tag_count)
        return mcc_fail(r,MCC_CACHE_BAD_TAG_INDEX,index_address);
    r->model_file_offset=mcc_u32(tags+0x14);
    r->model_vertex_bytes=mcc_u32(tags+0x1C);
    r->model_bytes=mcc_u32(tags+0x20);
    if (r->model_vertex_bytes>r->model_bytes ||
        !mcc_range(r->model_file_offset,r->model_bytes,r->identity.file_length))
        return mcc_fail(r,MCC_CACHE_BAD_MODEL_RANGE,r->model_file_offset);
    for (i=0;i<r->tag_count;i++) {
        uint8_t *entry=tags+0x28+i*0x20;
        uint32_t group=mcc_u32(entry), address=mcc_u32(entry+0x14), name_address=mcc_u32(entry+0x10);
        char *name=mcc_cache_pointer(data,r,name_address,1);
        r->problem_tag=i;
        if ((mcc_u32(entry+0xC)&0xFFFF)!=i || !name ||
            !memchr(name,0,r->identity.tag_data_size-(name_address-r->tag_base)))
            return mcc_fail(r,MCC_CACHE_BAD_TAG,index_address+i*0x20);
        if (mcc_u32(entry+0x18)) return mcc_fail(r,MCC_CACHE_EXTERNAL_TAG,index_address+i*0x20);
        if (group!=MCC_BSP && !mcc_cache_pointer(data,r,address,1))
            return mcc_fail(r,MCC_CACHE_BAD_TAG,address);
        if (group==MCC_FOURCC('m','o','d','2')) r->model_count++;
        if (group==MCC_FOURCC('s','o','t','r')) r->generic_shader_count++;
        status=mcc_check_assets(data,r,group,address);
        if (status) return mcc_fail(r,status,address);
    }
    r->problem_tag=r->scenario_handle&0xFFFF;
    {
        uint8_t *entry=tags+0x28+r->problem_tag*0x20;
        if (mcc_u32(entry)!=MCC_SCENARIO || mcc_u32(entry+0xC)!=r->scenario_handle)
            return mcc_fail(r,MCC_CACHE_BAD_SCENARIO,index_address+r->problem_tag*0x20);
        scenario=mcc_cache_pointer(data,r,mcc_u32(entry+0x14),0x5B0);
        if (!scenario || mcc_u16(scenario+0x3C)!=r->identity.scenario_type)
            return mcc_fail(r,MCC_CACHE_BAD_SCENARIO,mcc_u32(entry+0x14));
    }
    r->script_count=mcc_u32(scenario+0x49C);
    r->global_count=mcc_u32(scenario+0x4A8);
    {
        uint32_t syntax_size=mcc_u32(scenario+0x474);
        uint8_t *syntax=mcc_cache_pointer(data,r,mcc_u32(scenario+0x480),syntax_size);
        uint8_t *scripts=mcc_block(data,r,scenario+0x49C,0x5C);
        if ((syntax_size && (!syntax || syntax_size<0x38 || (syntax_size-0x38)%0x14)) || !scripts ||
            !mcc_block(data,r,scenario+0x4A8,0x5C)) return mcc_fail(r,MCC_CACHE_BAD_SCRIPT,0);
        if (syntax_size) r->script_node_count=(syntax_size-0x38)/0x14;
        for (i=0;i<r->script_count;i++) {
            if (mcc_u32(scripts+i*0x5C+0x50)) r->parameterized_script_count++;
        }
    }
    r->bsp_count=mcc_u32(scenario+0x5A4);
    bsp_refs=mcc_block(data,r,scenario+0x5A4,0x20);
    if (!r->bsp_count || r->bsp_count>MCC_CACHE_MAX_BSPS || !bsp_refs)
        return mcc_fail(r,MCC_CACHE_BAD_BSP,0);
    for (i=0;i<r->bsp_count;i++) {
        uint8_t *ref=bsp_refs+i*0x20;
        struct mcc_cache_bsp *b=&r->bsps[i];
        b->file_offset=mcc_u32(ref); b->size=mcc_u32(ref+4);
        b->address=mcc_u32(ref+8); b->tag_handle=mcc_u32(ref+0x1C);
        r->problem_tag=b->tag_handle&0xFFFF;
        if (r->problem_tag>=r->tag_count ||
            mcc_u32(tags+0x28+r->problem_tag*0x20)!=MCC_BSP ||
            mcc_u32(tags+0x34+r->problem_tag*0x20)!=b->tag_handle)
            return mcc_fail(r,MCC_CACHE_BAD_BSP,b->file_offset);
        status=mcc_check_bsp(source,r,b);
        if (status) return mcc_fail(r,status,b->file_offset);
        if (b->size>r->largest_bsp_bytes) r->largest_bsp_bytes=b->size;
    }
    r->problem_tag=UINT32_MAX;
    r->status=MCC_CACHE_OK;
    return MCC_CACHE_OK;
}

enum mcc_cache_status mcc_cache_inspect(struct mcc_cache_source const *source,
    struct mcc_cache_report *r) {
    uint8_t header[MCC_CACHE_HEADER_BYTES];
    struct mcc_cache_identity identity;
    enum mcc_cache_status status;
    void *tags;
    if (!source || !source->read || !r) return MCC_CACHE_BAD_ARGUMENT;
    memset(r,0,sizeof(*r)); r->problem_tag=UINT32_MAX;
    if (source->size<sizeof(header) || !source->read(source->context,0,sizeof(header),header))
        return mcc_fail(r,MCC_CACHE_READ_FAILED,0);
    status=mcc_cache_identify(header,sizeof(header),source->size,&identity);
    if (status) return mcc_fail(r,status,0);
    tags=malloc(identity.tag_data_size);
    if (!tags) return mcc_fail(r,MCC_CACHE_NO_MEMORY,0);
    status=mcc_cache_load(source,tags,identity.tag_data_size,r);
    free(tags);
    return status;
}

char const *mcc_cache_status_string(enum mcc_cache_status s) {
    static char const *const text[]={
        "ok", "invalid reader argument", "map read failed", "invalid MCC cache header",
        "not a Halo 1 MCC version-13 cache", "invalid MCC map file length",
        "unterminated MCC header string", "MCC map is not singleplayer or multiplayer",
        "invalid MCC tag data range", "insufficient memory for MCC tags",
        "invalid MCC tag index", "invalid MCC tag entry", "invalid MCC scenario",
        "invalid MCC model data range", "invalid MCC BSP or vertex range",
        "invalid MCC bitmap data", "invalid MCC sound data", "invalid MCC script data",
        "external indexed MCC tags require a resource provider"
    };
    return (unsigned)s<sizeof(text)/sizeof(text[0]) ? text[s] : "unknown MCC cache error";
}
