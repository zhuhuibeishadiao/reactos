/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Possible implementation of APCs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Unchecked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  12/11/99:  Phillip Susi: Reworked the APC code
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);

#define TAG_KAPC     TAG('K', 'A', 'P', 'C')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeApc(
	IN PKAPC  Apc,
	IN PKTHREAD  Thread,
	IN KAPC_ENVIRONMENT  TargetEnvironment,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine OPTIONAL,
	IN PKNORMAL_ROUTINE  NormalRoutine,
	IN KPROCESSOR_MODE  Mode,
	IN PVOID  Context)
/*
 * FUNCTION: Initialize an APC object
 * ARGUMENTS:
 *       Apc = Pointer to the APC object to initialized
 *       Thread = Thread the APC is to be delivered to
 *       TargetEnvironment = APC environment to use
 *       KernelRoutine = Routine to be called for a kernel-mode APC
 *       RundownRoutine = Routine to be called if the thread has exited with
 *                        the APC being executed
 *       NormalRoutine = Routine to be called for a user-mode APC
 *       Mode = APC mode
 *       Context = Parameter to be passed to the APC routine
 */
{
	DPRINT ("KeInitializeApc(Apc %x, Thread %x, Environment %d, "
		"KernelRoutine %x, RundownRoutine %x, NormalRoutine %x, Mode %d, "
		"Context %x)\n",Apc,Thread,TargetEnvironment,KernelRoutine,RundownRoutine,
		NormalRoutine,Mode,Context);

	/* Set up the basic APC Structure Data */
	RtlZeroMemory(Apc, sizeof(KAPC));
	Apc->Type = KApc;
	Apc->Size = sizeof(KAPC);
	
	/* Set the Environment */
	if (TargetEnvironment == CurrentApcEnvironment) {
		Apc->ApcStateIndex = Thread->ApcStateIndex;
	} else {
		Apc->ApcStateIndex = TargetEnvironment;
	}
	
	/* Set the Thread and Routines */
	Apc->Thread = Thread;
	Apc->KernelRoutine = KernelRoutine;
	Apc->RundownRoutine = RundownRoutine;
	Apc->NormalRoutine = NormalRoutine;
   
	/* Check if this is a Special APC, in which case we use KernelMode and no Context */
	if (ARGUMENT_PRESENT(NormalRoutine)) {
		Apc->ApcMode = Mode;
		Apc->NormalContext = Context;
	} else {
		Apc->ApcMode = KernelMode;
	}  
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeInsertQueueApc (PKAPC	Apc,
		  PVOID	SystemArgument1,
		  PVOID	SystemArgument2,
		  KPRIORITY PriorityBoost)
/*
 * FUNCTION: Queues an APC for execution
 * ARGUMENTS:
 *         Apc = APC to be queued
 *         SystemArgument[1-2] = Arguments we ignore and simply pass on.
 *         PriorityBoost = Priority Boost to give to the Thread
 */
{
 	KIRQL OldIrql;
	PKTHREAD Thread;
	PLIST_ENTRY ApcListEntry;
	PKAPC QueuedApc;
   
	DPRINT ("KeInsertQueueApc(Apc %x, SystemArgument1 %x, "
		"SystemArgument2 %x)\n",Apc,SystemArgument1,
		SystemArgument2);

	/* FIXME: Implement Dispatcher Lock */
	OldIrql = KeRaiseIrqlToDpcLevel();
	
	/* Get the Thread specified in the APC */
	Thread = Apc->Thread;
	   
	/* Make sure the thread allows APC Queues */
	if (Thread->ApcQueueable == FALSE) {
		DPRINT("Thread doesn't allow APC Queues\n");
		KeLowerIrql(OldIrql);
		return FALSE;
	}
	
	/* Set the System Arguments */
	Apc->SystemArgument1 = SystemArgument1;
	Apc->SystemArgument2 = SystemArgument2;

	/* Don't do anything if the APC is already inserted */
	if (Apc->Inserted) {
		KeLowerIrql(OldIrql);	
		return FALSE;
	}
	
	/* Three scenarios: 
	   1) Kernel APC with Normal Routine or User APC = Put it at the end of the List
	   2) User APC which is PsExitSpecialApc = Put it at the front of the List
	   3) Kernel APC without Normal Routine = Put it at the end of the No-Normal Routine Kernel APC list
	*/
	if ((Apc->ApcMode != KernelMode) && (Apc->KernelRoutine == (PKKERNEL_ROUTINE)PsExitSpecialApc)) {
		DPRINT ("Inserting the Process Exit APC into the Queue\n");
		Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->UserApcPending = TRUE;
		InsertHeadList(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode],
			       &Apc->ApcListEntry);
	} else if (Apc->NormalRoutine == NULL) {
		DPRINT ("Inserting Special APC %x into the Queue\n", Apc);
		for (ApcListEntry = Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode].Flink;
		     ApcListEntry != &Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode];
		     ApcListEntry = ApcListEntry->Flink) {

			QueuedApc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
			if (Apc->NormalRoutine != NULL) break;
		}
		
		/* We found the first "Normal" APC, so write right before it */
		ApcListEntry = ApcListEntry->Blink;
		InsertHeadList(ApcListEntry, &Apc->ApcListEntry);
	} else {
		DPRINT ("Inserting Normal APC %x into the %x Queue\n", Apc, Apc->ApcMode);
		InsertTailList(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode],
			       &Apc->ApcListEntry);
	}
	
	/* Confirm Insertion */	
	Apc->Inserted = TRUE;

	/* Three possibilites here again:
	   1) Kernel APC, The thread is Running: Request an Interrupt
	   2) Kernel APC, The Thread is Waiting at PASSIVE_LEVEL and APCs are enabled and not in progress: Unwait the Thread
	   3) User APC, Unwait the Thread if it is alertable
	*/ 
	if (Apc->ApcMode == KernelMode) { 
		Thread->ApcState.KernelApcPending = TRUE;
		if (Thread->State == THREAD_STATE_RUNNING) { 
			/* FIXME: Use IPI */
			DPRINT ("Requesting APC Interrupt for Running Thread \n");
			HalRequestSoftwareInterrupt(APC_LEVEL);
		} else if ((Thread->WaitIrql < APC_LEVEL) && (Apc->NormalRoutine == NULL)) {
			DPRINT ("Waking up Thread for Kernel-Mode APC Delivery \n");
			KeRemoveAllWaitsThread(CONTAINING_RECORD(Thread, ETHREAD, Tcb),
					       STATUS_KERNEL_APC,
					       TRUE);
		}
	} else if ((Thread->WaitMode == UserMode) && (Thread->Alertable)) {
		DPRINT ("Waking up Thread for User-Mode APC Delivery \n");
		Thread->ApcState.UserApcPending = TRUE;
		KeRemoveAllWaitsThread(CONTAINING_RECORD(Thread, ETHREAD, Tcb),
				       STATUS_USER_APC,
				       TRUE);
	}

	/* Return Sucess if we are here */
	KeLowerIrql(OldIrql);
	return TRUE;
}

