/*
BITMAPS.C

symbols in this file:
0006ADC0 0070:
	_bitmap_type_get_string (0000)
0006AE30 0080:
	_bitmap_format_get_string (0000)
0006AEB0 0070:
	_bitmap_format_get_bits_per_pixel (0000)
0006AF20 0040:
	_bitmap_changed (0000)
0006AF60 0050:
	_bitmap_delete (0000)
0006AFB0 0220:
	_bitmap_2d_address (0000)
0006B1D0 0290:
	_bitmap_3d_address (0000)
0006B460 0210:
	_bitmap_cube_map_address (0000)
0006B670 00d0:
	_bitmap_mipmap_address (0000)
0006B740 0220:
	_bitmap_format_to_a8r8g8b8 (0000)
0006B960 0010:
	_bitmap_byte_swap_pixels (0000)
0006B970 0100:
	_palette_find_closest_match (0000)
0006BA70 0020:
	_code_0006ba70 (0000)
0006BA90 0020:
	_code_0006ba90 (0000)
0006BAB0 0030:
	_code_0006bab0 (0000)
0006BAE0 0160:
	_bitmap_verify (0000)
0006BC40 0080:
	_bitmap_rebuild (0000)
0006BCC0 0090:
	_bitmap_get_max_mipmap_count (0000)
0006BD50 00a0:
	_bitmap_mipmap_get_width (0000)
0006BDF0 00a0:
	_bitmap_mipmap_get_height (0000)
0006BE90 0090:
	_bitmap_mipmap_get_depth (0000)
0006BF20 00b0:
	_bitmap_mipmap_get_pixel_count (0000)
0006BFD0 0090:
	_bitmap_mipmap_get_pixel_data_size (0000)
0006C060 00e0:
	_bitmap_mipmap_get_row_pitch (0000)
0006C140 0510:
	_bitmap_2d_get_pixel (0000)
0006C650 0060:
	_bitmap_get_pixel_count (0000)
0006C6B0 0070:
	_bitmap_get_pixel_data_size (0000)
0006C720 0180:
	_bitmap_2d_new (0000)
0006C8A0 01c0:
	_bitmap_3d_new (0000)
0006CA60 0170:
	_bitmap_cube_map_new (0000)
0006CBD0 0280:
	_bitmap_3d_slice_extract (0000)
0006CE50 0280:
	_bitmap_3d_slice_insert (0000)
0006D0D0 0280:
	_bitmap_cube_map_face_extract (0000)
0006D350 0280:
	_bitmap_cube_map_face_insert (0000)
002544CC 0013:
	_rdata_002544cc (0000)
002544E0 0014:
	??_C@_0BE@NMMIDGBH@palettized?5bump?5map?$AA@ (0000)
002544F4 0016:
	??_C@_0BG@OOFEMLJA@true?9color?5with?5alpha?$AA@ (0000)
0025450C 000b:
	??_C@_0L@BOBKAIGE@true?9color?$AA@ (0000)
00254518 0016:
	??_C@_0BG@KKGKOADO@high?9color?5with?5alpha?$AA@ (0000)
00254530 001c:
	??_C@_0BM@LMNMJHOE@high?9color?5with?51?9bit?5alpha?$AA@ (0000)
0025454C 0007:
	??_C@_06MOMCHLAF@r6g5b5?$AA@ (0000)
00254554 000b:
	??_C@_0L@HMILFLFB@high?9color?$AA@ (0000)
00254560 0019:
	??_C@_0BJ@NGPIOEHO@separate?5alpha?9intensity?$AA@ (0000)
0025457C 0019:
	??_C@_0BJ@GECAPECH@combined?5alpha?9intensity?$AA@ (0000)
00254598 000a:
	??_C@_09DDBFFAKC@intensity?$AA@ (0000)
002545A4 0006:
	??_C@_05IAEKHIAN@alpha?$AA@ (0000)
002545AC 000b:
	??_C@_0L@HGKEADFO@3d?5texture?$AA@ (0000)
002545B8 000b:
	??_C@_0L@LHCKNMJO@2d?5texture?$AA@ (0000)
002545C4 0037:
	??_C@_0DH@BEHPNKOO@bitmap_type_string_table?$FLNUMBER_@ (0000)
002545FC 0027:
	??_C@_0CH@BDFKHBGO@type?$DO?$DN0?5?$CG?$CG?5type?$DMNUMBER_OF_BITMAP@ (0000)
00254624 0021:
	??_C@_0CB@ICKBKEMJ@c?3?2halo?2SOURCE?2bitmaps?2bitmaps?4c@ (0000)
00254648 003b:
	??_C@_0DL@KKAOOLLI@bitmap_format_string_table?$FLNUMBE@ (0000)
00254684 002d:
	??_C@_0CN@FACOLOCE@format?$DO?$DN0?5?$CG?$CG?5format?$DMNUMBER_OF_BI@ (0000)
002546B4 002e:
	??_C@_0CO@OINHIEJL@bitmap_format_bits_per_pixel_tab@ (0000)
002546E8 0042:
	??_C@_0EC@HCBFGBH@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
00254730 0044:
	??_C@_0EE@JDKFDDJD@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
00254774 0036:
	??_C@_0DG@JNCCJPLP@mipmap_index?$DO?$DN0?5?$CG?$CG?5mipmap_index?$DM@ (0000)
002547AC 0019:
	??_C@_0BJ@HFDMDDBL@y?$DO?$DN0?5?$CG?$CG?5y?$DMbitmap?9?$DOheight?$AA@ (0000)
002547C8 0018:
	??_C@_0BI@PFMPFOOK@x?$DO?$DN0?5?$CG?$CG?5x?$DMbitmap?9?$DOwidth?$AA@ (0000)
002547E0 0015:
	??_C@_0BF@FJOEGAHK@bitmap?9?$DObase_address?$AA@ (0000)
002547F8 004a:
	??_C@_0EK@CEJFGEAF@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
00254848 004c:
	??_C@_0EM@ILMNFLHJ@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
00254894 0018:
	??_C@_0BI@DEJNOKHK@z?$DO?$DN0?5?$CG?$CG?5z?$DMbitmap?9?$DOdepth?$AA@ (0000)
002548AC 000f:
	??_C@_0P@FPPECFAG@mipmap_address?$AA@ (0000)
002548BC 001a:
	??_C@_0BK@JFOFDHCG@closest_match_index?$CB?$DNNONE?$AA@ (0000)
002548D8 0035:
	??_C@_0DF@KNEGKCJ@?$CD?$CD?$CD?5ERROR?5bitmap?5?$EA?$CFp?5?$CI?$CD?$CFdx?$CD?$CFd?$CJ?5a@ (0000)
00254910 0040:
	??_C@_0EA@GIFFGKIJ@?$CD?$CD?$CD?5ERROR?5bitmap?5?$EA?$CFp?5?$CI?$CD?$CFdx?$CD?$CFd?$CJ?5a@ (0000)
00254950 001d:
	??_C@_0BN@GIHJBEAI@bitmap_verify?$CIbitmap?0?5FALSE?$CJ?$AA@ (0000)
00254970 0030:
	??_C@_0DA@NMAFJIPF@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
002549A0 0032:
	??_C@_0DC@IAIEDLML@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
002549D4 000f:
	??_C@_0P@KMBFAFCG@y?$DO?$DN0?5?$CG?$CG?5y?$DM4096?$AA@ (0000)
002549E4 000f:
	??_C@_0P@LDJJNFHK@x?$DO?$DN0?5?$CG?$CG?5x?$DM4096?$AA@ (0000)
002549F8 0079:
	??_C@_0HJ@FCOPIPKI@bitmap_2d_get_pixel?5tried?5to?5acc@ (0000)
00254A78 007b:
	??_C@_0HL@JNIKKJE@bitmap_2d_get_pixel?5tried?5to?5acc@ (0000)
00254AF4 0017:
	??_C@_0BH@KGBHNMIE@lod?$DO?$DN0?40f?5?$CG?$CG?5lod?$DM?$DN1?40f?$AA@ (0000)
00254B0C 002e:
	??_C@_0CO@GAMEHMN@?$CBTEST_FLAG?$CIbitmap?9?$DOflags?0?5_bitma@ (0000)
00254B3C 0024:
	??_C@_0CE@IDMEMNBL@?$CD?$CD?$CD?5ERROR?5failed?5to?5allocate?5bit@ (0000)
00254B60 0032:
	??_C@_0DC@NHEJMJDK@?$CD?$CD?$CD?5ERROR?5failed?5to?5allocate?5bit@ (0000)
00254B98 0041:
	??_C@_0EB@LFCKBNF@bitmap_format_type_valid_height?$CI@ (0000)
00254BE0 0040:
	??_C@_0EA@PKAAFKPD@bitmap_format_type_valid_width?5?$CI@ (0000)
00254C20 0040:
	??_C@_0EA@IPIJEMGL@bitmap_format_type_valid_depth?5?$CI@ (0000)
00254C60 0041:
	??_C@_0EB@JAPHONLK@bitmap_format_type_valid_height?$CI@ (0000)
00254CA8 0040:
	??_C@_0EA@DLIOIFDD@bitmap_format_type_valid_width?5?$CI@ (0000)
00254CE8 0015:
	??_C@_0BF@IJMOOLDH@?$CIwidth?$CG?$CIwidth?91?$CJ?$CJ?$DN?$DN0?$AA@ (0000)
00254D00 0045:
	??_C@_0EF@IICJNIK@bitmap_format_type_valid_width?$CIf@ (0000)
00254D48 0036:
	??_C@_0DG@FMDLIHFJ@?$CBTEST_FLAG?$CIslice_bitmap?9?$DOflags?0?5@ (0000)
00254D80 002c:
	??_C@_0CM@EGJNMILG@slice_bitmap?9?$DOformat?$DN?$DNsource_bit@ (0000)
00254DAC 0024:
	??_C@_0CE@BICOCAEK@slice_bitmap?9?$DOtype?$DN?$DN_bitmap_type@ (0000)
00254DD0 001e:
	??_C@_0BO@KMMCPBDP@slice_bitmap?9?$DOmipmap_count?$DN?$DN0?$AA@ (0000)
00254DF0 0023:
	??_C@_0CD@CAAKKKMF@bitmap_verify?$CIslice_bitmap?0?5FALS@ (0000)
00254E14 0037:
	??_C@_0DH@HEICNHNC@?$CBTEST_FLAG?$CIsource_bitmap?9?$DOflags?0@ (0000)
00254E50 0049:
	??_C@_0EJ@MCLGGFPA@MAX?$CI1?0?5source_bitmap?9?$DOheight?$DO?$DOso@ (0000)
00254EA0 0048:
	??_C@_0EI@LIHIIGIH@MAX?$CI1?0?5source_bitmap?9?$DOwidth?5?$DO?$DOso@ (0000)
00254EE8 0041:
	??_C@_0EB@KNIHOCN@source_slice_index?$DO?$DN0?5?$CG?$CG?5source_@ (0000)
00254F30 0053:
	??_C@_0FD@PPNICDCJ@MAX?$CI1?0?5destination_bitmap?9?$DOheigh@ (0000)
00254F88 0052:
	??_C@_0FC@FNCEBHHP@MAX?$CI1?0?5destination_bitmap?9?$DOwidth@ (0000)
00254FE0 0050:
	??_C@_0FA@MMKBHEAM@destination_slice_index?$DO?$DN0?5?$CG?$CG?5de@ (0000)
00255030 0031:
	??_C@_0DB@DIIJDBNH@slice_bitmap?9?$DOformat?$DN?$DNdestinatio@ (0000)
00255064 0035:
	??_C@_0DF@JCFMHHNJ@?$CBTEST_FLAG?$CIface_bitmap?9?$DOflags?0?5_@ (0000)
0025509C 002b:
	??_C@_0CL@JBBIIFAN@face_bitmap?9?$DOformat?$DN?$DNsource_bitm@ (0000)
002550C8 0023:
	??_C@_0CD@KFLOJMCE@face_bitmap?9?$DOtype?$DN?$DN_bitmap_type_@ (0000)
002550EC 001d:
	??_C@_0BN@PLKACPJ@face_bitmap?9?$DOmipmap_count?$DN?$DN0?$AA@ (0000)
0025510C 0022:
	??_C@_0CC@KDDOJPOH@bitmap_verify?$CIface_bitmap?0?5FALSE@ (0000)
00255130 0048:
	??_C@_0EI@OAEENKNE@MAX?$CI1?0?5source_bitmap?9?$DOheight?$DO?$DOso@ (0000)
00255178 0047:
	??_C@_0EH@OIPLGNCD@MAX?$CI1?0?5source_bitmap?9?$DOwidth?5?$DO?$DOso@ (0000)
002551C0 0043:
	??_C@_0ED@NEHKMLHM@source_face_index?$DO?$DN0?5?$CG?$CG?5source_f@ (0000)
00255208 0052:
	??_C@_0FC@FGJHNELB@MAX?$CI1?0?5destination_bitmap?9?$DOheigh@ (0000)
00255260 0051:
	??_C@_0FB@GBNEJIOO@MAX?$CI1?0?5destination_bitmap?9?$DOwidth@ (0000)
002552B8 004d:
	??_C@_0EN@PLLIEHKF@destination_face_index?$DO?$DN0?5?$CG?$CG?5des@ (0000)
00255308 0030:
	??_C@_0DA@BPGHACIB@face_bitmap?9?$DOformat?$DN?$DNdestination@ (0000)
002DC660 045c:
	_global_vector_palette (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "math/integer_math.h"

/* ---------- constants */

