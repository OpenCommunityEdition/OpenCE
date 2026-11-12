/* MCC tag reservations belong to the host, before the guest starts. */
#ifndef HOST_MCC_MEMORY_H
#define HOST_MCC_MEMORY_H

void host_mcc_memory_initialize(void);
/* Guest ABI: address or negative errno; release returns zero or negative errno. */
long long host_mcc_memory_reserve(unsigned int address, unsigned int size);
int host_mcc_memory_release(unsigned int address, unsigned int size);

#endif