BOOLEAN STDCALL
KeRemoveQueueApc (PKAPC Apc)
/*
 * FUNCTION: Removes APC object from the apc queue
 * ARGUMENTS:
 *          Apc = APC to remove
 * RETURNS: TRUE if the APC was in the queue
 *          FALSE otherwise
 * NOTE: This function is not exported.
 */
{
	KIRQL OldIrql;
	PKTHREAD Thread = Apc->Thread;

	DPRINT("KeRemoveQueueApc called for APC: %x \n", Apc);
	
	/* FIXME: Implement Dispatcher Lock */
	OldIrql = KeRaiseIrqlToDpcLevel();
	KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
	
	/* Remove it from the Queue if it's inserted */
	if (!Apc->Inserted == FALSE) {
		RemoveEntryList(&Apc->ApcListEntry);
		Apc->Inserted = FALSE;
		
		/* If the Queue is completely empty, then no more APCs are pending */
		if (IsListEmpty(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode])) {
			if (Apc->ApcMode == KernelMode) {
				Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->KernelApcPending = FALSE;
			} else {
				Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->UserApcPending = FALSE;
			}
		}
	} else {
		KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
		KeLowerIrql(OldIrql);
		return(FALSE);
	}
	
	/* Restore IRQL and Return */
	KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
	KeLowerIrql(OldIrql);
	return(TRUE);
}

BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
{
	KIRQL OldIrql;
	PKTHREAD Thread = KeGetCurrentThread();
	BOOLEAN OldState;
   
	/* FIXME: Implement Dispatcher Lock */
	OldIrql = KeRaiseIrqlToDpcLevel();
	KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
	
	OldState = Thread->Alerted[(int)AlertMode];
	
	/* If the Thread is Alerted, Clear it */
	if (OldState) {
		Thread->Alerted[(int)AlertMode] = FALSE;	
	} else if ((AlertMode == UserMode) && (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))) {
		/* If the mode is User and the Queue isn't empty, set Pending */
		Thread->ApcState.UserApcPending = TRUE;
	}
	
	/* FIXME: Implement Dispatcher Lock */
	KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
	KeLowerIrql(OldIrql);
	return OldState;
}

/*
 * @implemented
 */
VOID 
STDCALL
KiDeliverApc(KPROCESSOR_MODE PreviousMode,
             PVOID Reserved,
             PKTRAP_FRAME TrapFrame)
/*
 * FUNCTION: Deliver an APC to the current thread.
 * NOTES: This is called from the IRQL switching code if the current thread
 * is returning from an IRQL greater than or equal to APC_LEVEL to 
 * PASSIVE_LEVEL and there are kernel-mode APCs pending. This means any
 * pending APCs will be delivered after a thread gets a new quantum and
 * after it wakes from a wait. Note that the TrapFrame is only valid if
 * the previous mode is User.
 */
{
	PKTHREAD Thread = KeGetCurrentThread();
	PLIST_ENTRY ApcListEntry;
	PKAPC Apc;
	KIRQL OldIrql;
	PKKERNEL_ROUTINE KernelRoutine;
	PVOID NormalContext;
	PKNORMAL_ROUTINE NormalRoutine;
	PVOID SystemArgument1;
	PVOID SystemArgument2;

	/* Lock the APC Queue and Raise IRQL to Synch */
	KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);

	/* Clear APC Pending */
	Thread->ApcState.KernelApcPending = FALSE;
	
	/* Do the Kernel APCs first */
	while (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) {
	
		/* Get the next Entry */
		ApcListEntry = Thread->ApcState.ApcListHead[KernelMode].Flink;
		Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
		
		/* Save Parameters so that it's safe to free the Object in Kernel Routine*/
		NormalRoutine   = Apc->NormalRoutine;
		KernelRoutine   = Apc->KernelRoutine;
		NormalContext   = Apc->NormalContext;
		SystemArgument1 = Apc->SystemArgument1;
		SystemArgument2 = Apc->SystemArgument2;
       
		/* Special APC */
		if (NormalRoutine == NULL) {
			/* Remove the APC from the list */
			Apc->Inserted = FALSE;
			RemoveEntryList(ApcListEntry);
			
			/* Go back to APC_LEVEL */
			KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
			
			/* Call the Special APC */
			DPRINT("Delivering a Special APC: %x\n", Apc);
			KernelRoutine(Apc,
				      &NormalRoutine,
				      &NormalContext,
				      &SystemArgument1,
				      &SystemArgument2);

			/* Raise IRQL and Lock again */
			KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
		} else {
			 /* Normal Kernel APC */
			if (Thread->ApcState.KernelApcInProgress || Thread->KernelApcDisable) {
				KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
				return;
			}
			
			/* Dequeue the APC */
			RemoveEntryList(ApcListEntry);
			Apc->Inserted = FALSE;
			
			/* Go back to APC_LEVEL */
			KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
			
			/* Call the Kernel APC */
			DPRINT("Delivering a Normal APC: %x\n", Apc);
			KernelRoutine(Apc,
				      &NormalRoutine,
				      &NormalContext,
				      &SystemArgument1,
				      &SystemArgument2);
			
			/* If There still is a Normal Routine, then we need to call this at PASSIVE_LEVEL */
			if (NormalRoutine != NULL) {
				/* At Passive Level, this APC can be prempted by a Special APC */
				Thread->ApcState.KernelApcInProgress = TRUE;
				KeLowerIrql(PASSIVE_LEVEL);
				
				/* Call and Raise IRQ back to APC_LEVEL */
				DPRINT("Calling the Normal Routine for a Normal APC: %x\n", Apc);
				NormalRoutine(&NormalContext, &SystemArgument1, &SystemArgument2);
				KeRaiseIrql(APC_LEVEL, &OldIrql);
			}

			/* Raise IRQL and Lock again */
			KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);
			Thread->ApcState.KernelApcInProgress = FALSE;
		}
	}
	
	/* Now we do the User APCs */
	if ((!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) &&
			 (PreviousMode == UserMode) &&
			 (Thread->ApcState.UserApcPending == TRUE)) {
			 
		/* It's not pending anymore */
		Thread->ApcState.UserApcPending = FALSE;

		/* Get the APC Object */
		ApcListEntry = Thread->ApcState.ApcListHead[UserMode].Flink;
		Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);
		
		/* Save Parameters so that it's safe to free the Object in Kernel Routine*/
		NormalRoutine   = Apc->NormalRoutine;
		KernelRoutine   = Apc->KernelRoutine;
		NormalContext   = Apc->NormalContext;
		SystemArgument1 = Apc->SystemArgument1;
		SystemArgument2 = Apc->SystemArgument2; 
		
		/* Remove the APC from Queue, restore IRQL and call the APC */
		RemoveEntryList(ApcListEntry);
		KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
		DPRINT("Calling the Kernel Routine for for a User APC: %x\n", Apc);
		KernelRoutine(Apc,
			      &NormalRoutine,
			      &NormalContext,
			      &SystemArgument1,
			      &SystemArgument2);

		if (NormalRoutine == NULL) {
			/* Check if more User APCs are Pending */
			KeTestAlertThread(UserMode);
		} else {
			/* Set up the Trap Frame and prepare for Execution in NTDLL.DLL */
			DPRINT("Delivering a User APC: %x\n", Apc);
			KiInitializeUserApc(Reserved, 
					    TrapFrame,
					    NormalRoutine,
					    NormalContext,
					    SystemArgument1,
					    SystemArgument2);
		}
	} else {
		/* Go back to APC_LEVEL */
		KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
	}
}

