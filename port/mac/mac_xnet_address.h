#ifndef HALO_MAC_XNET_ADDRESS_H
#define HALO_MAC_XNET_ADDRESS_H
/* Return the first up, non-loopback IPv4 interface and its Ethernet address
   when the host exposes one. The IPv4 value remains in network byte order. */
int mac_xnet_local_address(unsigned long *ipv4,unsigned char hardware[6]);
#endif
