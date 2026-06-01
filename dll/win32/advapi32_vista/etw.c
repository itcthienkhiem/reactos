/*
 * PROJECT:     ReactOS advapi32_vista
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ETW (Event Tracing for Windows) user-mode provider API (Vista+)
 * COPYRIGHT:   Copyright 2026 ReactOS contributors
 */

#include "advapi32_vista.h"

#define _EVNT_SOURCE_
#include <evntprov.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/*
 * NOTE: This is a minimal, no-op provider implementation that lets Vista/Win7
 * applications which link EventRegister/EventWrite load and run. Without a
 * registered ETW session listening, the real OS also performs almost no work
 * here, so returning success with no tracing is behaviourally compatible for
 * the common (no consumer attached) case. A full ETW backend can replace these
 * once the kernel-side WMI/ETW infrastructure is in place.
 */

/*
 * Per-thread current activity id, used by EventActivityIdControl.
 *
 * ReactOS DLLs are loaded dynamically, so MSVC __declspec(thread) static TLS
 * cannot be relied on here (no _tls_index in a dynamically loaded module).
 * Use a dynamically allocated TLS slot instead, holding a pointer to a
 * heap-allocated GUID per thread.
 */
static DWORD etw_activity_tls = TLS_OUT_OF_INDEXES;

/* Monotonic counter to fabricate unique activity ids for CREATE_ID. */
static volatile LONG etw_activity_counter;

VOID
EtwInitialize(VOID)
{
    if (etw_activity_tls == TLS_OUT_OF_INDEXES)
        etw_activity_tls = TlsAlloc();
}

static GUID *
etw_get_thread_activity_id(VOID)
{
    GUID *id;

    if (etw_activity_tls == TLS_OUT_OF_INDEXES)
        return NULL;

    id = TlsGetValue(etw_activity_tls);
    if (!id)
    {
        id = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*id));
        if (id)
            TlsSetValue(etw_activity_tls, id);
    }
    return id;
}

static VOID
etw_fabricate_activity_id(GUID *id)
{
    id->Data1 = (ULONG)InterlockedIncrement(&etw_activity_counter);
    id->Data2 = (USHORT)GetCurrentThreadId();
    id->Data3 = (USHORT)GetCurrentProcessId();
    *(ULONGLONG *)id->Data4 = GetTickCount();
}

/***********************************************************************
 *           EventRegister   (advapi32.@)
 */
ULONG WINAPI
EventRegister(
    _In_ LPCGUID ProviderId,
    _In_opt_ PENABLECALLBACK EnableCallback,
    _In_opt_ PVOID CallbackContext,
    _Out_ PREGHANDLE RegHandle)
{
    FIXME("(%s, %p, %p, %p): stub\n", wine_dbgstr_guid(ProviderId),
          EnableCallback, CallbackContext, RegHandle);

    if (!RegHandle)
        return ERROR_INVALID_PARAMETER;

    /* Hand back a non-zero opaque handle so callers treat us as registered. */
    *RegHandle = (REGHANDLE)(ULONG_PTR)0x1;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           EventUnregister   (advapi32.@)
 */
ULONG WINAPI
EventUnregister(
    _In_ REGHANDLE RegHandle)
{
    TRACE("(%I64x)\n", RegHandle);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           EventEnabled   (advapi32.@)
 *
 * With no session listening, no event is enabled.
 */
BOOLEAN WINAPI
EventEnabled(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
    TRACE("(%I64x, %p)\n", RegHandle, EventDescriptor);
    return FALSE;
}

/***********************************************************************
 *           EventProviderEnabled   (advapi32.@)
 */
BOOLEAN WINAPI
EventProviderEnabled(
    _In_ REGHANDLE RegHandle,
    _In_ UCHAR Level,
    _In_ ULONGLONG Keyword)
{
    TRACE("(%I64x, %u, %I64x)\n", RegHandle, Level, Keyword);
    return FALSE;
}

/***********************************************************************
 *           EventWrite   (advapi32.@)
 */
ULONG WINAPI
EventWrite(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData)
{
    TRACE("(%I64x, %p, %u, %p)\n", RegHandle, EventDescriptor, UserDataCount, UserData);

    if (!EventDescriptor)
        return ERROR_INVALID_PARAMETER;

    /* No consumer: drop the event and report success. */
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           EventWriteTransfer   (advapi32.@)
 */
ULONG WINAPI
EventWriteTransfer(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_opt_ LPCGUID ActivityId,
    _In_opt_ LPCGUID RelatedActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData)
{
    TRACE("(%I64x, %p, %p, %p, %u, %p)\n", RegHandle, EventDescriptor,
          ActivityId, RelatedActivityId, UserDataCount, UserData);

    if (!EventDescriptor)
        return ERROR_INVALID_PARAMETER;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           EventWriteString   (advapi32.@)
 */
ULONG WINAPI
EventWriteString(
    _In_ REGHANDLE RegHandle,
    _In_ UCHAR Level,
    _In_ ULONGLONG Keyword,
    _In_ PCWSTR String)
{
    TRACE("(%I64x, %u, %I64x, %s)\n", RegHandle, Level, Keyword, wine_dbgstr_w(String));
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           EventActivityIdControl   (advapi32.@)
 */
ULONG WINAPI
EventActivityIdControl(
    _In_ ULONG ControlCode,
    _Inout_ LPGUID ActivityId)
{
    GUID *current;

    TRACE("(%u, %p)\n", ControlCode, ActivityId);

    if (!ActivityId)
        return ERROR_INVALID_PARAMETER;

    current = etw_get_thread_activity_id();
    if (!current)
        return ERROR_OUTOFMEMORY;

    switch (ControlCode)
    {
        case EVENT_ACTIVITY_CTRL_GET_ID:
            *ActivityId = *current;
            break;

        case EVENT_ACTIVITY_CTRL_SET_ID:
            *current = *ActivityId;
            break;

        case EVENT_ACTIVITY_CTRL_CREATE_ID:
            /* Fabricate a unique-per-call id without external dependencies. */
            etw_fabricate_activity_id(ActivityId);
            break;

        case EVENT_ACTIVITY_CTRL_GET_SET_ID:
        {
            GUID previous = *current;
            *current = *ActivityId;
            *ActivityId = previous;
            break;
        }

        case EVENT_ACTIVITY_CTRL_CREATE_SET_ID:
            *ActivityId = *current;
            etw_fabricate_activity_id(current);
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}
