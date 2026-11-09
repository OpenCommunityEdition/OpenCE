/* Loading feedback never advances simulation or dispatches network messages.
 * Conversion work also feeds the separate bounded send-only MCC loading lease. */
#ifndef MCC_LOADING_H
#define MCC_LOADING_H

enum mcc_loading_stage {
    MCC_LOADING_RESOURCES,
    MCC_LOADING_TAGS,
    MCC_LOADING_SCRIPTS,
    MCC_LOADING_GEOMETRY,
    MCC_LOADING_AUDIO,
    MCC_LOADING_BITMAPS,
    MCC_LOADING_VALIDATION,
    MCC_LOADING_SCENARIO,
    MCC_LOADING_STAGES
};

/* Returns whether visual feedback started. Pair every load with end, even
 * when presentation is unavailable: network work reporting has its own lifetime. */
int mcc_loading_begin(char const *name);
void mcc_loading_stage(enum mcc_loading_stage stage);
void mcc_loading_update(enum mcc_loading_stage stage, unsigned long done, unsigned long total);
void mcc_loading_pulse(void);
void mcc_loading_end(int success);

#endif
