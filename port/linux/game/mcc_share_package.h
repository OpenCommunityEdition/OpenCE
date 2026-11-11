/* Build the host's explicit MCC transfer offer from its own installation. */
#ifndef MCC_SHARE_PACKAGE_H
#define MCC_SHARE_PACKAGE_H
struct mcc_share_host_package;
void mcc_share_package_build(char const *level, struct mcc_share_host_package *package);
#endif
