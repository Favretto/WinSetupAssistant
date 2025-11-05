//
// APPLICAZIONE CONCEPITA PER SEMPLIFICARE LA CONFIGURAZIONE INIZIALE DI UN SISTEMA OPERATIVO APPENA INSTALLATO,
// SENZA LA NECESSITÀ DI COMPONENTI ESTERNI.
// BASATA ESCLUSIVAMENTE SULLE API NATIVE DI WINDOWS, È PROGETTATA PER GARANTIRE UN UTILIZZO MINIMO DI RISORSE E
// UN IMPATTO NULLO SUL SISTEMA, MANTENENDLO LEGGERO E PULITO.
// ------------------------
// WTool - Versione 1.0
// Alessandro Favretto
// 24/10/2025
// ------------------------
//

#include <windows.h>
#include <shellapi.h>   // Aggiunto: necessario per la tray icon (Shell_NotifyIcon)
#include "resource.h"

#include <shlobj.h>     // GODMODE: SHGetFolderPathW, CSIDL_DESKTOP
#include <strsafe.h>    // GODMODE: StringCchPrintfW

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
#define BTN_UNINSTALLER 17
#define BTN_MRT 18

#define IDM_FILE_EXIT  101
#define IDM_GODMODE 120
#define IDM_DISKPART 105
#define IDM_TASKMGR 106
#define IDM_SYSINFO 107
#define IDM_ADMCMD 108
#define IDM_WINVER 102
#define IDM_HELP_ABOUT 103
#define IDM_ALWAYS_ON_TOP 109
#define IDM_CLIPBOARD 118
#define IDM_SCREENSHOT 119
#define IDM_TEMPDIR 110
#define IDM_USRDIR 111
#define IDM_WINDIR 112
#define IDM_SUU 113
#define IDM_SUC 114
#define IDM_ENABLE_AHCI 115
#define IDM_GPU_RESET 117
#define IDM_REFRESH_STATUES 116

#define IDM_HIDDEN_FILES 121
#define IDM_SYS_FILES 122
#define IDM_ALWAYS_EXT 123
// #define IDM_PATH 124         // NON ESISTE PIU' IN WINDOWS 11 ...
// #define IDM_PATH_ADDRBAR 125 // NON ESISTE PIU' IN WINDOWS 11 ...
#define IDM_MYCOMPUTER 127
#define IDM_TRASH 128
#define IDM_NETWORK 129
#define IDM_FOLDER_OPTS_APPLY 126
#define IDM_DESKTOP_ICO_APPLY 130

#define IDM_OPEN_GUI 200

WCHAR szTempPath[MAX_PATH];

HANDLE hMutex = NULL;

HWND hLabel = NULL;
HWND hLabelAHCI = NULL;
// HWND hLabelPS2 = NULL;
HWND hBtnMRT = NULL, hBtnDiskPart = NULL, hBtnDateTime = NULL, hBtnNICs = NULL, hBtnEnvVars = NULL;
HWND hwnd = NULL;

void SetAlwaysOnTop(HWND hwnd, bool enable);
bool isAHCI();
bool SetAHCI();
bool IsRunningAsAdmin();
bool IsWindows10OrGreater();
BOOL is64Bit();

// LAVORANO AL CONTRARIO 0=SHOWN, 1=HIDDEN
BOOL bMyComputer = FALSE;
BOOL bTrash = FALSE;
BOOL bNetwork = FALSE;

BOOL isTopMost = FALSE;
BOOL isAppRunningAsAdmin = FALSE;

