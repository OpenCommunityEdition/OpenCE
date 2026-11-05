/* The host's own list of the packages it attests are its to share. A line names a
 * package's whole-content SHA-256 (the identity the game record carries) and the
 * maker to credit. This is the local evidence mcc_share_rules.h asks the caller
 * for: content is classed custom only when its exact fingerprint is listed with
 * a maker; a rename, a changed byte or a missing line leaves it unknown. */
#ifndef MCC_SHARE_LIST_H
#define MCC_SHARE_LIST_H
#define MCC_SHARE_LIST_FILE "mcc_share.txt"
#define MCC_SHARE_CREDIT_SIZE 64

/* text: the file's bytes (NUL not required, length bounds it). Lines are
 * "<64 hexadecimal digits> <maker>"; blank lines and lines starting with # are
 * ignored; a line with a malformed digest is skipped. Returns 1 and the maker for
 * the first line naming the digest, -1 when that line names no printable maker
 * (listed, but not creditable: it is not shared), 0 when no line names it. */
int mcc_share_list_find(char const *text, unsigned long length, unsigned char const digest[32],
    char credit[MCC_SHARE_CREDIT_SIZE]);
/* 64 lowercase digits and a NUL */
void mcc_share_list_hex(unsigned char const digest[32], char text[65]);
#endif
