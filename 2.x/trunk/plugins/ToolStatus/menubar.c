/*
 * Copyright (c) 2012 Martin Mitas
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <phdk.h>
#include <windowsx.h>

#include <emenu.h>
#include "menubar.h"

/* TODO:
 *  -- Support to make a chevron in a parent ReBar easy to do.
 *  -- Fix the artifact bug (for now it is worked around, see below).
 */
/* The MenuBar implementation is partially based on the MSDN article "How to
 * Create an Internet Explorer-Style Menu Bar":
 * http://msdn.microsoft.com/en-us/library/windows/desktop/bb775450%28v=vs.85%29.aspx
 */
/* We have a bug somewhere which causes that when the menu is dropped down
 * by mouse and the mouse is then moved ober other toolbar button, the firstly
 * pressed button is displayed as hot.
 *
 * For now we "solved" by a very hacky workaround, which is guarded by this
 * macro. Comment this out to see and debug the bug...
 */
#define MENUBAR_ARTIFACT_WORKAROUND        1

/* Uncomment this to have more verbose traces from this module. */
/*#define MENUBAR_DEBUG     1*/
#ifdef MENUBAR_DEBUG
    #define MENUBAR_TRACE          MC_TRACE
#else
    #define MENUBAR_TRACE(...)     do { } while(0)
#endif


static const TCHAR menubar_wc[] = L"PH_WC_MENUBAR";    /* window class name */
static int extra_offset;
static WNDPROC orig_toolbar_proc = NULL;

typedef struct menubar_tag 
{
    HWND win;
    HWND notify_win;
    HWND old_focus;  /* We return focus to it on <ESC> */
    HMENU menu;
    short hot_item;
    short pressed_item;
    WORD continue_hot_track     : 1;
    WORD select_from_keyboard   : 1;
    WORD is_dropdown_active     : 1;
} menubar_t;


#define MENUBAR_ITEM_LABEL_MAXSIZE     32
#define MENUBAR_SEPARATOR_WIDTH        10

#define MENUBAR_SENDMSG(win, msg, wp, lp) CallWindowProc(orig_toolbar_proc, (win), (msg), (WPARAM)(wp), (LPARAM)(lp))


/* Forward declarations */
static void menubar_ht_enable(menubar_t* mb);
static void menubar_ht_disable(menubar_t* mb);

/* Send simple (i.e. using only NMHDR) notification */
static __inline LRESULT mc_send_notify(HWND parent, HWND win, UINT code)
{
    NMHDR hdr;

    hdr.hwndFrom = win;
    hdr.idFrom = GetWindowLongPtr(win, GWLP_ID);
    hdr.code = code;

    return SendMessage(parent, WM_NOTIFY, hdr.idFrom, (LPARAM)&hdr);
}

