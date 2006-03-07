/*
 * ReactOS Calendar Control
 * Copyright (C) 2006 Thomas Weidenmueller <w3seek@reactos.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <timedate.h>

static const TCHAR szMonthCalWndClass[] = TEXT("MonthCalWnd");

#define MONTHCAL_HEADERBG   COLOR_INACTIVECAPTION
#define MONTHCAL_HEADERFG   COLOR_INACTIVECAPTIONTEXT
#define MONTHCAL_CTRLBG     COLOR_WINDOW
#define MONTHCAL_CTRLFG     COLOR_WINDOWTEXT
#define MONTHCAL_SELBG      COLOR_ACTIVECAPTION
#define MONTHCAL_SELFG      COLOR_CAPTIONTEXT

#define ID_DAYTIMER 1

typedef struct _MONTHCALWND
{
    HWND hSelf;
    HWND hNotify;
    WORD Day;
    WORD Month;
    WORD Year;
    WORD FirstDayOfWeek;
    BYTE Days[6][7];
    TCHAR Week[7];
    SIZE CellSize;
    SIZE ClientSize;

    HFONT hFont;
    HBRUSH hbHeader;
    HBRUSH hbSelection;

    DWORD UIState;
    BOOL Changed : 1;
    BOOL DayTimerSet : 1;
    BOOL HasFocus : 1;
} MONTHCALWND, *PMONTHCALWND;

static LRESULT
MonthCalNotifyControlParent(IN PMONTHCALWND infoPtr,
                            IN UINT code,
                            IN OUT PVOID data)
{
    LRESULT Ret = 0;

    if (infoPtr->hNotify != NULL)
    {
        LPNMHDR pnmh = (LPNMHDR)data;

        pnmh->hwndFrom = infoPtr->hSelf;
        pnmh->idFrom = GetWindowLongPtr(infoPtr->hSelf,
                                        GWLP_ID);
        pnmh->code = code;

        Ret = SendMessage(infoPtr->hNotify,
                          WM_NOTIFY,
                          (WPARAM)pnmh->idFrom,
                          (LPARAM)pnmh);
    }
    
    return Ret;
}

static WORD
MonthCalMonthLength(IN WORD Month,
                    IN WORD Year)
{
    const BYTE MonthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};

    if(Month == 2)
        return MonthDays[Month - 1] + ((Year % 400 == 0) ? 1 : ((Year % 100 != 0) && (Year % 4 == 0)) ? 1 : 0);
    else
        return MonthDays[Month - 1];
}

static WORD
MonthCalWeekInMonth(IN WORD Day,
                    IN WORD DayOfWeek)
{
    return (Day - DayOfWeek + 5) / 7;
}

static WORD
MonthCalDayOfWeek(IN PMONTHCALWND infoPtr,
                  IN WORD Day,
                  IN WORD Month,
                  IN WORD Year)
{
    const BYTE DayOfWeek[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    WORD Ret;

    Year -= (Month < 3);
    Ret = (Year + (Year / 4) - (Year / 100) + (Year / 400) + DayOfWeek[Month - 1] + Day + 6) % 7;

    return (7 + Ret - infoPtr->FirstDayOfWeek) % 7;
}

static WORD
MonthCalFirstDayOfWeek(VOID)
{
    TCHAR szBuf[2] = {0};
    WORD Ret = 0;

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_IFIRSTDAYOFWEEK,
                      szBuf,
                      sizeof(szBuf) / sizeof(szBuf[0])) != 0)
    {
        Ret = (WORD)(szBuf[0] - TEXT('0'));
    }

    return Ret;
}

static BOOL
MonthCalValidDate(IN WORD Day,
                  IN WORD Month,
                  IN WORD Year)
{
    if (Month < 1 || Month > 12 ||
        Day == 0 || Day > MonthCalMonthLength(Month,
                                              Year) ||
        Year < 1980 || Year > 2099)
    {
        return FALSE;
    }

    return TRUE;
}

static VOID
MonthCalUpdate(IN PMONTHCALWND infoPtr)
{
    PBYTE pDay, pDayEnd;
    WORD DayOfWeek, MonthLength, d = 0;
    SIZE NewCellSize;
    BOOL RepaintHeader = FALSE;

    NewCellSize.cx = infoPtr->ClientSize.cx / 7;
    NewCellSize.cy = infoPtr->ClientSize.cy / 7;

    if (infoPtr->CellSize.cx != NewCellSize.cx ||
        infoPtr->CellSize.cy != NewCellSize.cy);
    {
        infoPtr->CellSize = NewCellSize;
        RepaintHeader = TRUE;
    }

    /* update the days layout of the current month */
    ZeroMemory(infoPtr->Days,
               sizeof(infoPtr->Days));

    DayOfWeek = MonthCalDayOfWeek(infoPtr,
                                  1,
                                  infoPtr->Month,
                                  infoPtr->Year);

    MonthLength = MonthCalMonthLength(infoPtr->Month,
                                      infoPtr->Year);

    pDay = &infoPtr->Days[0][DayOfWeek];
    pDayEnd = pDay + MonthLength;
    while (pDay != pDayEnd)
    {
        *(pDay++) = ++d;
    }

    /* repaint the control */
    if (RepaintHeader)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }
    else
    {
        RECT rcClient;

        rcClient.left = 0;
        rcClient.top = infoPtr->CellSize.cy;
        rcClient.right = infoPtr->ClientSize.cx;
        rcClient.bottom = infoPtr->ClientSize.cy - rcClient.top;

        InvalidateRect(infoPtr->hSelf,
                       &rcClient,
                       TRUE);
    }
}