BOOL bShowHiddenFiles = TRUE;
BOOL bShowSysFiles = TRUE;
BOOL bShowAlwaysExt = TRUE;
//BOOL bShowPath = TRUE;
//BOOL bShowPathAddrBar = TRUE;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE retSE = NULL;

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Imposta colore di sfondo nero e linee opache
        SetBkColor(hdc, RGB(96, 96, 96));           // sfondo grigio scuro ...
        SetBkMode(hdc, OPAQUE);                     // disegna lo sfondo con il colore impostato ...

        // Penna Navy per il bordo
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 128)); // Navy ...

        // Pennello a linee oblique
        HBRUSH hBrush = CreateHatchBrush(HS_BDIAGONAL, RGB(210, 240, 255));

        // Seleziona penna e pennello nel contesto
        HGDIOBJ oldPen = SelectObject(hdc, hPen);
        HGDIOBJ oldBrush = SelectObject(hdc, hBrush);

        // Disegna i rettangoli con sfondo nero e linee navy
        Rectangle(hdc, 315, 44, 575, 166);
        Rectangle(hdc, 315, 204, 576, 326);

        // Ripristina e libera
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        EndPaint(hwnd, &ps);
        break;
    }

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
            retSE = ShellExecute(hwnd, L"runas", L"rundll32.exe", L"sysdm.cpl,EditEnvironmentVariables", NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Impossibile aprire le variabili di ambiente!", L"Errore apertura ...", MB_OK | MB_ICONERROR);
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
        case BTN_UNINSTALLER:
            // retSE = ShellExecute(hwnd, L"open", L"ms-settings:appsfeatures", NULL, NULL, SW_SHOWNORMAL);
            retSE = ShellExecute(hwnd, L"open", L"shell:AppsFolder", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)retSE <= 32) {
                MessageBox(hwnd, L"Comando non trovato!", L"shell:AppsFolder not found ...", MB_OK | MB_ICONERROR);
            }
            break;
        case BTN_MRT:{
            HINSTANCE retSE = NULL;
            if(is64Bit()){
                retSE = ShellExecute(hwnd, L"runas", L"MRT.EXE", NULL, NULL, SW_SHOWNORMAL);
            }else{
                retSE = ShellExecute(hwnd, L"open", L"ms-settings:windowsdefender", NULL, NULL, SW_SHOWNORMAL);
            }
            
            if ((INT_PTR)retSE == 5)
                MessageBox(hwnd, L"Permessi insufficienti per accedere alla funzione!", L"Errore", MB_OK | MB_ICONERROR);
            else if ((INT_PTR)retSE <= 32)
                MessageBox(hwnd, L"Impossibile aprire lo strumento antimalware!", L"Errore", MB_OK | MB_ICONERROR);
            break;
        }
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
        case IDM_GODMODE: {
            WCHAR desktopPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath)))
            {
                // Costruisci percorso completo
                WCHAR godModePath[MAX_PATH];
                StringCchPrintfW(
                    godModePath,
                    MAX_PATH,
                    L"%s\\GodMode.{ED7BA470-8E54-465E-825C-99712043E01C}",
                    desktopPath
                );

                // Crea la cartella
                if (CreateDirectoryW(godModePath, NULL))
                {
                    MessageBoxW(NULL, L"Cartella GodMode creata con successo sul Desktop.", L"Successo", MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    DWORD err = GetLastError();
                    if (err == ERROR_ALREADY_EXISTS)
                        MessageBoxW(NULL, L"La cartella esiste già.", L"Informazione", MB_OK | MB_ICONINFORMATION);
                    else
                        MessageBoxW(NULL, L"Errore nella creazione della cartella.", L"Errore", MB_OK | MB_ICONERROR);
                }
            }

            break;
        }
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
            else if ((INT_PTR)retSE <= 32)
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
        case IDM_HELP_ABOUT: {
            if (is64Bit()) {
                MessageBox(hwnd, L"WTool v1.0 (x64) per Windows 10 e Windows 11\n\nRealizzato da Alessandro Favretto.\n\n\nApplicazione standalone realizzata in VC++ e WinAPI.",
                    L"Informazioni su WTool\n",
                    MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBox(hwnd, L"WTool v1.0 (x86) per Windows 10 e Windows 11\n\nRealizzato da Alessandro Favretto.\n\n\nApplicazione standalone realizzata in VC++ e WinAPI.",
                    L"Informazioni su WTool\n",
                    MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
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
        case IDM_HIDDEN_FILES:
            bShowHiddenFiles = !bShowHiddenFiles;
            CheckMenuItem(GetMenu(hwnd), IDM_HIDDEN_FILES, MF_BYCOMMAND | (bShowHiddenFiles ? MF_CHECKED : MF_UNCHECKED));
            break;
        case IDM_SYS_FILES:
            bShowSysFiles = !bShowSysFiles;
            CheckMenuItem(GetMenu(hwnd), IDM_SYS_FILES, MF_BYCOMMAND | (bShowSysFiles ? MF_CHECKED : MF_UNCHECKED));
            break;
        case IDM_ALWAYS_EXT:
            bShowAlwaysExt = !bShowAlwaysExt;
            CheckMenuItem(GetMenu(hwnd), IDM_ALWAYS_EXT, MF_BYCOMMAND | (bShowAlwaysExt ? MF_CHECKED : MF_UNCHECKED));
            break;
        //case IDM_PATH:
        //    bShowPath = !bShowPath;
        //    CheckMenuItem(GetMenu(hwnd), IDM_PATH, MF_BYCOMMAND | (bShowPath ? MF_CHECKED : MF_UNCHECKED));
        //    break;
        //case IDM_PATH_ADDRBAR:
        //    bShowPathAddrBar = !bShowPathAddrBar;
        //    CheckMenuItem(GetMenu(hwnd), IDM_PATH_ADDRBAR, MF_BYCOMMAND | (bShowPathAddrBar ? MF_CHECKED : MF_UNCHECKED));
        //    break;
        case IDM_MYCOMPUTER:
            bMyComputer = !bMyComputer;
            CheckMenuItem(GetMenu(hwnd), IDM_MYCOMPUTER, MF_BYCOMMAND | (bMyComputer ? MF_UNCHECKED : MF_CHECKED));
            break;
        case IDM_TRASH:
            bTrash = !bTrash;
            CheckMenuItem(GetMenu(hwnd), IDM_TRASH, MF_BYCOMMAND | (bTrash ? MF_UNCHECKED : MF_CHECKED));
            break;
        case IDM_NETWORK:
            bNetwork = !bNetwork;
            CheckMenuItem(GetMenu(hwnd), IDM_NETWORK, MF_BYCOMMAND | (bNetwork ? MF_UNCHECKED : MF_CHECKED));
            break;
        case IDM_DESKTOP_ICO_APPLY: {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel",
                0,
                KEY_SET_VALUE,
                &hKey) == ERROR_SUCCESS)
            {
                if (RegSetValueExW(hKey, L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, REG_DWORD, (BYTE*)&bMyComputer, sizeof(DWORD)) != ERROR_SUCCESS ||
                    RegSetValueExW(hKey, L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}", 0, REG_DWORD, (BYTE*)&bNetwork, sizeof(DWORD)) != ERROR_SUCCESS ||
                    RegSetValueExW(hKey, L"{645FF040-5081-101B-9F08-00AA002F954E}", 0, REG_DWORD, (BYTE*)&bTrash, sizeof(DWORD)) != ERROR_SUCCESS)
                {
                    MessageBoxW(NULL, L"Errore scrivendo uno o più valori di registro.", L"Errore", MB_ICONERROR);
                    RegCloseKey(hKey);
                } else {
                    RegCloseKey(hKey);

                    // Termina la shell
                    HWND hShellWnd = FindWindow(L"Shell_TrayWnd", NULL);
                    if (hShellWnd) {
                        DWORD dwPid;
                        GetWindowThreadProcessId(hShellWnd, &dwPid);
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
                        if (hProcess) {
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);

                            // Aspetta un po' prima di riavviare
                            Sleep(2000);
                        }
                    }

                    // Riavvia solo la shell (taskbar e desktop)
                    ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWDEFAULT);

                    MessageBoxW(NULL, L"Icone desktop aggiornate correttamente!", L"Info", MB_ICONINFORMATION);
                }
            } else {
                MessageBoxW(NULL, L"Impossibile aprire la chiave di registro.", L"Errore", MB_ICONERROR);
            }

            break;
        }
        case IDM_FOLDER_OPTS_APPLY: {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                0,
                KEY_SET_VALUE,
                &hKey) != ERROR_SUCCESS) {
                MessageBoxW(NULL, L"Impossibile aprire la chiave di registro.", L"Errore", MB_ICONERROR);
            } else {
                BOOL bHideKnownFilesExt = !bShowAlwaysExt;

                // Imposta i valori nel registro
                if (RegSetValueExW(hKey, L"Hidden", 0, REG_DWORD, (BYTE*)&bShowHiddenFiles, sizeof(DWORD)) != ERROR_SUCCESS ||
                    RegSetValueExW(hKey, L"ShowSuperHidden", 0, REG_DWORD, (BYTE*)&bShowSysFiles, sizeof(DWORD)) != ERROR_SUCCESS ||
                    RegSetValueExW(hKey, L"HideFileExt", 0, REG_DWORD, (BYTE*)&bHideKnownFilesExt, sizeof(DWORD)) != ERROR_SUCCESS) { // ||
                    //RegSetValueExW(hKey, L"FullPath", 0, REG_DWORD, (BYTE*)&bShowPath, sizeof(DWORD)) != ERROR_SUCCESS ||
                    //RegSetValueExW(hKey, L"FullPathAddress", 0, REG_DWORD, (BYTE*)&bShowPathAddrBar, sizeof(DWORD)) != ERROR_SUCCESS) {
                    MessageBoxW(NULL, L"Errore scrivendo uno o più valori di registro.", L"Errore", MB_ICONERROR);
                    RegCloseKey(hKey);
                } else {
                    RegCloseKey(hKey);
                    
                    // Termina la shell
                    HWND hShellWnd = FindWindow(L"Shell_TrayWnd", NULL);
                    if (hShellWnd) {
                        DWORD dwPid;
                        GetWindowThreadProcessId(hShellWnd, &dwPid);
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
                        if (hProcess) {
                            TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                        }
                    }

                    // Riavvia solo la shell (taskbar e desktop)
                    ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWDEFAULT);

                    MessageBoxW(NULL, L"Opzioni Cartella di aggiornate correttamente!", L"Info", MB_ICONINFORMATION);
                }
            }

            break;
        }
        case IDM_GPU_RESET: {
            
            INPUT inputs[8] = {};

            // CTRL
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_CONTROL;

            // SHIFT
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_SHIFT;

            // WIN
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = VK_LWIN;

            // B
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = 'B';

            // Rilascio in ordine inverso
            inputs[4].type = INPUT_KEYBOARD; inputs[4].ki.wVk = 'B'; inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[5].type = INPUT_KEYBOARD; inputs[5].ki.wVk = VK_LWIN; inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[6].type = INPUT_KEYBOARD; inputs[6].ki.wVk = VK_SHIFT; inputs[6].ki.dwFlags = KEYEVENTF_KEYUP;
            inputs[7].type = INPUT_KEYBOARD; inputs[7].ki.wVk = VK_CONTROL; inputs[7].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
            
            break;
        }
        case IDM_CLIPBOARD: { // WIN + V
            INPUT inputs[4] = {};

            // WIN
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_LWIN;

            // V
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = 'V';

            // Rilascio in ordine inverso
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = 'V';
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_LWIN;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

            // Invio della sequenza
            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

            break;
        }
        case IDM_SCREENSHOT: { // WIN + SHIFT + S
            INPUT inputs[6] = {};

            // WIN
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_LWIN;

            // SHIFT
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = VK_SHIFT;

            // S
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = 'S';

            // --- Rilascio in ordine inverso ---
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = 'S';
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

            inputs[4].type = INPUT_KEYBOARD;
            inputs[4].ki.wVk = VK_SHIFT;
            inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

            inputs[5].type = INPUT_KEYBOARD;
            inputs[5].ki.wVk = VK_LWIN;
            inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

            // Invio della sequenza
            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        }
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
            hCwfp == GetDlgItem(hwnd, BTN_PERFS) ||
            hCwfp == GetDlgItem(hwnd, BTN_UNINSTALLER) ||
            hCwfp == GetDlgItem(hwnd, BTN_MRT)
            )
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

