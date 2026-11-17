/* Optional legacy attribution: whole-content SHA-256 and maker. Automatic
 * sharing does not require this file or use it as approval. */
#ifndef MCC_SHARE_LIST_H
#define MCC_SHARE_LIST_H
#define MCC_SHARE_LIST_FILE "mcc_share.txt"
#define MCC_SHARE_CREDIT_SIZE 64

/* text: the file's bytes (NUL not required, length bounds it). Lines are
 * "<64 hexadecimal digits> <maker>"; blank lines and lines starting with # are
 * ignored; a line with a malformed digest is skipped. Returns 1 and the maker for
 * the first line naming the digest, -1 when that line names no printable maker
 * (listed, but no attribution), 0 when no line names it. */
int mcc_share_list_find(char const *text, unsigned long length, unsigned char const digest[32],
    char credit[MCC_SHARE_CREDIT_SIZE]);
/* 64 lowercase digits and a NUL */
void mcc_share_list_hex(unsigned char const digest[32], char text[65]);
#endif
