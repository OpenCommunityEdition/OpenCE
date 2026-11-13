/* ---------- headers */

#include "cseries.h"
#include "cseries/errors.h"

/* ---------- constants */

#define MAKE_D3D_HRESULT(code) ((long)(0x88760000UL | (code)))

#define E_OUTOFMEMORY ((long)0x8007000EUL)
#define E_FAIL ((long)0x80004005UL)
#define E_INVALIDARG ((long)0x80070057UL)

#define D3DERR_WRONGTEXTUREFORMAT MAKE_D3D_HRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION MAKE_D3D_HRESULT(2073)
#define D3DERR_UNSUPPORTEDCOLORARG MAKE_D3D_HRESULT(2074)
#define D3DERR_UNSUPPORTEDALPHAOPERATION MAKE_D3D_HRESULT(2075)
#define D3DERR_UNSUPPORTEDALPHAARG MAKE_D3D_HRESULT(2076)
#define D3DERR_TOOMANYOPERATIONS MAKE_D3D_HRESULT(2077)
#define D3DERR_CONFLICTINGTEXTUREFILTER MAKE_D3D_HRESULT(2078)
#define D3DERR_UNSUPPORTEDFACTORVALUE MAKE_D3D_HRESULT(2079)
#define D3DERR_CONFLICTINGRENDERSTATE MAKE_D3D_HRESULT(2081)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER MAKE_D3D_HRESULT(2082)
#define D3DERR_CONFLICTINGTEXTUREPALETTE MAKE_D3D_HRESULT(2086)
#define D3DERR_DRIVERINTERNALERROR MAKE_D3D_HRESULT(2087)
#define D3DERR_NOTFOUND MAKE_D3D_HRESULT(2150)
#define D3DERR_MOREDATA MAKE_D3D_HRESULT(2151)
#define D3DERR_DEVICELOST MAKE_D3D_HRESULT(2152)
#define D3DERR_DEVICENOTRESET MAKE_D3D_HRESULT(2153)
#define D3DERR_NOTAVAILABLE MAKE_D3D_HRESULT(2154)
#define D3DERR_OUTOFVIDEOMEMORY MAKE_D3D_HRESULT(380)
#define D3DERR_INVALIDDEVICE MAKE_D3D_HRESULT(2155)
#define D3DERR_INVALIDCALL MAKE_D3D_HRESULT(2156)

/* ---------- macros */

/* ---------- structures */

typedef long hresult;

/* ---------- prototypes */

void rasterizer_error(
	hresult error_result,
	char const *format,
	...);

hresult __stdcall D3DXGetErrorStringA(
	hresult error_result,
	char *buffer,
	unsigned long buffer_length);

/* ---------- globals */

/* ---------- public code */

void rasterizer_error(
	hresult error_result,
	char const *format,
	...)
{
	char formatted_message[1024];
	char error_description[1024];
	char const *error_name;
	va_list arguments;

	error_name = "<unknown error>";

	va_start(arguments, format);
	vsprintf(formatted_message, format, arguments);
	va_end(arguments);

	if (D3DXGetErrorStringA(error_result, error_description, sizeof(error_description) - 1) < 0)
	{
		csstrcpy(error_description, "<can't get description>");
	}

	switch (error_result)
	{
	case E_OUTOFMEMORY:
		error_name = "E_OUTOFMEMORY";
		break;
	case E_FAIL:
		error_name = "E_FAIL";
		break;
	case E_INVALIDARG:
		error_name = "E_INVALIDARG";
		break;
	case D3DERR_WRONGTEXTUREFORMAT:
		error_name = "D3DERR_WRONGTEXTUREFORMAT";
		break;
	case D3DERR_OUTOFVIDEOMEMORY:
		error_name = "D3DERR_OUTOFVIDEOMEMORY";
		break;
	case D3DERR_UNSUPPORTEDCOLOROPERATION:
		error_name = "D3DERR_UNSUPPORTEDCOLOROPERATION";
		break;
	case D3DERR_UNSUPPORTEDCOLORARG:
		error_name = "D3DERR_UNSUPPORTEDCOLORARG";
		break;
	case D3DERR_UNSUPPORTEDALPHAOPERATION:
		error_name = "D3DERR_UNSUPPORTEDALPHAOPERATION";
		break;
	case D3DERR_UNSUPPORTEDALPHAARG:
		error_name = "D3DERR_UNSUPPORTEDALPHAARG";
		break;
	case D3DERR_TOOMANYOPERATIONS:
		error_name = "D3DERR_TOOMANYOPERATIONS";
		break;
	case D3DERR_CONFLICTINGTEXTUREFILTER:
		error_name = "D3DERR_CONFLICTINGTEXTUREFILTER";
		break;
	case D3DERR_UNSUPPORTEDFACTORVALUE:
		error_name = "D3DERR_UNSUPPORTEDFACTORVALUE";
		break;
	case D3DERR_CONFLICTINGRENDERSTATE:
		error_name = "D3DERR_CONFLICTINGRENDERSTATE";
		break;
	case D3DERR_UNSUPPORTEDTEXTUREFILTER:
		error_name = "D3DERR_UNSUPPORTEDTEXTUREFILTER";
		break;
	case D3DERR_CONFLICTINGTEXTUREPALETTE:
		error_name = "D3DERR_CONFLICTINGTEXTUREPALETTE";
		break;
	case D3DERR_DRIVERINTERNALERROR:
		error_name = "D3DERR_DRIVERINTERNALERROR";
		break;
	case D3DERR_NOTFOUND:
		error_name = "D3DERR_NOTFOUND";
		break;
	case D3DERR_MOREDATA:
		error_name = "D3DERR_MOREDATA";
		break;
	case D3DERR_DEVICELOST:
		error_name = "D3DERR_DEVICELOST";
		break;
	case D3DERR_DEVICENOTRESET:
		error_name = "D3DERR_DEVICENOTRESET";
		break;
	case D3DERR_NOTAVAILABLE:
		error_name = "D3DERR_NOTAVAILABLE";
		break;
	case D3DERR_INVALIDDEVICE:
		error_name = "D3DERR_INVALIDDEVICE";
		break;
	case D3DERR_INVALIDCALL:
		error_name = "D3DERR_INVALIDCALL";
		break;
	}

	error(2, "%s in %s (code=%d, error=%s)", error_name, formatted_message, error_result, error_description);

	return;
}

/* ---------- private code */
