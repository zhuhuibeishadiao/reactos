/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cpl/system/sysdm.c
 * PURPOSE:         ReactOS System Control Panel
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>

#include "resource.h"
#include "access.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
INT_PTR CALLBACK DisplayPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK KeyboardPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MousePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
  {IDI_CPLACCESS, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

static void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}

/* Property Sheet Callback */
int CALLBACK
PropSheetProc(
	HWND hwndDlg,
	UINT uMsg,
	LPARAM lParam
)
{
  UNREFERENCED_PARAMETER(hwndDlg)
  switch(uMsg)
  {
    case PSCB_BUTTONPRESSED:
      switch(lParam)
      {
        case PSBTN_OK: /* OK */
          break;
        case PSBTN_CANCEL: /* Cancel */
          break;
        case PSBTN_APPLYNOW: /* Apply now */
          break;
        case PSBTN_FINISH: /* Close */
          break;
        default:
          return FALSE;
      }
      break;
      
    case PSCB_INITIALIZED:
      break;
  }
  return TRUE;
}

/* First Applet */

LONG CALLBACK
SystemApplet(VOID)
{
  PROPSHEETPAGE psp[5];
  PROPSHEETHEADER psh;
  TCHAR Caption[1024];
  
  LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
  
  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLACCESS));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;
  psh.pfnCallback = PropSheetProc;
  
  InitPropSheetPage(&psp[0], IDD_PROPPAGEKEYBOARD, (DLGPROC) KeyboardPageProc);
  InitPropSheetPage(&psp[1], IDD_PROPPAGESOUND, (DLGPROC) SoundPageProc);
  InitPropSheetPage(&psp[2], IDD_PROPPAGEDISPLAY, (DLGPROC) DisplayPageProc);
  InitPropSheetPage(&psp[3], IDD_PROPPAGEMOUSE, (DLGPROC) MousePageProc);
  InitPropSheetPage(&psp[4], IDD_PROPPAGEGENERAL, (DLGPROC) GeneralPageProc);
  
  return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(
	HWND hwndCPl,
	UINT uMsg,
	LPARAM lParam1,
	LPARAM lParam2)
{
  int i = (int)lParam1;
  UNREFERENCED_PARAMETER(hwndCPl)

  switch(uMsg)
  {
    case CPL_INIT:
    {
      return TRUE;
    }
    case CPL_GETCOUNT:
    {
      return NUM_APPLETS;
    }
    case CPL_INQUIRE:
    {
      CPLINFO *CPlInfo = (CPLINFO*)lParam2;
      CPlInfo->lData = 0;
      CPlInfo->idIcon = Applets[i].idIcon;
      CPlInfo->idName = Applets[i].idName;
      CPlInfo->idInfo = Applets[i].idDescription;
      break;
    }
    case CPL_DBLCLK:
    {
      Applets[i].AppletProc();
      break;
    }
  }
  return FALSE;
}


BOOL STDCALL
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
{
  UNREFERENCED_PARAMETER(lpvReserved)
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
      hApplet = hinstDLL;
      break;
  }
  return TRUE;
}