static int menubar_set_menu(menubar_t* mb, HMENU menu)
{
    BYTE* buffer = NULL;
    TBBUTTON* buttons;
    TCHAR* labels;
    int i, n;

    //MENUBAR_TRACE("menubar_set_menu(%p, %p)", mb, menu);

    /* Uninstall the old menu */
#if 0  /* not possible as long as MC_MBM_REFRESH works as it does now */
    if(menu == mb->menu)
        return;
#endif

    /* If dropped down, cancel it */
    if (mb->pressed_item >= 0) 
    {
        menubar_ht_disable(mb);
        MENUBAR_SENDMSG(mb->win, WM_CANCELMODE, 0, 0);
    }

    if (mb->menu != NULL)
    {
        n = (INT)MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0);
        
        for (i = 0; i < n; i++)
        {
            MENUBAR_SENDMSG(mb->win, TB_DELETEBUTTON, 0, 0);
        }

        mb->menu = NULL;
    }

    /* Install the new menu */
    n = (menu != NULL ? GetMenuItemCount(menu) : 0);
    if (n < 0) 
        return -1;

    if (n == 0) 
    {
        mb->menu = menu;
        return 0;
    }

    buffer = (BYTE*)_malloca(n * sizeof(TBBUTTON) + n * sizeof(TCHAR) * MENUBAR_ITEM_LABEL_MAXSIZE);
    if (buffer == NULL) 
    {
        //MC_TRACE("menubar_set_menu: _malloca() failed.");
        mc_send_notify(mb->notify_win, mb->win, NM_OUTOFMEMORY);
        return -1;
    }

    buttons = (TBBUTTON*)buffer;
    labels = (TCHAR*)(buffer + n * sizeof(TBBUTTON));

    memset(buttons, 0, n * sizeof(TBBUTTON));

    for (i = 0; i < n; i++) 
    {
        UINT state;
        state = GetMenuState(menu, i, MF_BYPOSITION);

        buttons[i].iBitmap = I_IMAGENONE;
        buttons[i].fsState = 0;

        if (!(state & (MF_DISABLED | MF_GRAYED)))
            buttons[i].fsState |= TBSTATE_ENABLED;
        if ((state & (MF_MENUBREAK | MF_MENUBARBREAK)) && i > 0)
            buttons[i-1].fsState |= TBSTATE_WRAP;

        if (state & MF_POPUP)
        {
            TCHAR* label = labels + i * MENUBAR_ITEM_LABEL_MAXSIZE;
            GetMenuString(menu, i, label, MENUBAR_ITEM_LABEL_MAXSIZE, MF_BYPOSITION);

            buttons[i].fsStyle = BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT;
            buttons[i].dwData = i;
            buttons[i].iString = (INT_PTR)PhaFormatString(L" %s", label)->Buffer;
            buttons[i].idCommand = i;
        }
        else
        {
            buttons[i].dwData = 0xffff;
            buttons[i].idCommand = 0xffff;

            if (state & MF_SEPARATOR) 
            {
                buttons[i].fsStyle = BTNS_SEP;
                buttons[i].iBitmap = MENUBAR_SEPARATOR_WIDTH;
            }
        }
    }

    MENUBAR_SENDMSG(mb->win, TB_ADDBUTTONS, n, buttons);
    mb->menu = menu;
    _freea(buffer);
    return 0;
}

static void menubar_dropdown_helper(menubar_t* mb)
{
    int index;
    DWORD btn_state;
    TPMPARAMS pmparams = { sizeof(TPMPARAMS) };
    PPH_EMENU menu = NULL;

    mb->is_dropdown_active = TRUE;
    menubar_ht_enable(mb);

    while (mb->pressed_item >= 0) 
    {
        index = mb->pressed_item;

        if (mb->select_from_keyboard) 
        {
            keybd_event(VK_DOWN, 0, 0, 0);
            keybd_event(VK_DOWN, 0, KEYEVENTF_KEYUP, 0);
        }

        mb->select_from_keyboard = FALSE;
        mb->continue_hot_track = FALSE;

        btn_state = (ULONG)MENUBAR_SENDMSG(mb->win, TB_GETSTATE, index, 0);
        MENUBAR_SENDMSG(mb->win, TB_SETSTATE, index, MAKELONG(btn_state | TBSTATE_PRESSED, 0));

        MENUBAR_SENDMSG(mb->win, TB_GETITEMRECT, index, &pmparams.rcExclude);
        MapWindowPoints(mb->win, HWND_DESKTOP, (POINT*)&pmparams.rcExclude, 2);

        //menu = PhCreateEMenu();
        //PhLoadResourceEMenuItem(
        //    menu, 
        //    GetModuleHandle(NULL), 
        //    MAKEINTRESOURCE(103), 
        //    index
        //    );
        //PhMwpInitializeSubMenu(menu, index);     
        //PhEMenuToHMenu2(GetSubMenu(mb->menu, index), menu, 0, NULL);

        TrackPopupMenuEx(
            GetSubMenu(mb->menu, index),   
            TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
            pmparams.rcExclude.left, pmparams.rcExclude.bottom,
            mb->win, 
            &pmparams
            );

        //SendMessage(
        //    PhMainWndHandle, 
        //    WM_INITMENUPOPUP, 
        //    (WPARAM)(GetSubMenu(mb->menu, index)), 
        //    index
        //    );

        MENUBAR_SENDMSG(mb->win, TB_SETSTATE, index, MAKELONG(btn_state, 0));

        if (!mb->continue_hot_track)
            mb->pressed_item = -1;
    }

    menubar_ht_disable(mb);
    mb->is_dropdown_active = FALSE;
}

