#ifndef MCC_SHA256_H
#define MCC_SHA256_H
#include <stdint.h>
struct mcc_sha256 {
    uint32_t state[8];
    unsigned char block[64];
    uint64_t length;
    int used;
};
void mcc_sha256_begin(struct mcc_sha256 *context);
void mcc_sha256_add(struct mcc_sha256 *context, void const *data, int size);
void mcc_sha256_end(struct mcc_sha256 *context, unsigned char digest[32]);
#endif