static VOID
MonthCalSetupDayTimer(IN PMONTHCALWND infoPtr)
{
    SYSTEMTIME LocalTime = {0};
    UINT uElapse;

    /* update the current date */
    GetLocalTime(&LocalTime);

    /* calculate the number of remaining milliseconds until midnight */
    uElapse = 1000 - (UINT)LocalTime.wMilliseconds;
    uElapse += (59 - (UINT)LocalTime.wSecond) * 1000;
    uElapse += (59 - (UINT)LocalTime.wMinute) * 60 * 1000;
    uElapse += (23 - (UINT)LocalTime.wHour) * 60 * 60 * 1000;

    if (uElapse < USER_TIMER_MINIMUM || uElapse > USER_TIMER_MAXIMUM)
        uElapse = 1000;
    else
        uElapse += 100; /* Add a delay of 0.1 seconds */

    /* setup the new timer */
    if (SetTimer(infoPtr->hSelf,
                 ID_DAYTIMER,
                 uElapse,
                 NULL) != 0)
    {
        infoPtr->DayTimerSet = TRUE;
    }
}

static VOID
MonthCalReload(IN PMONTHCALWND infoPtr)
{
    TCHAR szBuf[64];
    UINT i;

    infoPtr->UIState = SendMessage(GetAncestor(infoPtr->hSelf,
                                               GA_PARENT),
                                   WM_QUERYUISTATE,
                                   0,
                                   0);

    /* cache the configuration */
    infoPtr->FirstDayOfWeek = MonthCalFirstDayOfWeek();

    infoPtr->hbHeader = GetSysColorBrush(MONTHCAL_HEADERBG);
    infoPtr->hbSelection = GetSysColorBrush(MONTHCAL_SELBG);

    for (i = 0;
         i < 7;
         i++)
    {
        if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                          LOCALE_SABBREVDAYNAME1 +
                              ((i + infoPtr->FirstDayOfWeek) % 7),
                          szBuf,
                          sizeof(szBuf) / sizeof(szBuf[0])) != 0)
        {
            infoPtr->Week[i] = szBuf[0];
        }
    }

    /* update the control */
    MonthCalUpdate(infoPtr);
}

static BOOL
MonthCalGetDayRect(IN PMONTHCALWND infoPtr,
                   IN WORD Day,
                   OUT RECT *rcCell)
{
    if (Day >= 1 && Day <= MonthCalMonthLength(infoPtr->Month,
                                               infoPtr->Year))
    {
        WORD DayOfWeek;

        DayOfWeek = MonthCalDayOfWeek(infoPtr,
                                      Day,
                                      infoPtr->Month,
                                      infoPtr->Year);

        rcCell->left = DayOfWeek * infoPtr->CellSize.cx;
        rcCell->top = (MonthCalWeekInMonth(Day,
                                           DayOfWeek) + 1) * infoPtr->CellSize.cy;
        rcCell->right = rcCell->left + infoPtr->CellSize.cx;
        rcCell->bottom = rcCell->top + infoPtr->CellSize.cy;

        return TRUE;
    }

    return FALSE;
}

static VOID
MonthCalChange(IN PMONTHCALWND infoPtr)
{
    infoPtr->Changed = TRUE;

    /* kill the day timer */
    if (infoPtr->DayTimerSet)
    {
        KillTimer(infoPtr->hSelf,
                  ID_DAYTIMER);
        infoPtr->DayTimerSet = FALSE;
    }
}


