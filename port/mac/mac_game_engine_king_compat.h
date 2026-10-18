/* game_engine_king.c repeats two layout assertion typedefs from this header.
   GCC 4 rejects that C89 duplicate, so keep its second pair distinct without
   changing either assertion or the source unit. */
#include "rasterizer/rasterizer_model_types.h"
#define verify_model_vertex_uncompressed_size verify_king_model_vertex_uncompressed_size
#define verify_model_vertex_compressed_size verify_king_model_vertex_compressed_size
