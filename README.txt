==============================================================================
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
--------------
PingMonitor/
  Makefile
  README.txt
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


TROUBLESHOOTING
---------------
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
==============================================================================
