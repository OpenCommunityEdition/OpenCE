#ifndef D3D8_VISIBILITY_INTERNAL_H
#define D3D8_VISIBILITY_INTERNAL_H
#include "device_layout.h"
namespace D3D
{
/* January report payload: timestamp @0, samples @8, completion status @12.
 * The GPU writes the status asynchronously; this qualifier describes actual
 * device-shared storage rather than an instruction-selection workaround. */
struct VisibilityReport
{
    unsigned __int64 TimeStamp;
    DWORD Samples;
    volatile DWORD Status;
};
typedef char visibility_report_size[sizeof(VisibilityReport) == 16 ? 1 : -1];
BYTE *WINAPI GetVisibilityAddress(
    DWORD index);
}
#endif
