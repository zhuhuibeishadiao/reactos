#pragma once

#ifdef INTERFACE
#undef INTERFACE
#endif

/* FIXME: Ugly hack!!! FIX ASAP! Move to uuid! */
static const GUID VID_HACK_LargeIcons = {0x0057D0E0, 0x3573, 0x11CF, {0xAE, 0x69, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62}};
#define VID_LargeIcons VID_HACK_LargeIcons

static const GUID IID_HACK_IDeskBarClient = {0xEB0FE175, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBarClient IID_HACK_IDeskBarClient
static const GUID IID_HACK_IDeskBar = {0xEB0FE173, 0x1A3A, 0x11D0, {0x89, 0xB3, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xAC}};
#define IID_IDeskBar IID_HACK_IDeskBar

static const GUID SID_HACK_SMenuPopup = {0xD1E7AFEB,0x6A2E,0x11D0,{0x8C,0x78,0x00,0xC0,0x4F,0xD9,0x18,0xB4}};
#define SID_SMenuPopup SID_HACK_SMenuPopup

#ifdef COBJMACROS
#define IDeskBarClient_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IDeskBarClient_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IDeskBarClient_Release(T) (T)->lpVtbl->Release(T)
#define IDeskBarClient_GetWindow(T,a) (T)->lpVtbl->GetWindow(T,a)
#define IDeskBarClient_ContextSensitiveHelp(T,a) (T)->lpVtbl->ContextSensitiveHelp(T,a)
#define IDeskBarClient_SetDeskBarSite(T,a) (T)->lpVtbl->SetDeskBarSite(T,a)
#define IDeskBarClient_SetModeDBC(T,a) (T)->lpVtbl->SetModeDBC(T,a)
#define IDeskBarClient_UIActivateDBC(T,a) (T)->lpVtbl->UIActivateDBC(T,a)
#define IDeskBarClient_GetSize(T,a,b) (T)->lpVtbl->GetSize(T,a,b)
#endif
