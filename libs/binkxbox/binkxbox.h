#ifndef BINK_XBOX_H
#define BINK_XBOX_H

/* Values proven by the target masks; names are descriptive reconstructions. */
enum bink_converter_flags
{
	BINK_CONVERTER_FORMAT_MASK= 0x0f,
	BINK_CONVERTER_ALL_FORMATS= 0x20,
	BINK_CONVERTER_M_SUFFIX= 0x40,
	BINK_CONVERTER_X2_SUFFIX= 0x100
};

void __stdcall BinkLoadUnload(
	int load);
void __stdcall BinkLoadUnloadConverter(
	unsigned long flags,
	int load);

#endif
