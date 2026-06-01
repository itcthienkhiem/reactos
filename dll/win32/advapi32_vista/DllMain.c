
#include "advapi32_vista.h"

BOOL
RegInitialize(VOID);

VOID
EtwInitialize(VOID);

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    /* For now, there isn't much to do */
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDll);

        if (!RegInitialize())
        {
            return FALSE;
        }

        EtwInitialize();
    }

    return TRUE;
}