enum
{
	NUMBER_OF_ENTRIES_IN_PALETTE = 256,
	NUMBER_OF_BITMAP_TYPES = 3,
	NUMBER_OF_BITMAP_FORMATS = 18,
};

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

pixel32 global_vector_palette[NUMBER_OF_ENTRIES_IN_PALETTE] =
{
	0xFF7A19CC, 0xFF7E19CC, 0xFF8019CC, 0xFF8119CC, 0xFF8519CC, 0xFF742FE2, 0xFF7A2FE2, 0xFF7E2FE2,
	0xFF802FE2, 0xFF812FE2, 0xFF852FE2, 0xFF8B2FE2, 0xFF6B42ED, 0xFF7442EE, 0xFF7A42EF, 0xFF7E42EF,
	0xFF8042EF, 0xFF8142EF, 0xFF8542EF, 0xFF8B42EE, 0xFF9442ED, 0xFF6052F2, 0xFF6B52F5, 0xFF7452F6,
	0xFF7A52F7, 0xFF7E52F7, 0xFF8052F7, 0xFF8152F7, 0xFF8552F7, 0xFF8B52F6, 0xFF9452F5, 0xFF9F52F2,
	0xFF5260F2, 0xFF6060F7, 0xFF6B60F9, 0xFF7460FB, 0xFF7A60FB, 0xFF7E60FB, 0xFF8060FB, 0xFF8160FB,
	0xFF8560FB, 0xFF8B60FB, 0xFF9460F9, 0xFF9F60F7, 0xFFAD60F2, 0xFF426BED, 0xFF526BF5, 0xFF606BF9,
	0xFF6B6BFC, 0xFF746BFD, 0xFF7A6BFD, 0xFF7E6BFD, 0xFF806BFD, 0xFF816BFD, 0xFF856BFD, 0xFF8B6BFD,
	0xFF946BFC, 0xFF9F6BF9, 0xFFAD6BF5, 0xFFBD6BED, 0xFF2F74E2, 0xFF4274EE, 0xFF5274F6, 0xFF6074FB,
	0xFF6B74FD, 0xFF7474FE, 0xFF7A74FE, 0xFF7E74FE, 0xFF8074FE, 0xFF8174FE, 0xFF8574FE, 0xFF8B74FE,
	0xFF9474FD, 0xFF9F74FB, 0xFFAD74F6, 0xFFBD74EE, 0xFFD074E2, 0xFF197ACC, 0xFF2F7AE2, 0xFF427AEF,
	0xFF527AF7, 0xFF607AFB, 0xFF6B7AFD, 0xFF747AFE, 0xFF7A7AFF, 0xFF7E7AFF, 0xFF807AFF, 0xFF817AFF,
	0xFF857AFF, 0xFF8B7AFE, 0xFF947AFD, 0xFF9F7AFB, 0xFFAD7AF7, 0xFFBD7AEF, 0xFFD07AE2, 0xFFE57ACC,
	0xFF197ECC, 0xFF2F7EE2, 0xFF427EEF, 0xFF527EF7, 0xFF607EFB, 0xFF6B7EFD, 0xFF747EFE, 0xFF7A7EFF,
	0xFF7E7EFF, 0xFF807EFF, 0xFF817EFF, 0xFF857EFF, 0xFF8B7EFE, 0xFF947EFD, 0xFF9F7EFB, 0xFFAD7EF7,
	0xFFBD7EEF, 0xFFD07EE2, 0xFFE57ECC, 0xFF1980CC, 0xFF2F80E2, 0xFF4280EF, 0xFF5280F7, 0xFF6080FB,
	0xFF6B80FD, 0xFF7480FE, 0xFF7A80FF, 0xFF7E80FF, 0xFF8080FF, 0xFF8180FF, 0xFF8580FF, 0xFF8B80FE,
	0xFF9480FD, 0xFF9F80FB, 0xFFAD80F7, 0xFFBD80EF, 0xFFD080E2, 0xFFE580CC, 0xFF1981CC, 0xFF2F81E2,
	0xFF4281EF, 0xFF5281F7, 0xFF6081FB, 0xFF6B81FD, 0xFF7481FE, 0xFF7A81FF, 0xFF7E81FF, 0xFF8081FF,
	0xFF8181FF, 0xFF8581FF, 0xFF8B81FE, 0xFF9481FD, 0xFF9F81FB, 0xFFAD81F7, 0xFFBD81EF, 0xFFD081E2,
	0xFFE581CC, 0xFF1985CC, 0xFF2F85E2, 0xFF4285EF, 0xFF5285F7, 0xFF6085FB, 0xFF6B85FD, 0xFF7485FE,
	0xFF7A85FF, 0xFF7E85FF, 0xFF8085FF, 0xFF8185FF, 0xFF8585FF, 0xFF8B85FE, 0xFF9485FD, 0xFF9F85FB,
	0xFFAD85F7, 0xFFBD85EF, 0xFFD085E2, 0xFFE585CC, 0xFF2F8BE2, 0xFF428BEE, 0xFF528BF6, 0xFF608BFB,
	0xFF6B8BFD, 0xFF748BFE, 0xFF7A8BFE, 0xFF7E8BFE, 0xFF808BFE, 0xFF818BFE, 0xFF858BFE, 0xFF8B8BFE,
	0xFF948BFD, 0xFF9F8BFB, 0xFFAD8BF6, 0xFFBD8BEE, 0xFFD08BE2, 0xFF4294ED, 0xFF5294F5, 0xFF6094F9,
	0xFF6B94FC, 0xFF7494FD, 0xFF7A94FD, 0xFF7E94FD, 0xFF8094FD, 0xFF8194FD, 0xFF8594FD, 0xFF8B94FD,
	0xFF9494FC, 0xFF9F94F9, 0xFFAD94F5, 0xFFBD94ED, 0xFF529FF2, 0xFF609FF7, 0xFF6B9FF9, 0xFF749FFB,
	0xFF7A9FFB, 0xFF7E9FFB, 0xFF809FFB, 0xFF819FFB, 0xFF859FFB, 0xFF8B9FFB, 0xFF949FF9, 0xFF9F9FF7,
	0xFFAD9FF2, 0xFF60ADF2, 0xFF6BADF5, 0xFF74ADF6, 0xFF7AADF7, 0xFF7EADF7, 0xFF80ADF7, 0xFF81ADF7,
	0xFF85ADF7, 0xFF8BADF6, 0xFF94ADF5, 0xFF9FADF2, 0xFF6BBDED, 0xFF74BDEE, 0xFF7ABDEF, 0xFF7EBDEF,
	0xFF80BDEF, 0xFF81BDEF, 0xFF85BDEF, 0xFF8BBDEE, 0xFF94BDED, 0xFF74D0E2, 0xFF7AD0E2, 0xFF7ED0E2,
	0xFF80D0E2, 0xFF81D0E2, 0xFF85D0E2, 0xFF8BD0E2, 0xFF7AE5CC, 0xFF7EE5CC, 0xFF80E5CC, 0xFF81E5CC,
	0xFF85E5CC, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x008080FF,
};

