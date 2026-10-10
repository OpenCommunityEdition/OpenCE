#ifndef MCC_PAUSE_H
#define MCC_PAUSE_H

enum mcc_pause_layout {
    MCC_PAUSE_FULL,
    MCC_PAUSE_HALF,
    MCC_PAUSE_QUARTER,
    MCC_PAUSE_LAYOUT_COUNT
};

enum mcc_pause_art_kind {
    MCC_PAUSE_ART_PANEL,
    MCC_PAUSE_ART_HIGHLIGHT,
    MCC_PAUSE_ART_DIM
};

/* Logical viewport sizes: 640x480, 640x240, 320x240. The art callback owns
 * and deduplicates its resources; highlights have normal/focused frames. */
struct mcc_pause_assets {
    long font[MCC_PAUSE_LAYOUT_COUNT];
    long small_font[MCC_PAUSE_LAYOUT_COUNT];
    long (*art)(short width, short height, enum mcc_pause_art_kind kind);
    boolean settings;
};

struct mcc_pause_tag {
    long group;
    long handle;
    char const *name;
    void *data;
};

/* Builds independent, owned definitions. The caller appends these records
 * to a copied live tag table before selecting/opening any of the widgets. */
boolean mcc_pause_build(long first_index, short first_salt, struct mcc_pause_assets const *assets);
long mcc_pause_tag_count(void);
struct mcc_pause_tag const *mcc_pause_tag_at(long ordinal);
boolean mcc_pause_owns(long handle);
/* Only these owned paragraph widgets need wrapping within their own bounds. */
boolean mcc_pause_text_wrap_needed(long handle);
long mcc_pause_select(boolean campaign, boolean network, boolean host, boolean teams,
    short local_count, boolean first_player);
void mcc_pause_dispose(void);

#endif
