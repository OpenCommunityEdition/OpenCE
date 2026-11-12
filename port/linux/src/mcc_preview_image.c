/* MCC metadata and PNG interpretation, independently implemented from the
 * public PNG specification. No downloaded code, assets or image libraries. */
#include "mcc_preview_image.h"
#include "mcc_manifest.h"
#include "mcc_menu_metadata.h"
#include "posix.h"
#include "zlib_prefixed.h"
#include <stdlib.h>
#include <string.h>

static unsigned be32(const unsigned char *p)
{
    return (unsigned)p[0]<<24 | (unsigned)p[1]<<16 | (unsigned)p[2]<<8 | p[3];
}

void mcc_preview_image_free(struct mcc_preview_image *image)
{
    free(image->pixels);
    memset(image,0,sizeof(*image));
}

static unsigned char paeth(int a, int b, int c)
{
    int p = a+b-c, x = abs(p-a), y = abs(p-b), z = abs(p-c);
    return (unsigned char)(x <= y && x <= z ? a : y <= z ? b : c);
}

int mcc_preview_png(const unsigned char *data, unsigned size, struct mcc_preview_image *image)
{
    unsigned at = 8, width = 0, height = 0, channels = 0, compressed_size = 0;
    unsigned row, x, stride = 0, filtered_size = 0;
    int state = 0, ended = 0, palette = 0, ok = 0;
    unsigned char *compressed = NULL, *filtered = NULL;
    uint32_t *pixels = NULL;
    uLongf output; uLong input;
    memset(image,0,sizeof(*image));
    if (!data || size < 33 || size > MCC_PREVIEW_FILE_LIMIT || memcmp(data,"\211PNG\r\n\032\n",8)) return 0;
    compressed = malloc(size);
    if (!compressed) return 0;
    while (at <= size-12) {
        unsigned length = be32(data+at);
        const unsigned char *kind = data+at+4, *body = data+at+8;
        unsigned k;
        if (length > size-at-12 || crc32(0,kind,length+4) != be32(body+length)) goto done;
        for (k=0; k<4; k++) if (!((kind[k]>='A' && kind[k]<='Z') || (kind[k]>='a' && kind[k]<='z'))) goto done;
        if (kind[2]&32) goto done;
        if (!state) {
            if (memcmp(kind,"IHDR",4) || length != 13) goto done;
            width = be32(body); height = be32(body+4);
            if (!width || !height || width > MCC_PREVIEW_DIMENSION_LIMIT || height > MCC_PREVIEW_DIMENSION_LIMIT ||
                body[8] != 8 || (body[9] != 2 && body[9] != 6) || body[10] || body[11] || body[12]) goto done;
            channels = body[9] == 2 ? 3 : 4;
            stride = width*channels; filtered_size = (stride+1)*height;
            state = 1;
        } else if (!memcmp(kind,"IDAT",4)) {
            if (state > 2) goto done;
            memcpy(compressed+compressed_size,body,length); compressed_size += length; state = 2;
        } else if (!memcmp(kind,"IEND",4)) {
            if (length || state < 2 || !compressed_size || at+12 != size) goto done;
            ended = 1; break;
        } else {
            /* Optional RGB palettes do not change RGB/RGBA samples. Other
             * critical chunks, duplicate headers and tRNS are unsupported. */
            if (!memcmp(kind,"PLTE",4)) {
                if (state != 1 || palette || !length || length > 768 || length%3) goto done;
                palette=1;
            }
            else if (!(kind[0]&32) || !memcmp(kind,"tRNS",4)) goto done;
            if (state == 2) state = 3;
        }
        at += length+12;
    }
    if (!ended) goto done;
    filtered = malloc(filtered_size); pixels = malloc(width*height*sizeof(*pixels));
    if (!filtered || !pixels) goto done;
    output = filtered_size; input = compressed_size;
    if (uncompress2(filtered,&output,compressed,&input) != Z_OK || output != filtered_size || input != compressed_size) goto done;
    for (row = 0; row < height; row++) {
        unsigned char *line = filtered+row*(stride+1)+1;
        unsigned char *previous = row ? line-(stride+1) : NULL;
        unsigned filter = line[-1];
        if (filter > 4) goto done;
        for (x = 0; x < stride; x++) {
            int a = x >= channels ? line[x-channels] : 0, b = previous ? previous[x] : 0;
            int c = previous && x >= channels ? previous[x-channels] : 0;
            if (filter == 1) line[x] += (unsigned char)a;
            else if (filter == 2) line[x] += (unsigned char)b;
            else if (filter == 3) line[x] += (unsigned char)((a+b)/2);
            else if (filter == 4) line[x] += paeth(a,b,c);
        }
        for (x = 0; x < width; x++) {
            const unsigned char *p = line+x*channels;
            pixels[row*width+x] = (uint32_t)(channels == 4 ? p[3] : 255)<<24 | (uint32_t)p[0]<<16 | (uint32_t)p[1]<<8 | p[2];
        }
    }
    image->width=width; image->height=height; image->pixels=pixels; pixels=NULL; ok=1;
done:
    free(pixels); free(filtered); free(compressed); return ok;
}

int mcc_preview_fit(unsigned width, unsigned height, int box_width, int box_height, int *w, int *h)
{
    if (!width || !height || width > MCC_PREVIEW_DIMENSION_LIMIT || height > MCC_PREVIEW_DIMENSION_LIMIT ||
        box_width <= 0 || box_height <= 0 || box_width > 32767 || box_height > 32767) return 0;
    *w = box_width; *h = (int)(height*(unsigned)box_width/width);
    if (*h > box_height) { *h=box_height; *w=(int)(width*(unsigned)box_height/height); }
    if (*w < 1) *w=1;
    if (*h < 1) *h=1;
    return 1;
}

/* Only explicitly listed mission/map JSON can supply images. The actual map
 * resolution supplies the root, so a local override never borrows Steam art. */
int mcc_preview_image_load(const char *level, int campaign, struct mcc_preview_image *image)
{
    char root[1024];
    struct mcc_menu_metadata *metadata;
    const struct steam_workshop_map_text *selected;
    const char *name;
    unsigned char *bytes = NULL;
    int length, valid = 0;
    memset(image,0,sizeof(*image));
    if (!mcc_manifest_package_root(level,root) || !(name = strrchr(level,'\\'))) return 0;
    name++;
    metadata = malloc(sizeof(*metadata));
    if (!metadata) return 0;
    if (!mcc_menu_metadata_load(root,campaign,metadata) || !(selected=mcc_menu_metadata_find(metadata,name))) goto done;
    bytes = malloc(MCC_PREVIEW_FILE_LIMIT);
    if (!bytes) goto done;
    length = posix_mcc_preview_read(root,selected->large,bytes,MCC_PREVIEW_FILE_LIMIT);
    valid = mcc_preview_png(bytes,(unsigned)length,image);
    if (!valid) {
        length = posix_mcc_preview_read(root,selected->thumbnail,bytes,MCC_PREVIEW_FILE_LIMIT);
        valid = mcc_preview_png(bytes,(unsigned)length,image);
    }
done:
    free(bytes); free(metadata); return valid;
}