static __inline void menubar_dropdown(menubar_t* mb, int item, BOOL from_keyboard)
{
    //MENUBAR_TRACE("menubar_dropdown(%p, %d)", mb, item);

    MENUBAR_SENDMSG(mb->win, TB_SETHOTITEM, -1, 0);
    mb->pressed_item = item;
    mb->select_from_keyboard = from_keyboard;

#ifndef MENUBAR_ARTIFACT_WORKAROUND
    menubar_dropdown_helper(mb);
#endif
}

static void
menubar_notify(menubar_t* mb, NMHDR* hdr)
{
    switch(hdr->code) {
        case TBN_DROPDOWN:
        {
            NMTOOLBAR* info = (NMTOOLBAR*) hdr;
            MENUBAR_TRACE("menubar_notify(%p, TBN_DROPDOWN, %d)", mb, info->iItem);
            menubar_dropdown(mb, info->iItem, FALSE);
            break;
        }

        case TBN_HOTITEMCHANGE:
        {
            NMTBHOTITEM* info = (NMTBHOTITEM*) hdr;
            MENUBAR_TRACE("menubar_notify(%p, TBN_HOTITEMCHANGE, %d -> %d)", mb,
                          (info->dwFlags & HICF_ENTERING) ? -1 : info->idOld,
                          (info->dwFlags & HICF_LEAVING) ? -1 : info->idNew);
            mb->hot_item = (info->dwFlags & HICF_LEAVING) ? -1 : info->idNew;
            break;
        }
    }
}

static BOOL
menubar_key_down(menubar_t* mb, int vk, DWORD key_data)
{
    switch(vk) {
        case VK_ESCAPE:
            MENUBAR_TRACE("menubar_key_down(VK_ESCAPE)");
            if(mb->hot_item >= 0) {
                MENUBAR_SENDMSG(mb->win, TB_SETHOTITEM, -1, 0);
                if(mb->win == GetFocus())
                    SetFocus(mb->old_focus);
                return TRUE;
            }
            break;

        case VK_LEFT:
            MENUBAR_TRACE("menubar_key_down(VK_LEFT)");
            if(mb->hot_item >= 0) {
                int item = mb->hot_item - 1;
                if(item < 0)
                    item = (INT)MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0) - 1;
                MENUBAR_SENDMSG(mb->win, TB_SETHOTITEM, item, 0);
                return TRUE;
            }
            break;

        case VK_RIGHT:
            MENUBAR_TRACE("menubar_key_down(VK_RIGHT)");
            if(mb->hot_item >= 0) {
                int item = mb->hot_item + 1;
                if(item >= MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0))
                    item = 0;
                MENUBAR_SENDMSG(mb->win, TB_SETHOTITEM, item, 0);
                return TRUE;
            }
            break;

        case VK_DOWN:
        case VK_UP:
        case VK_RETURN:
            MENUBAR_TRACE("menubar_key_down(VK_DOWN/VK_UP/VK_RETURN)");
            if(mb->hot_item >= 0) {
                menubar_dropdown(mb, mb->hot_item, TRUE);
                return TRUE;
            }
            break;
    }

    /* If we have not consum the key, report it to the caller. */
    return FALSE;
}

