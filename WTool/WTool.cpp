// Applicazione concepita per semplificare la configurazione iniziale di un sistema operativo appena installato,
// senza la necessità di componenti esterni.
// Basata esclusivamente sulle API native di Windows, è progettata per garantire un utilizzo minimo di risorse e
// un impatto nullo sul sistema, mantenendolo leggero e pulito.

#include <windows.h>
#include <shellapi.h>   // Aggiunto: necessario per la tray icon (Shell_NotifyIcon)
#include "resource.h"

#define WM_TRAYICON (WM_USER + 1)   // Aggiunto: messaggio personalizzato per la tray icon
NOTIFYICONDATA nid = { 0 };         // Aggiunto: struttura per l’icona di notifica
#define TRAY_ICON_ID 1              // Per icona sulla TNR

#define BTN_NICS 1
#define BTN_TPM 2
#define BTN_LUSRMGR 3
#define BTN_SHARING 4
#define BTN_EVENTS 5
#define BTN_DEVS 6
#define BTN_SCHEDULER 7
#define BTN_EXPLORER 8
#define BTN_DATETIME 9
#define BTN_PERFS 10
#define BTN_THEME 13
#define BTN_DISKPART 14
#define BTN_DISKMAN 15
#define BTN_WAD 16

#define IDM_FILE_EXIT  101
#define IDM_DISKPART 105
#define IDM_TASKMGR 106
#define IDM_SYSINFO 107
#define IDM_ADMCMD 108
#define IDM_WINVER 102
#define IDM_HELP_ABOUT 103
#define IDM_ALWAYS_ON_TOP 109
#define IDM_TEMPDIR 110
#define IDM_USRDIR 111
#define IDM_WINDIR 112
#define IDM_SUU 113
#define IDM_SUC 114
#define IDM_ENABLE_AHCI 115
#define IDM_REFRESH_STATUES 116

#define IDM_OPEN_GUI 200

WCHAR szTempPath[MAX_PATH];

HWND hLabel = NULL;
HWND hLabelAHCI = NULL;
// HWND hLabelPS2 = NULL;
HWND hwnd = NULL;

void SetAlwaysOnTop(HWND hwnd, bool enable);
bool isAHCI();
bool SetAHCI();
bool IsRunningAsAdmin();
bool IsWindows10OrGreater();