VOID KiInitializeUserApc(IN PVOID Reserved,
			 IN PKTRAP_FRAME TrapFrame,
			 IN PKNORMAL_ROUTINE NormalRoutine,
			 IN PVOID NormalContext,
			 IN PVOID SystemArgument1,
			 IN PVOID SystemArgument2)  
/*
 * FUNCTION: Prepares the Context for a user mode APC through ntdll.dll
 */
{
	PCONTEXT Context;
	PULONG Esp;

	DPRINT("KiInitializeUserApc(TrapFrame %x/%x)\n", TrapFrame, KeGetCurrentThread()->TrapFrame);
   
	/*
	 * Save the thread's current context (in other words the registers
	 * that will be restored when it returns to user mode) so the
	 * APC dispatcher can restore them later
	 */
	Context = (PCONTEXT)(((PUCHAR)TrapFrame->Esp) - sizeof(CONTEXT));
	RtlZeroMemory(Context, sizeof(CONTEXT));
	Context->ContextFlags = CONTEXT_FULL;
	Context->SegGs = TrapFrame->Gs;
	Context->SegFs = TrapFrame->Fs;
	Context->SegEs = TrapFrame->Es;
	Context->SegDs = TrapFrame->Ds;
	Context->Edi = TrapFrame->Edi;
	Context->Esi = TrapFrame->Esi;
	Context->Ebx = TrapFrame->Ebx;
	Context->Edx = TrapFrame->Edx;
	Context->Ecx = TrapFrame->Ecx;
	Context->Eax = TrapFrame->Eax;
	Context->Ebp = TrapFrame->Ebp;
	Context->Eip = TrapFrame->Eip;
	Context->SegCs = TrapFrame->Cs;
	Context->EFlags = TrapFrame->Eflags;
	Context->Esp = TrapFrame->Esp;
	Context->SegSs = TrapFrame->Ss;
       
	/*
	 * Setup the trap frame so the thread will start executing at the
	 * APC Dispatcher when it returns to user-mode
	 */
	Esp = (PULONG)(((PUCHAR)TrapFrame->Esp) - (sizeof(CONTEXT) + (6 * sizeof(ULONG))));
	Esp[0] = 0xdeadbeef;
	Esp[1] = (ULONG)NormalRoutine;
	Esp[2] = (ULONG)NormalContext;
	Esp[3] = (ULONG)SystemArgument1;
	Esp[4] = (ULONG)SystemArgument2;
	Esp[5] = (ULONG)Context;
	TrapFrame->Eip = (ULONG)LdrpGetSystemDllApcDispatcher();
	TrapFrame->Esp = (ULONG)Esp;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeAreApcsDisabled(
	VOID
	)
{
 	return KeGetCurrentThread()->KernelApcDisable ? FALSE : TRUE;
}

VOID STDCALL
NtQueueApcRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}