static menubar_t*
menubar_nccreate(HWND win, CREATESTRUCT *cs)
{
    menubar_t* mb;
    TCHAR parent_class[16];

    MENUBAR_TRACE("menubar_nccreate(%p, %p)", win, cs);

    mb = (menubar_t*)malloc(sizeof(menubar_t));
    if (mb == NULL) 
    {
        //MC_TRACE("menubar_nccreate: malloc() failed.");
        return NULL;
    }

    memset(mb, 0, sizeof(menubar_t));
    mb->win = win;

    /* Lets be a little friendly to the app. developers: If the parent is
     * ReBar control, lets send WM_NOTIFY/WM_COMMAND to the ReBar's parent
     * as ReBar really is not interested in it, and embedding the menubar
     * in the ReBar is actually main advantage of this control in comparision
     * with the standard window menu. */
    GetClassName(cs->hwndParent, parent_class, _countof(parent_class));

    if (wcscmp(parent_class, L"ReBarWindow32") == 0)
    {
        mb->notify_win = GetAncestor(cs->hwndParent, GA_PARENT);
    }
    else
    {
        mb->notify_win = cs->hwndParent;
    }

    mb->hot_item = -1;
    mb->pressed_item = -1;

    return mb;
}

static int menubar_create(menubar_t* mb, CREATESTRUCT *cs)
{
    MENUBAR_TRACE("menubar_create(%p, %p)", mb, cs);

    if (MENUBAR_SENDMSG(mb->win, WM_CREATE, 0, cs) != 0)
    {
        //MC_TRACE_ERR("menubar_create: CallWindowProc() failed");
        return -1;
    }

    MENUBAR_SENDMSG(mb->win, TB_SETPARENT, mb->win, 0);
    MENUBAR_SENDMSG(mb->win, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    /* Add some styles we consider default */
    SetWindowLongPtr(mb->win, GWL_STYLE, cs->style | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | CCS_NODIVIDER);

    //if (cs->lpCreateParams != NULL)

    if (menubar_set_menu(mb, GetMenu(PhMainWndHandle)) != 0)
    {
        //MC_TRACE("menubar_create: menubar_set_menu() failed.");
        return -1;
    }

    return 0;
}

static __inline void menubar_destroy(menubar_t* mb)
{
    MENUBAR_TRACE("menubar_destroy(%p)", mb);
}

static __inline void menubar_ncdestroy(menubar_t* mb)
{
    MENUBAR_TRACE("menubar_ncdestroy(%p)", mb);
    free(mb);
}

static LRESULT CALLBACK menubar_proc(HWND win, UINT msg, WPARAM wp, LPARAM lp)
{
    menubar_t* mb = (menubar_t*)GetWindowLongPtr(win, extra_offset);

    switch (msg)
    {
        //case MC_MBM_REFRESH:
        //    // TODO: Do it in a more effecient way then by reinstalling
        //    //       all buttons.
        //    lp = (LPARAM)mb->menu;
        //    /* no break */
        //case MC_MBM_SETMENU:
        //    return (menubar_set_menu(mb, (HMENU)lp) == 0 ? TRUE : FALSE);

        case TB_SETPARENT:
        case CCM_SETNOTIFYWINDOW:
        {
            HWND old = mb->notify_win;
            mb->notify_win = (wp ? (HWND) wp : GetAncestor(win, GA_PARENT));
            return (LRESULT) old;
        }

        case WM_COMMAND:
            MENUBAR_TRACE("menubar_proc(WM_COMMAND): code=%d; wid=%d; control=%p",
                          (int)HIWORD(wp), (int)LOWORD(wp), (HWND)lp);
            if(lp == 0  &&  HIWORD(wp) == 0)  /* msg came from the menu */
                return SendMessage(mb->notify_win, msg, wp, lp);
            break;

        case WM_NOTIFY:
        {
            NMHDR* hdr = (NMHDR*) lp;
            if(hdr->hwndFrom == win) {
                menubar_notify(mb, hdr);
                return SendMessage(mb->notify_win, msg, wp, lp);
            }
            break;
        }

        case WM_ENTERMENULOOP:
        case WM_EXITMENULOOP:
        case WM_CONTEXTMENU:
        case WM_INITMENU:
        case WM_INITMENUPOPUP:
        case WM_UNINITMENUPOPUP:
        case WM_MENUSELECT:
        case WM_MENUCHAR:
        case WM_MENURBUTTONUP:
        case WM_MENUCOMMAND:
        case WM_MENUDRAG:
        case WM_MENUGETOBJECT:
        case WM_MEASUREITEM:
        case WM_DRAWITEM:
            return SendMessage(mb->notify_win, msg, wp, lp);

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (menubar_key_down(mb, (INT)wp, (DWORD)lp))
                return 0;
            break;

        case WM_GETDLGCODE:
            return MENUBAR_SENDMSG(win, msg, wp, lp) | DLGC_WANTALLKEYS | DLGC_WANTARROWS;

        case WM_NCCREATE:
            if (MENUBAR_SENDMSG(win, msg, wp, lp) == FALSE)
            {
                //MC_TRACE_ERR("menubar_proc: MENUBAR_SENDMSG(WM_NCCREATE) failed");
                return FALSE;
            }

            mb = menubar_nccreate(win, (CREATESTRUCT*)lp);

            if (mb == NULL)
                return FALSE;

            SetWindowLongPtr(win, extra_offset, (LONG_PTR)mb);
            return TRUE;

        case WM_SETFOCUS:
            mb->old_focus = (HWND) wp;
            break;

        case WM_KILLFOCUS:
            if (mb->old_focus != NULL) 
            {
                SetFocus(mb->old_focus);
                mb->old_focus = NULL;
            }
            MENUBAR_SENDMSG(mb->win, TB_SETHOTITEM, -1, 0);
            MENUBAR_SENDMSG(mb->win, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEACCEL), 0);
            break;

        case WM_CREATE:
            return menubar_create(mb, (CREATESTRUCT*) lp);

        case WM_DESTROY:
            menubar_destroy(mb);
            break;

        case WM_NCDESTROY:
            if (mb)
            {
                menubar_ncdestroy(mb);
                mb = NULL;
            }
            break;

        /* Disable those standard toolbar messages, which modify contents of
         * the toolbar, as it is our internal responsibility to set it
         * according to the menu. */
        case TB_ADDBITMAP:
        case TB_ADDSTRING:
            //MC_TRACE("menubar_proc: Suppressing message TB_xxxx (%d)", msg);
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return -1;
        case TB_ADDBUTTONS:
        case TB_BUTTONSTRUCTSIZE:
        case TB_CHANGEBITMAP:
        case TB_CUSTOMIZE:
        case TB_DELETEBUTTON:
        case TB_ENABLEBUTTON:
        case TB_HIDEBUTTON:
        case TB_INDETERMINATE:
        case TB_INSERTBUTTON:
        case TB_LOADIMAGES:
        case TB_MARKBUTTON:
        case TB_MOVEBUTTON:
        case TB_PRESSBUTTON:
        case TB_REPLACEBITMAP:
        case TB_SAVERESTORE:
        case TB_SETANCHORHIGHLIGHT:
        case TB_SETBITMAPSIZE:
        case TB_SETBOUNDINGSIZE:
        case TB_SETCMDID:
        case TB_SETDISABLEDIMAGELIST:
        case TB_SETHOTIMAGELIST:
        case TB_SETIMAGELIST:
        case TB_SETINSERTMARK:
        case TB_SETPRESSEDIMAGELIST:
        case TB_SETSTATE:
            //MC_TRACE("menubar_proc: Suppressing message TB_xxxx (%d)", msg);
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return 0;  /* FALSE == NULL == 0 */
    }

#ifdef MENUBAR_ARTIFACT_WORKAROUND
    if(mb && mb->pressed_item >= 0 && !mb->is_dropdown_active)
        menubar_dropdown_helper(mb);
#endif

    return MENUBAR_SENDMSG(win, msg, wp, lp);
}


