/* Local MCC packages retain their virtual identity in either disk layout. */
#ifndef MCC_LOCAL_PACKAGE_H
#define MCC_LOCAL_PACKAGE_H

/* Resolve only nested local map/resource files. Direct package files win;
 * returning zero leaves the established translation and missing-file path. */
int mcc_local_package_path(const char *request, const char *data_root, char *path, unsigned long size);

#endif