static BOOL
MonthCalSetDate(IN PMONTHCALWND infoPtr,
                IN WORD Day,
                IN WORD Month,
                IN WORD Year)
{
    NMMCCSELCHANGE sc;
    BOOL Ret = FALSE;

    sc.OldDay = infoPtr->Day;
    sc.OldMonth = infoPtr->Month;
    sc.OldYear = infoPtr->Year;
    sc.NewDay = Day;
    sc.NewMonth = Month;
    sc.NewYear = Year;

    /* notify the parent */
    if (!MonthCalNotifyControlParent(infoPtr,
                                     MCCN_SELCHANGE,
                                     &sc))
    {
        /* check if we actually need to update */
        if (infoPtr->Month != sc.NewMonth ||
            infoPtr->Year != sc.NewYear)
        {
            infoPtr->Day = sc.NewDay;
            infoPtr->Month = sc.NewMonth;
            infoPtr->Year = sc.NewYear;

            MonthCalChange(infoPtr);

            /* repaint the entire control */
            MonthCalUpdate(infoPtr);

            Ret = TRUE;
        }
        else if (infoPtr->Day != sc.NewDay)
        {
            RECT rcUpdate;

            infoPtr->Day = sc.NewDay;

            MonthCalChange(infoPtr);

            if (MonthCalGetDayRect(infoPtr,
                                   sc.OldDay,
                                   &rcUpdate))
            {
                /* repaint the day cells that need to be updated */
                InvalidateRect(infoPtr->hSelf,
                               &rcUpdate,
                               TRUE);
                if (MonthCalGetDayRect(infoPtr,
                                       sc.NewDay,
                                       &rcUpdate))
                {
                    InvalidateRect(infoPtr->hSelf,
                                   &rcUpdate,
                                   TRUE);
                }
            }

            Ret = TRUE;
        }
    }

    return Ret;
}

static VOID
MonthCalSetLocalTime(IN PMONTHCALWND infoPtr,
                     OUT SYSTEMTIME *Time)
{
    NMMCCAUTOUPDATE au;
    SYSTEMTIME LocalTime = {0};

    GetLocalTime(&LocalTime);

    au.SystemTime = LocalTime;
    if (!MonthCalNotifyControlParent(infoPtr,
                                     MCCN_AUTOUPDATE,
                                     &au))
    {
        if (MonthCalSetDate(infoPtr,
                            LocalTime.wDay,
                            LocalTime.wMonth,
                            LocalTime.wYear))
        {
            infoPtr->Changed = FALSE;
        }
    }

    /* kill the day timer */
    if (infoPtr->DayTimerSet)
    {
        KillTimer(infoPtr->hSelf,
                  ID_DAYTIMER);
        infoPtr->DayTimerSet = FALSE;
    }

    /* setup the new day timer */
    MonthCalSetupDayTimer(infoPtr);

    if (Time != NULL)
    {
        *Time = LocalTime;
    }
}

static VOID
MonthCalRepaintDay(IN PMONTHCALWND infoPtr,
                   IN WORD Day)
{
    RECT rcCell;

    if (MonthCalGetDayRect(infoPtr,
                           Day,
                           &rcCell))
    {
        InvalidateRect(infoPtr->hSelf,
                       &rcCell,
                       TRUE);
    }
}