/********************
 *** Hot Tracking ***
 ********************/

static CRITICAL_SECTION menubar_ht_lock;
static HHOOK menubar_ht_hook = NULL;
static menubar_t* menubar_ht_mb = NULL;
static HMENU menubar_ht_sel_menu = NULL;
static int menubar_ht_sel_item = -1;
static UINT menubar_ht_sel_flags = 0;
static POINT menubar_ht_last_pos = {0};

static void
menubar_ht_change_dropdown(menubar_t* mb, int item, BOOL from_keyboard)
{
    //MENUBAR_TRACE("menubar_ht_change_dropdown(%p,%d)", mb, item);

    mb->pressed_item = item;
    mb->select_from_keyboard = from_keyboard;
    mb->continue_hot_track = TRUE;
    MENUBAR_SENDMSG(mb->win, WM_CANCELMODE, 0, 0);
}

static LRESULT CALLBACK
menubar_ht_proc(int code, WPARAM wp, LPARAM lp)
{
    MSG* msg = (MSG*)lp;
    menubar_t* mb = menubar_ht_mb;

    switch (msg->message)
    {
        case WM_MENUSELECT:
            menubar_ht_sel_menu = (HMENU)msg->lParam;
            menubar_ht_sel_item = LOWORD(msg->wParam);
            menubar_ht_sel_flags = HIWORD(msg->wParam);
            MENUBAR_TRACE("menubar_ht_proc: WM_MENUSELECT %p %d", menubar_ht_sel_menu, menubar_ht_sel_item);
            break;

        case WM_MOUSEMOVE:
        {
            POINT pt = msg->pt;
            int item;

            MapWindowPoints(NULL, mb->win, &pt, 1);
            item = (INT)MENUBAR_SENDMSG(mb->win, TB_HITTEST, 0, (LPARAM)&pt);
            if(menubar_ht_last_pos.x != pt.x  ||  menubar_ht_last_pos.y != pt.y) {
                menubar_ht_last_pos = pt;
                if(item != mb->pressed_item  &&
                   0 <= item  &&  item < (int)MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0)) {
                    //MENUBAR_TRACE("menubar_ht_proc: Change dropdown by mouse move "
                    //              "[%d -> %d]", mb->pressed_item, item);
                    menubar_ht_change_dropdown(mb, item, FALSE);
                }
            }
            break;
        }

        case WM_KEYDOWN:
            switch(msg->wParam) {
                case VK_LEFT:
                    if(menubar_ht_sel_menu == NULL  ||
                       menubar_ht_sel_menu == GetSubMenu(mb->menu, mb->pressed_item))
                    {
                        int item = mb->pressed_item - 1;
                        if(item < 0)
                            item = (int)MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0) - 1;
                        //MENUBAR_TRACE("menubar_ht_proc: Change dropdown by VK_LEFT");
                        if(item != mb->pressed_item)
                            menubar_ht_change_dropdown(mb, item, TRUE);
                    }
                    break;

                case VK_RIGHT:
                    if (menubar_ht_sel_menu == NULL || !(menubar_ht_sel_flags & MF_POPUP) || (menubar_ht_sel_flags & (MF_GRAYED | MF_DISABLED)))
                    {
                        int item = mb->pressed_item + 1;
                        if(item >= (int)MENUBAR_SENDMSG(mb->win, TB_BUTTONCOUNT, 0, 0))
                            item = 0;
                        //MENUBAR_TRACE("menubar_ht_proc: Change dropdown by VK_RIGHT");
                        if(item != mb->pressed_item)
                            menubar_ht_change_dropdown(mb, item, TRUE);
                    }
                    break;
            }
            break;
    }

    return CallNextHookEx(menubar_ht_hook, code, wp, lp);
}

