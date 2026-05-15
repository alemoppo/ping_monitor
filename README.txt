==============================================================================
<<<<<<< HEAD
                          PING MONITOR - C / SDL2 / ImGui
==============================================================================

DESCRIPTION
-----------
Ping Monitor is a desktop application for real-time network latency monitoring.
It replicates the functionality of the original ping.py script but written in
C with SDL2 + Dear ImGui.

Features:
  - Continuous ping to configurable IP addresses (up to 4 targets)
  - Real-time chart with boxplot for each data point
  - Colored dots and segments: trace color (<100ms), orange (100-300ms), red (>300ms)
  - Timeout detection (red X marker)
  - Configurable rolling average over N pings
  - Switch between mean and median for dots and segments
  - Global statistics: max, avg, min, lost packets, percentage
  - Per-visible-point statistics next to each IP in the legend
  - Configurable max chart points
  - Configurable ping interval
  - Reset statistics
  - Dark/Light theme toggle
  - Resizable window
  - Very low memory footprint (~10-20 MB)


FILE STRUCTURE
=======
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
>>>>>>> 2fdd924df44b540ed3aadbcdd897a504e597d49f
--------------
PingMonitor/
  Makefile
  README.txt
<<<<<<< HEAD
  ico.png
  src/
    main.c          - Entry point, SDL2 + ImGui initialization
    app.c           - Application logic and UI
    app.h
    chart.c         - Chart rendering with ImDrawList
    chart.h
    ping.c          - Ping logic (Windows, CreateProcess)
    ping.h
    config.c        - INI config file read/write
    config.h
    icon_data.h     - Generated icon pixel data (from ico.png)
  vendor/
    imgui/          - Dear ImGui (bundled)
    SDL2-2.32.10/   - SDL2 MinGW64 development files (bundled)


PREREQUISITES
-------------
1. MSYS2 with MinGW64 toolchain
   Download from: https://www.msys2.org/
   After installation, open MSYS2 MinGW64 and install the compiler:
     pacman -S mingw-w64-x86_64-gcc make

   NOTE: SDL2 and Dear ImGui are ALREADY included in the vendor/ folder.
   No need to install SDL2 via pacman.


INSTALLATION
------------
1. Install MSYS2 from https://www.msys2.org/
2. Open "MSYS2 MinGW64" (not MSYS2 UCRT64 or MSYS)
3. Install the compiler:
     pacman -Syu
     pacman -S mingw-w64-x86_64-gcc make
4. Navigate to the project directory and build:
     cd /path/to/ping-monitor
     make

   That's it. All dependencies are in vendor/.


BUILDING
--------
Open MSYS2 MinGW64 and navigate to the project directory:

  cd /path/to/ping-monitor

Build:

  make

The executable will be created at: bin/ping_monitor.exe

Build and run:

  make run

Clean build artifacts:

  make clean


DISTRIBUTION
------------
To distribute the executable to another Windows user, copy:
  - bin/ping_monitor.exe
  - vendor/SDL2-2.32.10/x86_64-w64-mingw32/bin/SDL2.dll

Place SDL2.dll in the same folder as the exe.
No installation required on the target machine.


USAGE
-----
1. Run the application:
     ./bin/ping_monitor.exe

2. Configuration:
   - IP address or hostname (default: 8.8.8.8)
   - Interval (s): seconds between pings (default: 0.5)
   - Max points: number of visible chart points (default: 30, max: 512)
   - Rolling avg pings: number of pings to average per point (default: 4)

3. Controls:
   - "Go": start monitoring
   - "Stop": stop monitoring
   - "Reset": clear statistics for that target
   - "+": add a new target row (up to 4)

4. The chart shows:
   - Boxplot per point (Q1-Q3 box, median line, whiskers min-max)
   - Colored connecting segments (trace color <100ms, orange 100-300ms, red >300ms)
   - Circle dots when no timeout, colored by value
   - Red X on points with timeout, dark red segment leading to it
   - Visible-point stats in the legend above the chart
   - Global run stats in the footer below the chart
   - Tooltip on hover over stats


TECHNICAL NOTES
---------------
- Uses CreateProcess on Windows to run ping and capture output,
  avoiding subprocess overhead of Python.
- Threads managed with SDL_Thread and mutex for thread safety.
- Chart rendering uses ImDrawList (immediate mode) — no persistent
  object overhead.
- Zero memory leaks: all buffers have fixed maximum sizes.
- Typical RAM usage: 10-20 MB (vs 40-80+ MB for the Python version).
- All dependencies (SDL2, Dear ImGui) are bundled in the project.


BUNDLED DEPENDENCIES
--------------------
- SDL2 2.32.10  -  https://github.com/libsdl-org/SDL
  License: zlib
  Location: vendor/SDL2-2.32.10/

- Dear ImGui (latest)  -  https://github.com/ocornut/imgui
  License: MIT
  Location: vendor/imgui/
=======
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
>>>>>>> 2fdd924df44b540ed3aadbcdd897a504e597d49f


TROUBLESHOOTING
---------------
<<<<<<< HEAD
Error: "gcc: command not found"
  -> Install the compiler: pacman -S mingw-w64-x86_64-gcc make

Error: "SDL2/SDL.h: No such file or directory"
  -> The vendor/SDL2-2.32.10/ folder is missing. Restore from the
     original zip or git pull.

Error: "imgui.h: No such file or directory"
  -> The vendor/imgui/ folder is missing.
     Restore with: git submodule update --init  (if configured)

Error: "SDL2.dll not found" when launching the exe
  -> Copy SDL2.dll from vendor/SDL2-2.32.10/x86_64-w64-mingw32/bin/
     into the same folder as ping_monitor.exe

The chart shows no points
  -> Check that the IP is reachable and the interval is not too low.

The program closes immediately
  -> Run from MSYS2 MinGW64 terminal to see error messages.


==============================================================================
                              END OF DOCUMENTATION
=======
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
>>>>>>> 2fdd924df44b540ed3aadbcdd897a504e597d49f
==============================================================================
