/*
 * OpenCE Forge - Custom Edition & Map Share File I/O Redirection
 * Serves embedded .bmp / .txt sidecars from the 2048-byte map header.
 */

#ifndef FORGE_CE_IO_REDIRECT_H
#define FORGE_CE_IO_REDIRECT_H

FILE *forge_ce_fopen(char const *filename, char const *mode);
size_t forge_ce_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int forge_ce_fseek(FILE *stream, long offset, int whence);
long forge_ce_ftell(FILE *stream);
int forge_ce_fclose(FILE *stream);

#undef fopen
#define fopen forge_ce_fopen

#ifdef FORGE_CE_IO_STDIO
#undef fread
#define fread forge_ce_fread
#undef fseek
#define fseek forge_ce_fseek
#undef ftell
#define ftell forge_ce_ftell
#undef fclose
#define fclose forge_ce_fclose
#endif

#endif /* FORGE_CE_IO_REDIRECT_H */