VOID STDCALL
NtQueueApcKernelRoutine(PKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
{
   ExFreePool(Apc);
}

NTSTATUS STDCALL
NtQueueApcThread(HANDLE			ThreadHandle,
		 PKNORMAL_ROUTINE	ApcRoutine,
		 PVOID			NormalContext,
		 PVOID			SystemArgument1,
		 PVOID			SystemArgument2)
{
   PKAPC Apc;
   PETHREAD Thread;
   NTSTATUS Status;
   PVOID ThreadVar;

   ThreadVar = &Thread;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_ALL_ACCESS, /* FIXME */
				      PsThreadType,
				      UserMode,
				      &ThreadVar,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_KAPC);
   if (Apc == NULL)
     {
	ObDereferenceObject(Thread);
	return(STATUS_NO_MEMORY);
     }
   
   KeInitializeApc(Apc,
		   &Thread->Tcb,
                   OriginalApcEnvironment,
		   NtQueueApcKernelRoutine,
		   NtQueueApcRundownRoutine,
		   ApcRoutine,
		   UserMode,
		   NormalContext);
   KeInsertQueueApc(Apc,
		    SystemArgument1,
		    SystemArgument2,
		    IO_NO_INCREMENT);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtTestAlert(VOID)
{
	if (KeTestAlertThread(KeGetPreviousMode())) {
		return STATUS_ALERTED;
	} else {
		return STATUS_SUCCESS;
	}
}

VOID INIT_FUNCTION
PiInitApcManagement(VOID)
{
}

static VOID FASTCALL
RepairList(PLIST_ENTRY Original, PLIST_ENTRY Copy, int Mode)
{
  if (IsListEmpty(&Original[Mode]))
    {
      InitializeListHead(&Copy[Mode]);
    }
  else
    {
      Copy[Mode].Flink->Blink = &Copy[Mode];
      Copy[Mode].Blink->Flink = &Copy[Mode];
    }
}


VOID FASTCALL
KiSwapApcEnvironment(
  PKTHREAD Thread,
  PKPROCESS NewProcess)
{
  if (Thread->ApcStateIndex == AttachedApcEnvironment)
  {
    /* NewProcess must be the same as in the original-environment */
    ASSERT(NewProcess == Thread->ApcStatePointer[OriginalApcEnvironment]->Process);

    /*    
    FIXME: Deliver any pending apc's queued to the attached environment before 
    switching back to the original environment.
    This is not crucial thou, since i don't think we'll ever target apc's
    to attached environments (or?)...   
    Remove the following asserts if implementing this.
    -Gunnar
    */

    /* we don't support targeting apc's at attached-environments (yet)... */
    ASSERT(IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]));
    ASSERT(IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]));
    ASSERT(Thread->ApcState.KernelApcInProgress == FALSE);
    ASSERT(Thread->ApcState.KernelApcPending == FALSE);
    ASSERT(Thread->ApcState.UserApcPending == FALSE);
    
    /* restore backup of original environment */
    Thread->ApcState = Thread->SavedApcState;
    /* repair lists */
    RepairList(Thread->SavedApcState.ApcListHead, Thread->ApcState.ApcListHead,
               KernelMode);
    RepairList(Thread->SavedApcState.ApcListHead, Thread->ApcState.ApcListHead,
               UserMode);

    /* update environment pointers */
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;

    /* update current-environment index */
    Thread->ApcStateIndex = OriginalApcEnvironment;
  }
  else if (Thread->ApcStateIndex == OriginalApcEnvironment)
  {
    /* backup original environment */
    Thread->SavedApcState = Thread->ApcState;
    /* repair lists */
    RepairList(Thread->ApcState.ApcListHead, Thread->SavedApcState.ApcListHead,
               KernelMode);
    RepairList(Thread->ApcState.ApcListHead, Thread->SavedApcState.ApcListHead,
               UserMode);

    /*
    FIXME: Is it possible to target an apc to an attached environment even if the 
    thread is not currently attached???? If so, then this is bougus since it 
    reinitializes the attached apc environment then located in SavedApcState.
    -Gunnar
    */

    /* setup a fresh new attached environment */
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = NewProcess;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;

    /* update environment pointers */
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;

    /* update current-environment index */
    Thread->ApcStateIndex = AttachedApcEnvironment;
  }
  else
  {
    /* FIXME: Is this the correct bug code? */
    KEBUGCHECK(APC_INDEX_MISMATCH);
  }
}

