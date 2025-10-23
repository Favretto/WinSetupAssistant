# WinSetupAssistant (WTool)

**Applicazione standalone per Windows 10/11 (x64)**

WinSetupAssistant è un tool pensato per semplificare la configurazione iniziale di un sistema operativo appena installato, senza la necessità di software aggiuntivo.  
L'app utilizza solo le API native di Windows ed è progettata per essere leggera e poco invasiva sul sistema.

## Funzionalità principali

- Accesso rapido a strumenti di sistema:
  - NICs (`ncpa.cpl`)
  - TPM (`tpm.msc`)
  - Utenti e gruppi (`lusrmgr.msc`)
  - Condivisioni (`fsmgmt.msc`)
  - Eventi (`eventvwr.msc`)
  - Dispositivi (`devmgmt.msc`)
  - Task Scheduler (`taskschd.msc`)
  - Opzioni File (`control folders`)
  - Data e ora (`timedate.cpl`)
  - Prestazioni e swap
  - Temi di Windows
  - Disk Management (`diskmgmt.msc`)
  - Funzionalità Windows (`optionalfeatures.exe`)
  - Avvio automatico (cartella utente e comune)
- Abilitazione AHCI con verifica dello stato IDE/Legacy
- Icona nella system tray con menu rapido e doppio click per ripristinare
- Supporto per sempre in primo piano (Always On Top)
- Interfaccia semplice con pulsanti chiari e informazioni contestuali

## Installazione

1. Scaricare la release più recente dalla sezione [Releases](https://github.com/Favretto/WinSetupAssistant/releases)  
2. Estrarre l’eseguibile in una cartella a piacere  
3. Eseguire `WTool.exe` (consigliato come amministratore per alcune funzionalità)

## Uso

- Minimizzare la finestra: l'app si sposterà nella system tray  
- Doppio click sull'icona della tray per ripristinare la finestra  
- Click destro sull'icona della tray per accedere rapidamente ad alcune funzioni  

## Requisiti

- Windows 10 o successivi (x64)  
- Privilegi di amministratore per alcune funzioni (AHCI, Task Manager, Command Prompt)

## Licenza

Questo progetto è rilasciato sotto licenza [MIT](LICENSE).

## Autore

Alessandro Favretto
