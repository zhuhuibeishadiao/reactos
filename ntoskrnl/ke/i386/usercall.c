/* $Id: usercall.c,v 1.31 2004/11/10 02:51:00 ion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/usercall.c
 * PURPOSE:         2E interrupt handler
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                  ???
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
KiSystemCallHook(ULONG Nr, ...)
{
#if 0
   va_list ap;
   ULONG i;

   va_start(ap, Nr);

   DbgPrint("%x/%d ", KeServiceDescriptorTable[0].SSDT[Nr].SysCallPtr, Nr);
   DbgPrint("%x (", KeServiceDescriptorTable[0].SSPT[Nr].ParamBytes);
   for (i = 0; i < KeServiceDescriptorTable[0].SSPT[Nr].ParamBytes / 4; i++)
     {
	DbgPrint("%x, ", va_arg(ap, ULONG));
     }
   DbgPrint(")\n");
   ASSERT_IRQL(PASSIVE_LEVEL);
   va_end(ap);
#endif
}

VOID
KiAfterSystemCallHook(PKTRAP_FRAME TrapFrame)
{
  if (KeGetCurrentThread()->Alerted[1] != 0 && TrapFrame->Cs != KERNEL_CS)
    {
      KiDeliverApc(KernelMode, NULL, NULL);
    }
  if (KeGetCurrentThread()->Alerted[0] != 0 && TrapFrame->Cs != KERNEL_CS)
    {
      KiDeliverApc(UserMode, NULL, TrapFrame);
    }
}


VOID
KiServiceCheck (ULONG Nr)
{
  PETHREAD Thread;

  Thread = PsGetCurrentThread();

#if 0
  DbgPrint ("KiServiceCheck(%p) called\n", Thread);
  DbgPrint ("Service %d (%p)\n", Nr, KeServiceDescriptorTableShadow[1].SSDT[Nr].SysCallPtr);
#endif

  if (Thread->Tcb.ServiceTable != KeServiceDescriptorTableShadow)
    {
#if 0
      DbgPrint ("Initialize Win32 thread\n");
#endif

      PsInitWin32Thread (Thread);

      Thread->Tcb.ServiceTable = KeServiceDescriptorTableShadow;
    }
}

// This function should be used by win32k.sys to add its own user32/gdi32 services
// TableIndex is 0 based
// ServiceCountTable its not used at the moment
/*
 * @implemented
 */
BOOLEAN STDCALL
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	)
{
    if (TableIndex > SSDT_MAX_ENTRIES - 1)
        return FALSE;

    /* check if descriptor table entry is free */
    if ((KeServiceDescriptorTable[TableIndex].SSDT != NULL) ||
        (KeServiceDescriptorTableShadow[TableIndex].SSDT != NULL))
        return FALSE;

    /* initialize the shadow service descriptor table */
    KeServiceDescriptorTableShadow[TableIndex].SSDT = SSDT;
    KeServiceDescriptorTableShadow[TableIndex].SSPT = SSPT;
    KeServiceDescriptorTableShadow[TableIndex].NumberOfServices = NumberOfServices;
    KeServiceDescriptorTableShadow[TableIndex].ServiceCounterTable = ServiceCounterTable;

    /* initialize the service descriptor table (not for win32k services) */
    if (TableIndex != 1)
    {
        KeServiceDescriptorTable[TableIndex].SSDT = SSDT;
        KeServiceDescriptorTable[TableIndex].SSPT = SSPT;
        KeServiceDescriptorTable[TableIndex].NumberOfServices = NumberOfServices;
        KeServiceDescriptorTable[TableIndex].ServiceCounterTable = ServiceCounterTable;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
KeRemoveSystemServiceTable(
    IN PUCHAR Number
)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
KeUserModeCallback(
    IN ULONG	FunctionID,
    IN PVOID	InputBuffer,
    IN ULONG	InputLength,
    OUT PVOID	*OutputBuffer,
    OUT PULONG	OutputLength
)
{
	UNIMPLEMENTED;
	return 0;
}

/* EOF */
