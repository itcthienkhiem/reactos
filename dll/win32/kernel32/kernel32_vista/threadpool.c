#include "k32_vista.h"

#include <threadpoolapiset.h>

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/* ntdll thread pool primitives (exported from ntdll_vista) */
extern NTSTATUS WINAPI TpAllocPool( TP_POOL **, PVOID );
extern NTSTATUS WINAPI TpAllocCleanupGroup( TP_CLEANUP_GROUP ** );
extern NTSTATUS WINAPI TpAllocIoCompletion( TP_IO **, HANDLE, PVOID, PVOID, TP_CALLBACK_ENVIRON * );
extern NTSTATUS WINAPI TpAllocTimer( TP_TIMER **, PTP_TIMER_CALLBACK, PVOID, TP_CALLBACK_ENVIRON * );
extern NTSTATUS WINAPI TpAllocWait( TP_WAIT **, PTP_WAIT_CALLBACK, PVOID, TP_CALLBACK_ENVIRON * );
extern NTSTATUS WINAPI TpAllocWork( TP_WORK **, PTP_WORK_CALLBACK, PVOID, TP_CALLBACK_ENVIRON * );
extern NTSTATUS WINAPI TpCallbackMayRunLong( TP_CALLBACK_INSTANCE * );
extern NTSTATUS WINAPI TpSetPoolStackInformation( TP_POOL *, TP_POOL_STACK_INFORMATION * );
extern NTSTATUS WINAPI TpQueryPoolStackInformation( TP_POOL *, TP_POOL_STACK_INFORMATION * );
extern NTSTATUS WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK, PVOID, TP_CALLBACK_ENVIRON * );

/***********************************************************************
 *           CallbackMayRunLong   (kernel32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CallbackMayRunLong( PTP_CALLBACK_INSTANCE instance )
{
    return set_ntstatus( TpCallbackMayRunLong( instance ));
}

/***********************************************************************
 *           CreateThreadpool   (kernel32.@)
 */
PTP_POOL WINAPI DECLSPEC_HOTPATCH CreateThreadpool( PVOID reserved )
{
    TP_POOL *pool;

    if (!set_ntstatus( TpAllocPool( &pool, reserved ))) return NULL;
    return pool;
}

/***********************************************************************
 *           CreateThreadpoolCleanupGroup   (kernel32.@)
 */
PTP_CLEANUP_GROUP WINAPI DECLSPEC_HOTPATCH CreateThreadpoolCleanupGroup( void )
{
    TP_CLEANUP_GROUP *group;

    if (!set_ntstatus( TpAllocCleanupGroup( &group ))) return NULL;
    return group;
}

static void WINAPI tp_io_callback( PTP_CALLBACK_INSTANCE instance, PVOID userdata, PVOID cvalue,
                                   PIO_STATUS_BLOCK iosb, PTP_IO io )
{
    PTP_WIN32_IO_CALLBACK callback = *(void **)io;
    callback( instance, userdata, cvalue, RtlNtStatusToDosError( iosb->Status ), iosb->Information, io );
}

/***********************************************************************
 *           CreateThreadpoolIo   (kernel32.@)
 */
PTP_IO WINAPI DECLSPEC_HOTPATCH CreateThreadpoolIo( HANDLE handle, PTP_WIN32_IO_CALLBACK callback,
                                                    PVOID userdata, PTP_CALLBACK_ENVIRON environment )
{
    TP_IO *io;

    if (!set_ntstatus( TpAllocIoCompletion( &io, handle, tp_io_callback, userdata, environment ))) return NULL;
    /* ntdll leaves us space to store our callback at the beginning of the TP_IO struct */
    *(void **)io = callback;
    return io;
}

/***********************************************************************
 *           CreateThreadpoolTimer   (kernel32.@)
 */
PTP_TIMER WINAPI DECLSPEC_HOTPATCH CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                                          PTP_CALLBACK_ENVIRON environment )
{
    TP_TIMER *timer;

    if (!set_ntstatus( TpAllocTimer( &timer, callback, userdata, environment ))) return NULL;
    return timer;
}

/***********************************************************************
 *           CreateThreadpoolWait   (kernel32.@)
 */
PTP_WAIT WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                                        PTP_CALLBACK_ENVIRON environment )
{
    TP_WAIT *wait;

    if (!set_ntstatus( TpAllocWait( &wait, callback, userdata, environment ))) return NULL;
    return wait;
}

/***********************************************************************
 *           CreateThreadpoolWork   (kernel32.@)
 */
PTP_WORK WINAPI DECLSPEC_HOTPATCH CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                                        PTP_CALLBACK_ENVIRON environment )
{
    TP_WORK *work;

    if (!set_ntstatus( TpAllocWork( &work, callback, userdata, environment ))) return NULL;
    return work;
}

/***********************************************************************
 *           TrySubmitThreadpoolCallback   (kernel32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           PTP_CALLBACK_ENVIRON environment )
{
    return set_ntstatus( TpSimpleTryPost( callback, userdata, environment ));
}

/***********************************************************************
 *           SetThreadpoolStackInformation   (kernel32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadpoolStackInformation( PTP_POOL pool, PTP_POOL_STACK_INFORMATION stack_info )
{
    return set_ntstatus( TpSetPoolStackInformation( pool, stack_info ));
}

/***********************************************************************
 *           QueryThreadpoolStackInformation   (kernel32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryThreadpoolStackInformation( PTP_POOL pool, PTP_POOL_STACK_INFORMATION stack_info )
{
    return set_ntstatus( TpQueryPoolStackInformation( pool, stack_info ));
}
