/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_main.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */


#include "rosdraw.h"

HRESULT WINAPI
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface,
                                REFIID id,
                                LPVOID *obj)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    /* FIXME
       the D3D object can be optained from here
       Direct3D7
    */
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        /* DirectDraw7 Vtable */
        This->lpVtbl = &DirectDraw7_Vtable;
        This->lpLcl->dwLocalFlags = This->lpLcl->dwLocalFlags + DDRAWILCL_DIRECTDRAW7;
        *obj = &This->lpVtbl;
    }
    else
    {
        *obj = NULL;
        DX_STUB_str("E_NOINTERFACE");
        return E_NOINTERFACE;
    }

    Main_DirectDraw_AddRef(iface);
    DX_STUB_str("DD_OK");
    return DD_OK;
}

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    return This->dwIntRefCnt;
}

ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->lpLcl->dwLocalRefCnt--;
        This->dwIntRefCnt--;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt--;
        }

        if ( This->lpLcl->lpGbl->dwRefCnt == 0)
        {
            // set resoltion back to the one in registry
            /*if(This->cooperative_level & DDSCL_EXCLUSIVE)
            {
                ChangeDisplaySettings(NULL, 0);
            }*/

            Cleanup(iface);
            return 0;
        }
    }
    return This->dwIntRefCnt;
}

HRESULT WINAPI
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    return DD_OK; // not implemented in ms ddraw either
}

HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD dwTotal, LPDWORD dwFree)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    // There is no HEL implentation of this api
    if (!(This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY))
    {
        return DDERR_NODIRECTDRAWHW;
    }

    if ((!dwTotal && !dwFree) || !ddscaps)
    {
        return DDERR_INVALIDPARAMS;
    }

    DDHAL_GETAVAILDRIVERMEMORYDATA  memdata;
    memdata.lpDD = This->lpLcl->lpGbl;
    memdata.ddRVal = DDERR_INVALIDPARAMS;
    memcpy(&memdata.DDSCaps, ddscaps, sizeof(DDSCAPS2));

    if (This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks.GetAvailDriverMemory(&memdata) == DDHAL_DRIVER_NOTHANDLED)
		return DDERR_NODIRECTDRAWHW;

    if (dwTotal)
       *dwTotal = memdata.dwTotal;

    if (dwFree)
       *dwFree = memdata.dwFree;

    return memdata.ddRVal;
}

HRESULT WINAPI 
Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret;
   DDSURFACEDESC2 dd_desc_v2;
   LPDDRAWI_DIRECTDRAW_INT dd_int;
   LPDDRAWI_DIRECTDRAW_LCL dd_lcl;
   LPDDRAWI_DIRECTDRAW_GBL dd_gbl;

   DX_WINDBG_trace();

   /* FIXME vaildate input pointers or warp everthing with SEH */

   if (pUnkOuter)
   {
       /* FIXME send back right return code */
        return DDERR_GENERIC;
   }

   if (pDDSD)
   {
        /* FIXME send back right return code */
        return DDERR_GENERIC;
   }

   EnterCriticalSection(&ddcs);

   ret = DDERR_GENERIC;

   dd_int = (LPDDRAWI_DIRECTDRAW_INT)iface;
   dd_lcl = dd_int->lpLcl;
   dd_gbl = dd_lcl->lpGbl;

   if (dd_lcl->dwLocalFlags == 0)
   {
       LeaveCriticalSection(&ddcs);
       /* FIXME send back right return code */
       return  DDERR_GENERIC;
   }

   if (pDDSD->dwSize == sizeof(DDSURFACEDESC))
   {
       CopyDDSurfDescToDDSurfDesc2(&dd_desc_v2, (LPDDSURFACEDESC)pDDSD);
   }
   else if (pDDSD->dwSize == sizeof(DDSURFACEDESC2))
   {
       RtlCopyMemory(&dd_desc_v2, pDDSD,sizeof(DDSURFACEDESC2));
   }
   else
   {
       LeaveCriticalSection(&ddcs);
       return  DDERR_INVALIDPARAMS;
   }

   /* Check if this process belongs to this ddraw object */
   if ( dd_int->lpLcl->dwProcessId != GetCurrentProcessId() )
   {
       /* FIXME send back right return code */
        return  DDERR_GENERIC;
   }

  ret = Internal_CreateSurface(dd_int,&dd_desc_v2, ppSurf,pUnkOuter);

  LeaveCriticalSection(&ddcs);
  return ret;
}


IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