static void __inline menubar_ht_perform_disable(void)
{
    if (menubar_ht_hook) 
    {
        UnhookWindowsHookEx(menubar_ht_hook);
        menubar_ht_hook = NULL;
        menubar_ht_mb = NULL;
        menubar_ht_sel_menu = NULL;
        menubar_ht_sel_item = -1;
        menubar_ht_sel_flags = 0;
    }
}

static void menubar_ht_enable(menubar_t* mb)
{
    //MENUBAR_TRACE("menubar_ht_enable(%p)", mb);
    EnterCriticalSection(&menubar_ht_lock);

    if(menubar_ht_hook != NULL) {
        //MC_TRACE("menubar_ht_enable: Another menubar hot tracks???");
        menubar_ht_perform_disable();
    }

    menubar_ht_hook = SetWindowsHookEx(WH_MSGFILTER, menubar_ht_proc, GetModuleHandle(NULL), GetCurrentThreadId());
    if (menubar_ht_hook == NULL)
    {
        //MC_TRACE_ERR("menubar_ht_enable: SetWindowsHookEx() failed");
        goto err_hook;
    }

    menubar_ht_mb = mb;

err_hook:
    LeaveCriticalSection(&menubar_ht_lock);
}

static void
menubar_ht_disable(menubar_t* mb)
{
    MENUBAR_TRACE("menubar_ht_disable(%p)", mb);

    EnterCriticalSection(&menubar_ht_lock);

    if (menubar_ht_mb != mb)
    {
        //MC_TRACE("menubar_ht_disable: Another menubar hot tracks???");
    }
    else
    {
        menubar_ht_perform_disable();
    }

    LeaveCriticalSection(&menubar_ht_lock);
}