BOOL is64Bit() {
#if defined(_WIN64)
    return TRUE;  // Processo a 64 bit
#else
    return FALSE; // Processo a 32 bit
#endif
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
    // Impedisce l'esecuzione multipla ...
    hMutex = CreateMutexW(NULL, TRUE, L"WTool_Mutex_SingleInstance");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"Processo già in esecuzione.", L"WTool", MB_OK | MB_ICONEXCLAMATION);
        if (NULL != hMutex) CloseHandle(hMutex);
        return 0; // termina subito
    }

    // Richiede minimo Windows 10 ...
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
    if (is64Bit()) {
        if (isAppRunningAsAdmin)
            hwnd = CreateWindowEx(
                0,
                L"MiniLauncher",
                L"WTool v1.0 (x64) - {Administrator}",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
        else
            hwnd = CreateWindowEx(
                0,
                L"MiniLauncher",
                L"WTool v1.0 (x64)",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
    } else {
        if (isAppRunningAsAdmin)
            hwnd = CreateWindowEx(
                0,
                L"MiniLauncher",
                L"WTool v1.0 (x86) - {Administrator}",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
        else
            hwnd = CreateWindowEx(
                0,
                L"MiniLauncher",
                L"WTool v1.0 (x86)",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                x, y,
                640, 480,
                NULL, NULL, hInstance, NULL
            );
    }

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

    // Impostazione FONTS ...
    HFONT hFontTextBoxes = CreateFontW(
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

    HFONT hFontButtons = CreateFontW(
        16,                 // altezza del font in pixel
        0,                  // larghezza carattere (0 = proporzionale)
        0, 0,               // angolo escapement e orientamento
        FW_NORMAL,          // spessore (FW_BOLD per grassetto)
        FALSE, FALSE, FALSE,// italic, underline, strikeout
        ANSI_CHARSET,       // charset
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, // stile font
        L"Courier New"         // nome font nativo Windows (alternative: "Segoe UI", "Courier New", "Arial")
    );

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
    hBtnNICs = CreateWindowEx(0, L"BUTTON", L"🛡️NICs",
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

    hBtnEnvVars = CreateWindowEx(0, L"BUTTON", L"🛡️Variabili d'ambiente",
        WS_CHILD | WS_VISIBLE,
        50, 330, 250, 30,
        hwnd, (HMENU)BTN_EXPLORER, hInstance, NULL);

    hBtnDateTime=CreateWindowEx(0, L"BUTTON", L"🛡️Data&&Ora",
        WS_CHILD | WS_VISIBLE,
        50, 370, 250, 30,
        hwnd, (HMENU)BTN_DATETIME, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Prestazioni e Swap",
        WS_CHILD | WS_VISIBLE,
        320, 50, 250, 30,
        hwnd, (HMENU)BTN_PERFS, hInstance, NULL);

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

    SendMessageW(hSwapInfo, WM_SETFONT, (WPARAM)hFontTextBoxes, TRUE);

    SetWindowTextW(hSwapInfo, L"Con disco SSD o M.2:\r\nPer preservare la durata del disco, impostare un file di swap statico (MIN = MAX) di dimensione pari al doppio della RAM installata.\r\n\r\nEsempio:\r\nSe la RAM è di 8 GB, il file di swap statico ideale sarà:\r\n8 GB × 2 = 16 GB → 16 × 1024  = 16.384 MB.");

    // END

    CreateWindowEx(0, L"BUTTON", L"Selezione Tema",
        WS_CHILD | WS_VISIBLE,
        320, 170, 250, 30,
        hwnd, (HMENU)BTN_THEME, hInstance, NULL);

    hBtnDiskPart = CreateWindowEx(0, L"BUTTON", L"🛡️ Disk Part",
        WS_CHILD | WS_VISIBLE,
        320, 210, 125, 30,
        hwnd, (HMENU)BTN_DISKPART, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Gestione Dischi",
        WS_CHILD | WS_VISIBLE,
        445, 210, 125, 30,
        hwnd, (HMENU)BTN_DISKMAN, hInstance, NULL);

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

    SendMessageW(hDiskPartInfo, WM_SETFONT, (WPARAM)hFontTextBoxes, TRUE);

    SetWindowTextW(hDiskPartInfo, L"Dove non arriva Gestione Dischi:\r\nUsare DISKPART\r\n\r\nEsempio pulizia disco:\r\nlist disk\r\nselect disk 1\r\nclean\r\n\r\nEsempio eliminazione di una partizione:\r\nselect disk 1\r\nlist partition\r\nselect partition 3\r\ndelete partition\r\n\r\nPer uscire:\r\nexit");

    CreateWindowEx(0, L"BUTTON", L"Funzionalità OS",
        WS_CHILD | WS_VISIBLE,
        320, 330, 125, 30,
        hwnd, (HMENU)BTN_WAD, hInstance, NULL);

    CreateWindowEx(0, L"BUTTON", L"Apps Manager",
        WS_CHILD | WS_VISIBLE,
        445, 330, 125, 30,
        hwnd, (HMENU)BTN_UNINSTALLER, hInstance, NULL);
    
    hBtnMRT = CreateWindowEx(0, L"BUTTON", L"🛡️ Rimozione Malware",
        WS_CHILD | WS_VISIBLE,
        320, 370, 250, 30,
        hwnd, (HMENU)BTN_MRT, hInstance, NULL);

    // Imposto il FONT dei caratteri della casella di testo anche ai bottoni ...
    SendMessage(hBtnNICs, WM_SETFONT, (WPARAM)hFontButtons, TRUE);
    SendMessage(hBtnEnvVars, WM_SETFONT, (WPARAM)hFontButtons, TRUE);
    SendMessage(hBtnDateTime, WM_SETFONT, (WPARAM)hFontButtons, TRUE);
    SendMessage(hBtnDiskPart, WM_SETFONT, (WPARAM)hFontButtons, TRUE);
    SendMessage(hBtnMRT, WM_SETFONT, (WPARAM)hFontButtons, TRUE);

    //370
    // 
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

    HMENU hFolderOpts = CreateMenu();   // Opzioni files e cartelle
    HMENU hDesktopIco = CreateMenu();   // Opzioni icone Desktop

    // Opzioni files e cartelle ...
    AppendMenu(hFolderOpts, MF_STRING, IDM_HIDDEN_FILES, L"Files nascosti");
    AppendMenu(hFolderOpts, MF_STRING, IDM_SYS_FILES, L"Files di Sistema");
    AppendMenu(hFolderOpts, MF_STRING, IDM_ALWAYS_EXT, L"Estensione files noti");
    //AppendMenu(hFolderOpts, MF_STRING, IDM_PATH, L"Percorso completo sulla barra del titolo");
    //AppendMenu(hFolderOpts, MF_STRING, IDM_PATH_ADDRBAR, L"Percorso completo sulla barra del degli indirizzi");
    AppendMenu(hFolderOpts, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFolderOpts, MF_STRING, IDM_FOLDER_OPTS_APPLY, L"Applica!");

    AppendMenu(hDesktopIco, MF_STRING, IDM_MYCOMPUTER, L"My Computer sul Desktop");
    AppendMenu(hDesktopIco, MF_STRING, IDM_TRASH, L"Cestino sul Desktop");
    AppendMenu(hDesktopIco, MF_STRING, IDM_NETWORK, L"Rete sul Desktop");
    AppendMenu(hDesktopIco, MF_SEPARATOR, 0, NULL);
    AppendMenu(hDesktopIco, MF_STRING, IDM_DESKTOP_ICO_APPLY, L"Applica!");
    
    // e abilito tutte le voci ...
    CheckMenuItem(hFolderOpts, IDM_HIDDEN_FILES, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hFolderOpts, IDM_SYS_FILES, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hFolderOpts, IDM_ALWAYS_EXT, MF_BYCOMMAND | MF_CHECKED);
    //CheckMenuItem(hFolderOpts, IDM_PATH, MF_BYCOMMAND | MF_CHECKED);
    //CheckMenuItem(hFolderOpts, IDM_PATH_ADDRBAR, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hDesktopIco, IDM_MYCOMPUTER, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hDesktopIco, IDM_TRASH, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hDesktopIco, IDM_NETWORK, MF_BYCOMMAND | MF_CHECKED);

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
    AppendMenu(hFileMenu, MF_STRING, IDM_ADMCMD, L"🛡️Console");
    AppendMenu(hFileMenu, MF_STRING, IDM_GODMODE, L"Crea God Mode sul desktop");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Esci");
    
    AppendMenu(hWindowMenu, MF_STRING, IDM_ALWAYS_ON_TOP, L"Sempre in primo piano");
    AppendMenu(hWindowMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hWindowMenu, MF_STRING, IDM_CLIPBOARD, L"Cronologia Appunti (WIN-V)");
    AppendMenu(hWindowMenu, MF_STRING, IDM_SCREENSHOT, L"Cattura Schermo (WIN-SHIFT-S)");

    AppendMenu(hActions, MF_STRING, IDM_ENABLE_AHCI, L"Abilita AHCI");
    AppendMenu(hActions, MF_STRING, IDM_GPU_RESET, L"Reset GPU (CTRL-WIN-SHIFT-B)");
    AppendMenu(hActions, MF_POPUP, (UINT_PTR)hFolderOpts, L"Opzioni Cartella ...");
    AppendMenu(hActions, MF_POPUP, (UINT_PTR)hDesktopIco, L"Icone Desktop ...");
    AppendMenu(hActions, MF_SEPARATOR, 0, NULL);
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

    if(NULL != hMutex) CloseHandle(hMutex);

    return (int)msg.wParam;
}