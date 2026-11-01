/* MCC lobby checks are advanced only while the server is in pregame. */
#ifndef MCC_VALIDATION_H
#define MCC_VALIDATION_H
#include <stdint.h>

/* One pending package, four bounded successful results; no session identity. */
/* Request returns 1 when ready, 0 while pending, -1 after failure. Lookup
 * never schedules work. Cancel releases readers but retains completed facts. */
int mcc_validation_request(char const *name, uint32_t *checksum, unsigned char digest[32]);
int mcc_validation_lookup(char const *name, uint32_t *checksum, unsigned char digest[32]);
void mcc_validation_update(void);
void mcc_validation_cancel(void);
char const *mcc_validation_error(void);
#endif