/**********************
 *** Initialization ***
 **********************/
int menubar_init_module(void)
{
    WNDCLASS wc = { 0 };

    //mc_init_common_controls(ICC_BAR_CLASSES | ICC_COOL_CLASSES);

    if (!GetClassInfo(NULL, L"ToolbarWindow32", &wc))
    {
        //MC_TRACE_ERR("menubar_init_module: GetClassInfo() failed");
        return -1;
    }

    /* Remember needed values of standard toolbar window class */
    orig_toolbar_proc = wc.lpfnWndProc;
    extra_offset = wc.cbWndExtra;

    /* Create our subclass. */
    wc.lpfnWndProc = menubar_proc;
    wc.cbWndExtra += sizeof(menubar_t*);
    wc.style |= CS_GLOBALCLASS;
    wc.hInstance = NULL;
    wc.lpszClassName = menubar_wc;

    if (!RegisterClass(&wc)) 
    {
        //MC_TRACE_ERR("menubar_init_module: RegisterClass() failed");
        return -1;
    }

    InitializeCriticalSection(&menubar_ht_lock);

    return 0;
}

void
menubar_fini_module(void)
{
    DeleteCriticalSection(&menubar_ht_lock);
    UnregisterClass(menubar_wc, NULL);
}


/**************************
 *** Exported functions ***
 **************************/

BOOL mcIsMenubarMessage(HWND hwndMenubar, LPMSG lpMsg)
{
    switch(lpMsg->message) {
        case WM_SYSKEYUP:
        case WM_KEYUP:
            /* Handle <F10> or <ALT> */
            if((lpMsg->wParam == VK_F10 || lpMsg->wParam == VK_MENU)  &&
               !(lpMsg->lParam & 0x20000000)  &&
               !(GetKeyState(VK_SHIFT) & 0x8000)) {
                /* Toggle focus */
                if(hwndMenubar != GetFocus()) {
                    SetFocus(hwndMenubar);
                } else {
                    menubar_t* mb = (menubar_t*) GetWindowLongPtr(hwndMenubar, extra_offset);
                    SetFocus(mb->old_focus);
                }
                return TRUE;
            }
            break;

        case WM_SYSCHAR:
        case WM_CHAR:
            /* Handle hotkeys (<ALT> + something) */
            if(lpMsg->lParam & 0x20000000) {
                UINT item;
                if(MENUBAR_SENDMSG(hwndMenubar, TB_MAPACCELERATOR, lpMsg->wParam, &item) != 0) {
                    menubar_t* mb = (menubar_t*) GetWindowLongPtr(hwndMenubar, extra_offset);
                    menubar_dropdown(mb, item, TRUE);
                    return TRUE;
                }
            }
            break;
    }

    return FALSE;
}