static char const *bitmap_type_string_table[NUMBER_OF_BITMAP_TYPES + 1] =
{
	"2d texture",
	"3d texture",
	"cube map",
	NULL,
};

static char const *bitmap_format_string_table[NUMBER_OF_BITMAP_FORMATS + 1] =
{
	"alpha",
	"intensity",
	"combined alpha-intensity",
	"separate alpha-intensity",
	"",
	"",
	"high-color",
	"r6g5b5",
	"high-color with 1-bit alpha",
	"high-color with alpha",
	"true-color",
	"true-color with alpha",
	"",
	"",
	"compressed with color-key transparency",
	"compressed with explicit alpha",
	"compressed with interpolated alpha",
	"palettized bump map",
	NULL,
};

/* ---------- public code */

char const *bitmap_type_get_string(
	short type)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 80, type>=0 && type<NUMBER_OF_BITMAP_TYPES);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 81, bitmap_type_string_table[NUMBER_OF_BITMAP_TYPES]==NULL);

	return bitmap_type_string_table[type];
}

char const *bitmap_format_get_string(
	short format)
{
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 134, format>=0 && format<NUMBER_OF_BITMAP_FORMATS);
	match_assert("c:\\halo\\SOURCE\\bitmaps\\bitmaps.c", 135, bitmap_format_string_table[NUMBER_OF_BITMAP_FORMATS]==NULL);

	return bitmap_format_string_table[format];
}

/* ---------- private code */
