/* Management of locally received MCC snapshots, separate from original packages. */
#ifndef MCC_SHARE_MENU_H
#define MCC_SHARE_MENU_H
void mcc_share_menu_open(void);
void mcc_share_menu_step(int direction);
void mcc_share_menu_remove(void);
void mcc_share_menu_text(wchar_t text[512]);
int mcc_share_menu_confirming(void);
int mcc_share_menu_can_remove(void);
#endif