static VOID
MonthCalPaint(IN PMONTHCALWND infoPtr,
              IN HDC hDC,
              IN LPRECT prcUpdate)
{
    UINT x, y;
    RECT rcCell;
    COLORREF crOldText, crOldCtrlText = CLR_INVALID;
    HFONT hOldFont;
    INT iOldBkMode;

    iOldBkMode = SetBkMode(hDC,
                           TRANSPARENT);
    hOldFont = (HFONT)SelectObject(hDC,
                                   infoPtr->hFont);

    for (y = prcUpdate->top / infoPtr->CellSize.cy;
         y <= prcUpdate->bottom / infoPtr->CellSize.cy && y < 7;
         y++)
    {
        rcCell.top = y * infoPtr->CellSize.cy;
        rcCell.bottom = rcCell.top + infoPtr->CellSize.cy;

        if (y == 0)
        {
            RECT rcHeader;

            /* paint the header */
            rcHeader.left = prcUpdate->left;
            rcHeader.top = rcCell.top;
            rcHeader.right = prcUpdate->right;
            rcHeader.bottom = rcCell.bottom;

            FillRect(hDC,
                     &rcHeader,
                     infoPtr->hbHeader);

            crOldText = SetTextColor(hDC,
                                     GetSysColor(MONTHCAL_HEADERFG));

            for (x = prcUpdate->left / infoPtr->CellSize.cx;
                 x <= prcUpdate->right / infoPtr->CellSize.cx && x < 7;
                 x++)
            {
                rcCell.left = x * infoPtr->CellSize.cx;
                rcCell.right = rcCell.left + infoPtr->CellSize.cx;

                /* write the first letter of each weekday */
                DrawText(hDC,
                         &infoPtr->Week[x],
                         1,
                         &rcCell,
                         DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_VCENTER);
            }

            SetTextColor(hDC,
                         crOldText);
        }
        else
        {
            if (crOldCtrlText == CLR_INVALID)
            {
                crOldCtrlText = SetTextColor(hDC,
                                             MONTHCAL_CTRLFG);
            }

            for (x = prcUpdate->left / infoPtr->CellSize.cx;
                 x <= prcUpdate->right / infoPtr->CellSize.cx && x < 7;
                 x++)
            {
                UINT Day = infoPtr->Days[y - 1][x];

                rcCell.left = x * infoPtr->CellSize.cx;
                rcCell.right = rcCell.left + infoPtr->CellSize.cx;

                /* write the day number */
                if (Day != 0 && Day < 100)
                {
                    TCHAR szDay[3];
                    INT szDayLen;
                    RECT rcText;
                    SIZE TextSize;

                    szDayLen = _stprintf(szDay,
                                         TEXT("%lu"),
                                         Day);

                    if (GetTextExtentPoint32(hDC,
                                             szDay,
                                             szDayLen,
                                             &TextSize))
                    {
                        RECT rcHighlight = {0};

                        rcText.left = rcCell.left + (infoPtr->CellSize.cx / 2) - (TextSize.cx / 2);
                        rcText.top = rcCell.top + (infoPtr->CellSize.cy / 2) - (TextSize.cy / 2);
                        rcText.right = rcText.left + TextSize.cx;
                        rcText.bottom = rcText.top + TextSize.cy;

                        if (Day == infoPtr->Day)
                        {
                            rcHighlight = rcText;

                            InflateRect(&rcHighlight,
                                        GetSystemMetrics(SM_CXFOCUSBORDER),
                                        GetSystemMetrics(SM_CYFOCUSBORDER));

                            if (!FillRect(hDC,
                                          &rcHighlight,
                                          infoPtr->hbSelection))
                            {
                                goto FailNoHighlight;
                            }

                            /* highlight the selected day */
                            crOldText = SetTextColor(hDC,
                                                     GetSysColor(MONTHCAL_SELFG));
                        }
                        else
                        {
FailNoHighlight:
                            /* don't change the text color, we're not highlighting it... */
                            crOldText = CLR_INVALID;
                        }

                        TextOut(hDC,
                                rcText.left,
                                rcText.top,
                                szDay,
                                szDayLen);

                        if (Day == infoPtr->Day && crOldText != CLR_INVALID)
                        {
                            if (infoPtr->HasFocus && !(infoPtr->UIState & UISF_HIDEFOCUS))
                            {
                                COLORREF crOldBk;

                                crOldBk = SetBkColor(hDC,
                                                     GetSysColor(MONTHCAL_SELBG));

                                DrawFocusRect(hDC,
                                              &rcHighlight);

                                SetBkColor(hDC,
                                           crOldBk);
                            }

                            SetTextColor(hDC,
                                         crOldText);
                        }
                    }
                }
            }
        }
    }

    if (crOldCtrlText != CLR_INVALID)
    {
        SetTextColor(hDC,
                     crOldCtrlText);
    }

    SetBkMode(hDC,
              iOldBkMode);
    SelectObject(hDC,
                 (HGDIOBJ)hOldFont);
}

static HFONT
MonthCalChangeFont(IN PMONTHCALWND infoPtr,
                   IN HFONT hFont,
                   IN BOOL Redraw)
{
    HFONT hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;

    if (Redraw)
    {
        InvalidateRect(infoPtr->hSelf,
                       NULL,
                       TRUE);
    }

    return hOldFont;
}