BOOL isTopMost = FALSE;
BOOL isAppRunningAsAdmin = FALSE;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE retSE = NULL;

    switch (msg) {
    case WM_TRAYICON:
        // Aggiunto: gestisce i click sull’icona della tray
        if (lParam == WM_LBUTTONDBLCLK)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid); // rimuove l’icona, spostato in WM_DESTROY ...
            ShowWindow(hwnd, SW_SHOW);          // ripristina la finestra
            SetForegroundWindow(hwnd);
        }
        else if (lParam == WM_RBUTTONUP) // AGGIUNTO: click destro
        {
            HMENU hTrayMenu = CreatePopupMenu();
            AppendMenu(hTrayMenu, MF_STRING, IDM_OPEN_GUI, L"▶  Apri GUI");
            AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);

            HMENU hHelpMenu = CreatePopupMenu();
            AppendMenu(hHelpMenu, MF_STRING, IDM_WINVER, L"Versione OS");
            AppendMenu(hHelpMenu, MF_STRING, IDM_SYSINFO, L"Informazioni di Sistema");
            AppendMenu(hHelpMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"Circa");
            AppendMenu(hTrayMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"?");
            AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hTrayMenu, MF_STRING, IDM_FILE_EXIT, L"Esci da WTool");
            
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);

            DestroyMenu(hHelpMenu);
            DestroyMenu(hTrayMenu);

            //// Ottiene il menu Help esistente come sottomenu
            //HMENU hHelpMenu = GetSubMenu(GetMenu(hwnd), 3);

            //// Crea un menu temporaneo
            //HMENU hTrayMenu = CreatePopupMenu();
            //AppendMenu(hTrayMenu, MF_STRING, IDM_OPEN_GUI, L"Apri GUI");
            //AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
            //AppendMenu(hTrayMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Help");

            //// Mostra il menu nella posizione del cursore
            //POINT pt;
            //GetCursorPos(&pt);
            //SetForegroundWindow(hwnd);
            //TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            //PostMessage(hwnd, WM_NULL, 0, 0);

            //DestroyMenu(hTrayMenu);  // OK, questo sì: è quello temporaneo
        }
        break;

    case WM_SYSCOMMAND:
        // Aggiunto: intercetta la minimizzazione per nascondere la finestra e mettere l’icona nella tray
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            ShowWindow(hwnd, SW_HIDE); // nasconde la finestra
            Shell_NotifyIcon(NIM_ADD, &nid); // aggiunge l’icona di notifica
            return 0;
        }

        // Dopo aver gestito la parte che ti interessa, devi lasciare che Windows completi l’elaborazione predefinita richiamando DefWindowProc
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_NICS:
            retSE = ShellExecute(hwnd, L"runas", L"ncpa.cpl", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE == 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            else if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Impossibile aprire ncpa.cpl", L"Errore", MB_OK | MB_ICONERROR);

            break;
        case BTN_TPM:
            retSE = ShellExecute(hwnd, L"open", L"tpm.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Impossibile aprire tpm.msc", L"Errore", MB_OK | MB_ICONERROR);

            break;
        case BTN_LUSRMGR:
            retSE = ShellExecute(hwnd, L"open", L"lusrmgr.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"lusrmgr.msc not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_SHARING:
            retSE = ShellExecute(hwnd, L"open", L"fsmgmt.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"lusrmgr.msc not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_EVENTS:
            retSE = ShellExecute(hwnd, L"open", L"eventvwr.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"eventvwr.msc not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_DEVS:
            retSE = ShellExecute(hwnd, L"open", L"devmgmt.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"devmgmt.msc not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_SCHEDULER:
            retSE = ShellExecute(hwnd, L"open", L"taskschd.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"taskschd.msc not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_EXPLORER:
            retSE = ShellExecute(hwnd, L"open", L"control.exe", L"folders", NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"control folders not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_DATETIME:
            retSE = ShellExecute(hwnd, L"runas", L"timedate.cpl", NULL, NULL, SW_SHOWNORMAL);
            if((INT_PTR)retSE == 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            else if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Comando non trovato!", L"timedate.cpl not found ...", MB_OK | MB_ICONERROR);

            break;
        case BTN_THEME:
            retSE = ShellExecute(hwnd, L"open", L"ms-settings:colors", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"Thema selection not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_PERFS:
            retSE = ShellExecute(hwnd, L"open", L"SystemPropertiesPerformance", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"SystemPropertiesPerformance not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_DISKMAN:
            retSE = ShellExecute(hwnd, L"open", L"diskmgmt.msc", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"SystemPropertiesPerformance not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_WAD:
            retSE = ShellExecute(hwnd, L"open", L"optionalfeatures.exe", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"SystemPropertiesPerformance not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_DISKPART:
            retSE = ShellExecute(hwnd, L"runas", L"diskpart.exe", NULL, NULL, SW_SHOWNORMAL);
            if((INT_PTR)retSE == 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            else if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Comando non trovato!", L"diskpart.exe not found ...", MB_OK | MB_ICONERROR);

            break;
        // FINE

        case IDM_FILE_EXIT:
            // PostQuitMessage(0);
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_TEMPDIR:
            GetEnvironmentVariableW(L"TEMP", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_USRDIR:
            GetEnvironmentVariableW(L"USERPROFILE", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_WINDIR:
            GetEnvironmentVariableW(L"WINDIR", szTempPath, MAX_PATH);

            retSE = ShellExecute(hwnd, L"open", szTempPath, NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"%TEMP% not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_TASKMGR:
            retSE = ShellExecute(hwnd, L"runas", L"taskmgr.exe", NULL, NULL, SW_SHOWNORMAL);
            if((INT_PTR)retSE <= 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            else if ((INT_PTR)retSE <= 32) 
                MessageBox(hwnd, L"Comando non trovato!", L"taskmgr.exe not found ...", MB_OK | MB_ICONERROR);

            break;
        case IDM_SYSINFO:
            retSE = ShellExecute(hwnd, L"open", L"msinfo32.exe", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"msinfo32.exe not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_WINVER:
            retSE = ShellExecute(hwnd, L"open", L"winver.exe", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"winver.exe not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_ADMCMD:
            retSE = ShellExecute(hwnd, L"runas", L"cmd.exe", NULL, NULL, SW_SHOWNORMAL);

            if ((INT_PTR)retSE == 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Comando non trovato!", L"cmd.exe not found ...", MB_OK | MB_ICONERROR);

            break;
        case IDM_ALWAYS_ON_TOP:

            SetAlwaysOnTop(hwnd, !isTopMost);
            isTopMost = !isTopMost;

            MENUITEMINFO mii;
            ZeroMemory(&mii, sizeof(MENUITEMINFO)); // azzera la struttura
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STATE;

            GetMenuItemInfo(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, FALSE, &mii);

            if (mii.fState & MFS_CHECKED)
                CheckMenuItem(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_UNCHECKED);
            else
                CheckMenuItem(GetMenu(hwnd), IDM_ALWAYS_ON_TOP, MF_BYCOMMAND | MF_CHECKED);

            break;
        case IDM_HELP_ABOUT:
            MessageBox(hwnd, L"WTool v1.0 (x64) per Windows 10 e Windows 11\n\nRealizzato da Alessandro Favretto.\n\n\nApplicazione standalone realizzata in VC++ e WinAPI.",
                L"Informazioni su WTool\n",
                MB_OK | MB_ICONINFORMATION);
            break;

            // ESECUZIONE AUTOMATICA ...
            // shell:startup
            // shell:common startup
        case IDM_SUU:
            retSE = ShellExecute(hwnd, L"open", L"explorer.exe", L"shell:startup", NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"user startup not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_SUC:
            retSE = ShellExecute(hwnd, L"open", L"explorer.exe", L"shell:common startup", NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"common startup not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case IDM_ENABLE_AHCI:
            if (!isAppRunningAsAdmin) {
                MessageBoxW(
                    NULL,
                    L"Impossibile eseguire l'operazione.\nRiavviare l'applicazione come amministratore.",
                    L"Autorizzazione necessaria ...",
                    MB_ICONHAND | MB_OK );
            } else {
                if (MessageBoxW(
                    hwnd, L"AHCI offre prestazioni superiori rispetto al driver legacy.\nComunque, il BIOS deve supportare tale funzionalità e deve essere abilitata al successivo avvio.\n\nIn caso contrario il sistema potrebbe diventare inutilizzabile.\n\nSicuro di voler procedere?\n", L"Conferma attivazione AHCI ...",
                    MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
                ) == IDYES) {
                    // L'utente ha premuto Sì (applica le modifiche) ...
                    if (SetAHCI())
                        MessageBoxW(NULL,
                            L"Si è verificato un errore durante l'abilitazione AHCI.",
                            L"Errore ...",
                            MB_ICONERROR | MB_OK);
                }
            }

            break;
        case IDM_REFRESH_STATUES:
            // Controllo se è IDE (Legacy) o AHCI ...
            if (isAHCI())
                ShowWindow(hLabelAHCI, SW_HIDE);
            else
                ShowWindow(hLabelAHCI, SW_SHOW);
            break;
        case IDM_OPEN_GUI:
            ShowWindow(hwnd, SW_SHOW);          // ripristina la finestra
            SetForegroundWindow(hwnd);
            break;
        }

        break;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndCtrl = (HWND)lParam;

        if (hwndCtrl == hLabel)  { // Gestione Label Header
            SetTextColor(hdcStatic, RGB(192, 125, 0));      // COLORE FOREGROUND
            SetBkMode(hdcStatic, RGB(0, 0, 0));             // COLORE BACKGROUND
            return (INT_PTR)GetStockObject(BLACK_BRUSH);
        } else if (hwndCtrl == hLabelAHCI) { // Gestione Label AHCI
            SetTextColor(hdcStatic, RGB(255, 0, 0));      // COLORE FOREGROUND
            SetBkMode(hdcStatic, RGB(0, 0, 0));             // COLORE BACKGROUND
            return (INT_PTR)GetStockObject(BLACK_BRUSH);
        }
        //else if (hwndCtrl == hLabelPS2) { // Gestione Label PS2
        //    SetTextColor(hdcStatic, RGB(255, 0, 0));      // COLORE FOREGROUND
        //    SetBkMode(hdcStatic, RGB(0, 0, 0));             // COLORE BACKGROUND
        //    return (INT_PTR)GetStockObject(BLACK_BRUSH);
        //}
        
        break;
    }

    case WM_SETCURSOR:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        HWND hCwfp = ChildWindowFromPoint(hwnd, pt);

        if (hCwfp == GetDlgItem(hwnd, BTN_NICS) ||
            hCwfp == GetDlgItem(hwnd, BTN_TPM) ||
            hCwfp == GetDlgItem(hwnd, BTN_LUSRMGR) ||
            hCwfp == GetDlgItem(hwnd, BTN_SHARING) ||
            hCwfp == GetDlgItem(hwnd, BTN_EVENTS) ||
            hCwfp == GetDlgItem(hwnd, BTN_DEVS) ||
            hCwfp == GetDlgItem(hwnd, BTN_SCHEDULER) ||
            hCwfp == GetDlgItem(hwnd, BTN_EXPLORER) ||
            hCwfp == GetDlgItem(hwnd, BTN_DATETIME) ||
            hCwfp == GetDlgItem(hwnd, BTN_THEME) ||
            hCwfp == GetDlgItem(hwnd, BTN_DISKPART) ||
            hCwfp == GetDlgItem(hwnd, BTN_DISKMAN) ||
            hCwfp == GetDlgItem(hwnd, BTN_WAD) ||
            hCwfp == GetDlgItem(hwnd, BTN_PERFS))
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE; // messaggio gestito
        }
        else {
            SetCursor(LoadCursor(NULL, IDC_ARROW)); // freccia se fuori bottone
            return TRUE;
        }

        break;
    }

    case WM_CLOSE:
    {
        int result = MessageBox(hwnd,
            L"Vuoi davvero uscire dall'applicazione?",
            L"Conferma uscita",
            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

        if (result == IDYES) {
            DestroyWindow(hwnd);
        }
        // se NO, ignora la chiusura
        return 0;
    }
    case WM_DESTROY:{
        NOTIFYICONDATA nid = { 0 };
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = TRAY_ICON_ID;
        Shell_NotifyIcon(NIM_DELETE, &nid); // Aggiunto: rimuove l’icona di notifica se presente
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void SetAlwaysOnTop(HWND hwnd, bool enable)
{
    SetWindowPos(
        hwnd,
        enable ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE
    );
}

bool IsWindows10OrGreater()
{
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (!hMod) return false;

    RtlGetVersionPtr fn = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
    if (!fn) return false;

    RTL_OSVERSIONINFOW rovi = { 0 };
    rovi.dwOSVersionInfoSize = sizeof(rovi);
    fn(&rovi);

    // Windows 10 = major 10, minor 0
    return (rovi.dwMajorVersion > 10) ||
        (rovi.dwMajorVersion == 10 && rovi.dwMinorVersion >= 0);
}

bool SetAHCI() {
    HKEY hKey = NULL;

    LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\storahci", 0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS) {
        // std::wcerr << L"Errore aprendo la chiave: " << subKey << L" Codice: " << res << std::endl;
        return false;
    }
    else {
        DWORD data = 0;
        res = RegSetValueExW(hKey, L"Start", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(data));
        RegCloseKey(hKey);

        if (res != ERROR_SUCCESS) {
            //std::wcerr << L"Errore scrivendo valore: " << valueName << L" Codice: " << res << std::endl;
            return false;
        } else
            return true;
    }
}

bool isAHCI()
{
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\storahci",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD start;
        DWORD size = sizeof(start);
        if (RegQueryValueEx(hKey, L"Start", NULL, NULL, (LPBYTE)&start, &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return start == 0 || start == 3; // driver pronto per AHCI
        }
        RegCloseKey(hKey);
    }
    return false;
}

bool IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    // Crea un SID per il gruppo Administrators
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (!IsWindows10OrGreater()) {
        MessageBoxW(NULL,
            L"Questa applicazione richiede Windows 10 o versioni successive.",
            L"Versione di Windows non supportata",
            MB_ICONERROR | MB_OK);
        return 0;
    }

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW); // obbligatorio per WNDCLASSEX
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MiniLauncher";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);

    // --- Aggiunta icona ---
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));    // icona grande (taskbar, Alt+Tab)
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));  // icona piccola (tray, titlebar)

    RegisterClassExW(&wc);

    // CENTRATURA DELLA FINESTRA SULLO SCHERMO ...
    // 
    // Dimensioni dello schermo
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calcolo posizione centrata
    int x = (screenWidth - 640) / 2;
    int y = (screenHeight - 480) / 2;

    isAppRunningAsAdmin = IsRunningAsAdmin();
    if(isAppRunningAsAdmin)
        hwnd = CreateWindowEx(
            0,
            L"MiniLauncher",
            L"WTool v.1.0 (x64) - {Administrator}",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            x, y,
            640, 480,
            NULL, NULL, hInstance, NULL
        );
    else
        hwnd = CreateWindowEx(
            0,
            L"MiniLauncher",
            L"WTool v.1.0 (x64)",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            x, y,
            640, 480,
            NULL, NULL, hInstance, NULL
        );

    // Aggiunto: inizializzazione struttura tray icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID; // 1; Icona definita sopra
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    //nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // icona

    lstrcpy(nid.szTip, L"WTool - Doppio-click per ripristinare");

    Shell_NotifyIcon(NIM_ADD, &nid); // aggiunge l'icona nella tray

    // Label Header
    hLabel = CreateWindowEx(
        0,
        L"STATIC",
        L"SELEZIONA UN'APPLICAZIONE DA LANCIARE:",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 10, 600, 20,       // posizione e dimensione
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Label AHCI
    hLabelAHCI = CreateWindowEx(
        0,
        L"STATIC",
        L"Windows sta usando IDE (Legacy)",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 402, 295, 15,       // posizione e dimensione
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Label PS2
    //hLabelPS2 = CreateWindowEx(
    //    0,
    //    L"STATIC",
    //    L"Nessuna porta PS2 attiva",
    //    WS_CHILD | WS_VISIBLE | SS_CENTER,
    //    315, 402, 295, 15,       // posizione e dimensione
    //    hwnd,
    //    NULL,
    //    hInstance,
    //    NULL
    //);

    // Pulsanti
    CreateWindowEx(0, L"BUTTON", L"NICs",
        WS_CHILD | WS_VISIBLE,
        50, 50, 250, 30,
        hwnd, (HMENU)BTN_NICS, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"TPM",
        WS_CHILD | WS_VISIBLE,
        50, 90, 250, 30,
        hwnd, (HMENU)BTN_TPM, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Utenti&&Gruppi",
        WS_CHILD | WS_VISIBLE,
        50, 130, 250, 30,
        hwnd, (HMENU)BTN_LUSRMGR, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Condivisioni",
        WS_CHILD | WS_VISIBLE,
        50, 170, 250, 30,
        hwnd, (HMENU)BTN_SHARING, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Eventi",
        WS_CHILD | WS_VISIBLE,
        50, 210, 250, 30,
        hwnd, (HMENU)BTN_EVENTS, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Dispositivi",
        WS_CHILD | WS_VISIBLE,
        50, 250, 250, 30,
        hwnd, (HMENU)BTN_DEVS, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Scheduler",
        WS_CHILD | WS_VISIBLE,
        50, 290, 250, 30,
        hwnd, (HMENU)BTN_SCHEDULER, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Opzioni Files",
        WS_CHILD | WS_VISIBLE,
        50, 330, 250, 30,
        hwnd, (HMENU)BTN_EXPLORER, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Data&&Ora",
        WS_CHILD | WS_VISIBLE,
        50, 370, 250, 30,
        hwnd, (HMENU)BTN_DATETIME, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Prestazioni e Swap",
        WS_CHILD | WS_VISIBLE,
        320, 50, 250, 30,
        hwnd, (HMENU)BTN_PERFS, hInstance, NULL);

    //HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    HFONT hFont = CreateFontW(
        15,                 // altezza del font in pixel
        0,                  // larghezza carattere (0 = proporzionale)
        0, 0,               // angolo escapement e orientamento
        FW_NORMAL,          // spessore (FW_BOLD per grassetto)
        TRUE, FALSE, FALSE,// italic, underline, strikeout
        ANSI_CHARSET,       // charset
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, // stile font
        L"Courier New"         // nome font nativo Windows (alternative: "Segoe UI", "Courier New", "Arial")
    );

    // BEGIN: SWAP TEXTBOX MULTILINE ...
    // 
    HWND hSwapInfo = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
        320, 90, 250, 70,
        hwnd,
        (HMENU)1001,
        hInstance,
        NULL);

    SendMessageW(hSwapInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowTextW(hSwapInfo, L"Con disco SSD o M.2:\r\nPer preservare la durata del disco, impostare un file di swap statico (MIN = MAX) di dimensione pari al doppio della RAM installata.\r\n\r\nEsempio:\r\nSe la RAM è di 8 GB, il file di swap statico ideale sarà:\r\n8 GB × 2 = 16 GB → 16 × 1024  = 16.384 MB.");

    // END

    CreateWindowEx(0, L"BUTTON", L"Selezione Tema",
        WS_CHILD | WS_VISIBLE,
        320, 170, 250, 30,
        hwnd, (HMENU)BTN_THEME, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Disk Part",
        WS_CHILD | WS_VISIBLE,
        320, 210, 250, 30,
        hwnd, (HMENU)BTN_DISKPART, hInstance, NULL);
    
    // BEGIN: DISKPART TEXTBOX MULTILINE ...
    // 
    HWND hDiskPartInfo = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_READONLY,
        320, 250, 250, 70,
        hwnd,
        (HMENU)1001,
        hInstance,
        NULL);

    SendMessageW(hDiskPartInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowTextW(hDiskPartInfo, L"Dove non arriva Gestione Dischi:\r\nUsare DISKPART\r\n\r\nEsempio pulizia disco:\r\nlist disk\r\nselect disk 1\r\nclean\r\n\r\nEsempio eliminazione di una partizione:\r\nselect disk 1\r\nlist partition\r\nselect partition 3\r\ndelete partition\r\n\r\nPer uscire:\r\nexit");
    // END

    CreateWindowEx(0, L"BUTTON", L"Gestione Dischi",
        WS_CHILD | WS_VISIBLE,
        320, 330, 250, 30,
        hwnd, (HMENU)BTN_DISKMAN, hInstance, NULL);

    // BEGIN: STARTUP 20251021AF...
    //
    CreateWindowEx(0, L"BUTTON", L"Funzionalità OS",
        WS_CHILD | WS_VISIBLE,
        320, 370, 250, 30,
        hwnd, (HMENU)BTN_WAD, hInstance, NULL);

    //CreateWindowEx(0, L"BUTTON", L"StartUp Common",
    //    WS_CHILD | WS_VISIBLE,
    //    445, 370, 125, 30,
    //    hwnd, (HMENU)IDM_SUC, hInstance, NULL);
    //
    // END


    // BEGIN: GESTIONE MENU A TENDINA ...
    // 
    HMENU hMenuBar = CreateMenu();      // menu principale (barra)

    HMENU hFileMenu = CreateMenu();     // menu File
    HMENU hWindowMenu = CreateMenu();   // menu Finestra
    HMENU hActions = CreateMenu();      // menu Azioni
    HMENU hHelpMenu = CreateMenu();     // menu Help

    HMENU hGoTo = CreateMenu();         // Navigazione Cartelle Notevoli
    HMENU hStartUp = CreateMenu();      // Navigazione Avvio Automatico

    // Navigazione Cartelle Notevoli: Aggiunge voci al sotto-sottomenu ...
    AppendMenu(hGoTo, MF_STRING, IDM_TEMPDIR, L"Cartella TEMP");
    AppendMenu(hGoTo, MF_STRING, IDM_USRDIR, L"Cartella USER");
    AppendMenu(hGoTo, MF_STRING, IDM_WINDIR, L"Cartella OS");
    
    // Navigazione Avvio Automatico: Aggiunge voci al sotto-sottomenu ...
    AppendMenu(hStartUp, MF_STRING, IDM_SUU, L"Utente");
    AppendMenu(hStartUp, MF_STRING, IDM_SUC, L"Comune");

    // Aggiunge i sottomenu al menu File
    AppendMenu(hFileMenu, MF_POPUP, (UINT_PTR)hGoTo, L"Vai a ...");
    AppendMenu(hFileMenu, MF_POPUP, (UINT_PTR)hStartUp, L"Avvio automatico ...");

    // Aggiunge voci ai sottomenu ...
    AppendMenu(hFileMenu, MF_STRING, IDM_TASKMGR, L"Task Manager");
    AppendMenu(hFileMenu, MF_STRING, IDM_ADMCMD, L"Console (Admin)");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Esci");

    AppendMenu(hWindowMenu, MF_STRING, IDM_ALWAYS_ON_TOP, L"Sempre in primo piano");

    AppendMenu(hActions, MF_STRING, IDM_ENABLE_AHCI, L"Abilita AHCI");
    AppendMenu(hActions, MF_STRING, IDM_REFRESH_STATUES, L"Aggiona");

    AppendMenu(hHelpMenu, MF_STRING, IDM_WINVER, L"Versione OS");
    AppendMenu(hHelpMenu, MF_STRING, IDM_SYSINFO, L"Informazioni di Sistema");
    AppendMenu(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"Circa");
 
    // Aggiunge i sottomenu alla barra principale ...
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hFileMenu, L"&File");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hActions, L"&Azioni");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hWindowMenu, L"Fi&nestra");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR) hHelpMenu, L"&?");

    // Imposta la barra menu sulla finestra ...
    SetMenu(hwnd, hMenuBar);

    // END

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Controllo se è IDE (Legacy) o AHCI ...
    if (isAHCI()) {
        ShowWindow(hLabelAHCI, SW_HIDE);

        EnableMenuItem(GetMenu(hwnd), IDM_ENABLE_AHCI, MF_BYCOMMAND | MF_GRAYED);
        DrawMenuBar(hwnd);
    } else {
        ShowWindow(hLabelAHCI, SW_SHOW);

        EnableMenuItem(GetMenu(hwnd), IDM_ENABLE_AHCI, MF_BYCOMMAND | MF_ENABLED);
        DrawMenuBar(hwnd);
    }

    // Riduco la finestra sulla TNR postando rispettivo messaggio ...
    PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}