==============================================================================
                         PING MONITOR - C / SDL2 / ImGui
==============================================================================

DESCRIZIONE
-----------
Ping Monitor e un'applicazione desktop per il monitoraggio in tempo reale della
latenza di rete. Replica le funzionalita del programma ping.py originale ma
scritta in C con interfaccia grafica SDL2 + Dear ImGui.

Funzionalita:
  - Ping continuo a un indirizzo IP configurabile
  - Grafico in tempo reale con boxplot per ogni punto
  - Segmenti colorati: verde (<100ms), arancione (100-200ms), rosso (>200ms)
  - Rilevamento timeout (marker X rossa)
  - Media mobile configurabile su N ping
  - Statistiche globali: media, minimo, massimo
  - Numero massimo di punti configurabile
  - Intervallo tra i ping configurabile
  - Reset statistiche
  - Finestra ridimensionabile
  - Footprint di memoria molto ridotto (~10-20 MB)


STRUTTURA FILE
--------------
PingMonitor/
  Makefile
  README.txt
  src/
    main.c          - Entry point, inizializzazione SDL2 + ImGui
    app.c           - Logica applicativa e UI
    app.h
    chart.c         - Rendering del grafico con ImDrawList
    chart.h
    ping.c          - Logica ping (Windows, CreateProcess)
    ping.h
  vendor/
    imgui/          - Dear ImGui (incluso)
    SDL2-2.32.10/   - SDL2 sviluppo MinGW64 (incluso)


PREREQUISITI
------------
1. MSYS2 con toolchain MinGW64
   Scarica da: https://www.msys2.org/
   Dopo l'installazione, apri MSYS2 MinGW64 e installa solo il compilatore:
     pacman -S mingw-w64-x86_64-gcc make

   NOTA: SDL2 e Dear ImGui sono GIA inclusi nella cartella vendor/.
   NON serve installare SDL2 tramite pacman.


INSTALLAZIONE
-------------
1. Installa MSYS2 da https://www.msys2.org/
2. Apri "MSYS2 MinGW64" (non MSYS2 UCRT64 o MSYS)
3. Installa il compilatore:
     pacman -Syu
     pacman -S mingw-w64-x86_64-gcc make
4. Vai alla directory del progetto e compila:
     cd /c/Users/alemo/Documents/Agentic\ 1
     make

   Questo e tutto. Le dipendenze sono gia presenti in vendor/.


COMPILAZIONE
------------
Apri MSYS2 MinGW64 e vai alla directory del progetto:
  cd /c/Users/alemo/Documents/Agentic\ 1

Compila:
  make

L'eseguibile sara creato in: bin/ping_monitor.exe

Compila ed esegui:
  make run

Pulire i file compilati:
  make clean


DISTRIBUZIONE
-------------
Per distribuire l'eseguibile a un altro utente Windows, copia:
  - bin/ping_monitor.exe
  - vendor/SDL2-2.32.10/x86_64-w64-mingw32/bin/SDL2.dll

Metti SDL2.dll nella stessa cartella dell'exe.
NON serve installare nulla sulla macchina di destinazione.


UTILIZZO
--------
1. Esegui l'applicazione:
     ./bin/ping_monitor.exe

2. Configurazione:
   - "IP da pingare": indirizzo IP o hostname (default: 8.8.8.8)
   - "Intervallo (s)": secondi tra un ping e l'altro (default: 0.5)
   - "N. massimo punti": numero massimo di punti sul grafico (default: 30, max: 512)
   - "Media su N ping": numero di ping da mediare per ogni punto (default: 4)

3. Controlli:
   - "Avvia": inizia il monitoraggio
   - "Ferma": ferma il monitoraggio
   - "Reset Stats": azzera le statistiche globali

4. Il grafico mostra:
   - Boxplot per ogni punto (Q1-Q3 box, mediana, whiskers min-max)
   - Linee colorate che collegano i punti
   - X rossa sui punti con timeout
   - Statistiche globali in alto a sinistra


NOTE TECNICHE
-------------
- Il programma usa CreateProcess su Windows per eseguire il ping e catturare
  l'output, evitando subprocess overhead del Python.
- I thread sono gestiti con SDL_Thread con mutex per la thread safety.
- Il rendering del grafico usa ImDrawList (immediate mode) - nessun overhead
  di oggetti persistenti come matplotlib.
- Nessun leak di memoria: tutti i buffer hanno dimensione fissa massima.
- Consumo RAM tipico: 10-20 MB (vs 40-80+ MB della versione Python).
- Tutte le dipendenze (SDL2, Dear ImGui) sono incluse nel progetto.


DIPENDENZE INCLUSE
------------------
- SDL2 2.32.10  -  https://github.com/libsdl-org/SDL
  Licenza: zlib
  File: vendor/SDL2-2.32.10/

- Dear ImGui (latest)  -  https://github.com/ocornut/imgui
  Licenza: MIT
  File: vendor/imgui/


TROUBLESHOOTING
---------------
Errore: "gcc: command not found"
  -> Installa il compilatore: pacman -S mingw-w64-x86_64-gcc make

Errore: "SDL2/SDL.h: No such file or directory"
  -> La cartella vendor/SDL2-2.32.10 e mancante.
     Ripristinala dallo zip originale.

Errore: "imgui.h: No such file or directory"
  -> La cartella vendor/imgui e mancante.
     Ripristinala con: git clone https://github.com/ocornut/imgui.git vendor/imgui

Errore: "SDL2.dll not found" all'avvio dell'exe
  -> Copia SDL2.dll dalla cartella vendor/SDL2-2.32.10/x86_64-w64-mingw32/bin/
     nella stessa cartella di ping_monitor.exe

Il grafico non mostra i punti
  -> Verifica che l'IP sia raggiungibile e che l'intervallo non sia troppo basso.

Il programma si chiude subito
  -> Esegui da terminale MSYS2 MinGW64 per vedere gli errori.


==============================================================================
                              FINE DOCUMENTAZIONE
==============================================================================