static WORD
MonthCalPtToDay(IN PMONTHCALWND infoPtr,
                IN SHORT x,
                IN SHORT y)
{
    WORD Ret = 0;

    if (infoPtr->CellSize.cx != 0 && infoPtr->CellSize.cy != 0 &&
        x >= 0 && y >= 0)
    {
        x /= infoPtr->CellSize.cx;
        y /= infoPtr->CellSize.cy;

        if (x < 7 && y != 0 && y < 7)
        {
            Ret = (WORD)infoPtr->Days[y - 1][x];
        }
    }

    return Ret;
}

static LRESULT CALLBACK
MonthCalWndProc(IN HWND hwnd,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{
    PMONTHCALWND infoPtr;
    LRESULT Ret = 0;
    
    infoPtr = (PMONTHCALWND)GetWindowLongPtr(hwnd,
                                             0);
    
    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }
    
    switch (uMsg)
    {
        case WM_PAINT:
        case WM_PRINTCLIENT:
        {
            if (infoPtr->CellSize.cx != 0 && infoPtr->CellSize.cy != 0)
            {
                PAINTSTRUCT ps;
                HDC hDC;

                if (wParam != 0)
                {
                    if (!GetUpdateRect(hwnd,
                                       &ps.rcPaint,
                                       TRUE))
                    {
                        break;
                    }
                    hDC = (HDC)wParam;
                }
                else
                {
                    hDC = BeginPaint(hwnd,
                                     &ps);
                    if (hDC == NULL)
                    {
                        break;
                    }
                }

                MonthCalPaint(infoPtr,
                              hDC,
                              &ps.rcPaint);

                if (wParam == 0)
                {
                    EndPaint(hwnd,
                             &ps);
                }
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        {
            WORD SelDay;

            SelDay = MonthCalPtToDay(infoPtr,
                                     GET_X_LPARAM(lParam),
                                     GET_Y_LPARAM(lParam));
            if (SelDay != 0 && SelDay != infoPtr->Day)
            {
                MonthCalSetDate(infoPtr,
                                SelDay,
                                infoPtr->Month,
                                infoPtr->Year);
            }

            /* fall through */
        }

        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (!infoPtr->HasFocus)
            {
                SetFocus(hwnd);
            }
            break;
        }

        case WM_KEYDOWN:
        {
            WORD NewDay = 0;

            switch (wParam)
            {
                case VK_UP:
                {
                    if (infoPtr->Day > 7)
                    {
                        NewDay = infoPtr->Day - 7;
                    }
                    break;
                }

                case VK_DOWN:
                {
                    if (infoPtr->Day + 7 <= MonthCalMonthLength(infoPtr->Month,
                                                                infoPtr->Year))
                    {
                        NewDay = infoPtr->Day + 7;
                    }
                    break;
                }

                case VK_LEFT:
                {
                    if (infoPtr->Day > 1)
                    {
                        NewDay = infoPtr->Day - 1;
                    }
                    break;
                }

                case VK_RIGHT:
                {
                    if (infoPtr->Day < MonthCalMonthLength(infoPtr->Month,
                                                           infoPtr->Year))
                    {
                        NewDay = infoPtr->Day + 1;
                    }
                    break;
                }
            }

            /* update the selection */
            if (NewDay != 0)
            {
                MonthCalSetDate(infoPtr,
                                NewDay,
                                infoPtr->Month,
                                infoPtr->Year);
            }

            goto HandleDefaultMessage;
        }

        case WM_GETDLGCODE:
        {
            INT virtKey;
            
            virtKey = (lParam != 0 ? (INT)((LPMSG)lParam)->wParam : 0);
            switch (virtKey)
            {
                case VK_TAB:
                {
                    /* change the UI status */
                    SendMessage(GetAncestor(hwnd,
                                            GA_PARENT),
                                WM_CHANGEUISTATE,
                                MAKEWPARAM(UIS_INITIALIZE,
                                           0),
                                0);
                    break;
                }
            }

            Ret |= DLGC_WANTARROWS;
            break;
        }

        case WM_SETFOCUS:
        {
            infoPtr->HasFocus = TRUE;
            MonthCalRepaintDay(infoPtr,
                               infoPtr->Day);
            break;
        }

        case WM_KILLFOCUS:
        {
            infoPtr->HasFocus = FALSE;
            MonthCalRepaintDay(infoPtr,
                               infoPtr->Day);
            break;
        }

        case WM_UPDATEUISTATE:
        {
            DWORD OldUIState = infoPtr->UIState;
            switch (LOWORD(wParam))
            {
                case UIS_SET:
                    infoPtr->UIState |= HIWORD(wParam);
                    break;

                case UIS_CLEAR:
                    infoPtr->UIState &= ~HIWORD(wParam);
                    break;
            }

            if (infoPtr->UIState != OldUIState)
            {
                MonthCalRepaintDay(infoPtr,
                                   infoPtr->Day);
            }
            break;
        }

        case MCCM_SETDATE:
        {
            WORD Day, Month, Year, DaysCount;

            Day = LOWORD(wParam);
            Month = HIWORD(wParam);
            Year = LOWORD(lParam);

            if (Day == (WORD)-1)
                Day = infoPtr->Day;
            if (Month == (WORD)-1)
                Month = infoPtr->Month;
            if (Year == (WORD)-1)
                Year = infoPtr->Year;

            DaysCount = MonthCalMonthLength(Month,
                                            Year);
            if (Day > DaysCount)
                Day = DaysCount;

            if (MonthCalValidDate(Day,
                                  Month,
                                  Year))
            {
                if (Day != infoPtr->Day ||
                    Month != infoPtr->Month ||
                    Year != infoPtr->Year)
                {
                    Ret = MonthCalSetDate(infoPtr,
                                          Day,
                                          Month,
                                          Year);
                }
            }
            break;
        }

        case MCCM_GETDATE:
        {
            LPSYSTEMTIME lpSystemTime = (LPSYSTEMTIME)wParam;

            lpSystemTime->wYear = infoPtr->Year;
            lpSystemTime->wMonth = infoPtr->Month;
            lpSystemTime->wDay = infoPtr->Day;

            Ret = TRUE;
            break;
        }

        case MCCM_RESET:
        {
            MonthCalSetLocalTime(infoPtr,
                                 NULL);
            Ret = TRUE;
            break;
        }

        case MCCM_CHANGED:
        {
            Ret = infoPtr->Changed;
            break;
        }

        case WM_TIMER:
        {
            switch (wParam)
            {
                case ID_DAYTIMER:
                {
                    /* kill the timer */
                    KillTimer(hwnd,
                              ID_DAYTIMER);
                    infoPtr->DayTimerSet = FALSE;

                    if (!infoPtr->Changed)
                    {
                        /* update the system time and setup the new day timer */
                        MonthCalSetLocalTime(infoPtr,
                                             NULL);

                        /* update the control */
                        MonthCalUpdate(infoPtr);
                    }
                    break;
                }
            }
            break;
        }

        case WM_SETFONT:
        {
            Ret = (LRESULT)MonthCalChangeFont(infoPtr,
                                              (HFONT)wParam,
                                              (BOOL)LOWORD(lParam));
            break;
        }

        case WM_SIZE:
        {
            infoPtr->ClientSize.cx = LOWORD(lParam);
            infoPtr->ClientSize.cy = HIWORD(lParam);
            infoPtr->CellSize.cx = infoPtr->ClientSize.cx / 7;
            infoPtr->CellSize.cy = infoPtr->ClientSize.cy / 7;

            /* repaint the control */
            InvalidateRect(hwnd,
                           NULL,
                           TRUE);
            break;
        }

        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }

        case WM_CREATE:
        {
            infoPtr = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(MONTHCALWND));
            if (infoPtr == NULL)
            {
                Ret = (LRESULT)-1;
                break;
            }

            SetWindowLongPtr(hwnd,
                             0,
                             (DWORD_PTR)infoPtr);

            ZeroMemory(infoPtr,
                       sizeof(MONTHCALWND));

            infoPtr->hSelf = hwnd;
            infoPtr->hNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;

            MonthCalSetLocalTime(infoPtr,
                                 NULL);

            MonthCalReload(infoPtr);
            break;
        }

        case WM_DESTROY:
        {
            HeapFree(GetProcessHeap(),
                     0,
                     infoPtr);
            SetWindowLongPtr(hwnd,
                             0,
                             (DWORD_PTR)NULL);
            break;
        }

        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProc(hwnd,
                                uMsg,
                                wParam,
                                lParam);
            break;
        }
    }

    return Ret;
}

BOOL
RegisterMonthCalControl(IN HINSTANCE hInstance)
{
    WNDCLASS wc = {0};

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MonthCalWndProc;
    wc.cbWndExtra = sizeof(PMONTHCALWND);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL,
                            (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(MONTHCAL_CTRLBG + 1);
    wc.lpszClassName = szMonthCalWndClass;

    return RegisterClass(&wc) != 0;
}

VOID
UnregisterMonthCalControl(IN HINSTANCE hInstance)
{
    UnregisterClass(szMonthCalWndClass,
                    hInstance);
}
