#pragma once

typedef struct _ACCELERATOR_TABLE
{
  HEAD head;
  int Count;
  LPACCEL Table;
} ACCELERATOR_TABLE, *PACCELERATOR_TABLE;

INIT_FUNCTION NTSTATUS NTAPI InitAcceleratorImpl(VOID);
NTSTATUS FASTCALL CleanupAcceleratorImpl(VOID);
PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL);
