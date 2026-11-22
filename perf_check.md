# Ping Monitor — Performance Check (locale)

## Ambiente
- OS: Windows (MinGW64 build)
- Branch: perf-check-local (locale, non pushato)
- Misurazione post-ottimizzazione, app a riposo (nessun ping attivo), VSync on

## Misurazioni empiriche

| Metrica | Prima | Dopo |
|---|---|---|
| Working Set | ~85 MB | ~69 MB |
| Private Working Set | n/d | ~43.8 MB |
| Private Bytes (commit) | n/d | ~162 MB |
| CPU a riposo | ~0.34 s cumulati | ~5% |
| Thread | 4+ | 12 |

Nota: il codice applicativo occupa circa 1 MB (storico grafico). Il resto è
riconducibile a runtime, SDL2, ImGui e soprattutto driver/contesto OpenGL.
Il README dichiara 10-20 MB: la cifra non è comparabile con il Working Set
misurato su questa macchina.

## Misura CPU isolata (solo processo ping_monitor)

Metodo: `Get-Process -Id <pid>.CPU` campionato ogni secondo per 15 s,
calcolando il delta di tempo CPU del solo processo. Nessun dato di sistema
globale è stato usato. Test con 4 target ICMP attivi (8.8.8.8, 1.1.1.1,
9.9.9.9, 208.67.222.222, intervallo 0.5 s).

| Metrica | Valore |
|---|---|
| CPU media (1 core) | ~4.98% |
| CPU massima (1 core) | ~9.4% |
| CPU media su 20 core logici | ~0.25% |
| Working Set | ~81.2 MB |
| Private Bytes | ~151.7 MB |
| Thread | 15 |

Conclusione: il carico CPU del processo è basso e stabile; il rendering
VSync a 60 FPS è il contributo principale, i ping aggiungono poco.
Non emerge la necessità di un limite a 30 FPS o di ottimizzazioni CPU urgenti.

## Analisi statica
- Allocazioni dinamiche: SDL_malloc temporanei in start_target/remove_target
  (max 512 ChartPoint) — trascurabili.
- ThreadCtx allocato per target e freed nel thread — OK.
- proc.c: malloc/free per tabelle MIB — path di errore verificato, free presente.
- VSync abilitato (SDL_GL_SetSwapInterval(1)) — CPU a riposo ~5%.
- Mutex usato correttamente attorno a stato condiviso.

## Ottimizzazioni applicate (secondo revisione ChatGPT)
1. **Mediana/Q1/Q3 precalcolate** in `chart_add_point()` (chart.c):
   niente più `qsort` e scansioni dello storico a ogni frame.
2. **MAX_POINTS 2048 → 512** (chart.h): coerente col limite GUI, ~850 KB in meno.
3. **Statistiche aggregate per punto** (somma, min, max, timeout, valid_count)
   precalcolate: legenda e global_max non scandiscono più lo storico a ogni frame.
4. **Verifica VSync** in main.c con log di `SDL_GL_GetSwapInterval()`.

## Ottimizzazioni applicate (fase 2, dopo revisione ChatGPT)
1. **`ChartData.points` dinamico** in base a `max_points`:
   - `chart_init()` alloca solo i punti necessari;
   - `chart_resize()` ridimensiona a runtime conservando i punti più recenti;
   - `chart_shutdown()` libera la memoria.
2. **`history_raw`/`history_count` rimossi da `ChartPoint`**:
   - il rendering usa solo statistiche precalcolate;
   - il buffer temporaneo per ordinare i campioni esiste solo durante
     `chart_add_point()`.
3. **Compattazione punti** centralizzata in `chart_remove_points_for_target()`
   e `chart_remove_target_and_shift()` (Start, Reset, Remove target).
4. **Thread safety**: `chart_render()` e le operazioni sul grafico avvengono
   sotto il mutex dell'app.
5. **`rolling_avg`**: limite superiore `MAX_RAW_PINGS` e azzeramento di
   `raw_count` quando cambia a runtime.

Con 30 punti la memoria del grafico passa da ~317 KB a ~4 KB.
Test di unità su init/add/resize/remove/shutdown: OK.

## Ottimizzazioni applicate (fase 3, su indicazione ChatGPT)
1. **Snapshot `target_stats` sotto mutex** in `app_render`: elimina la data
   race su `stats_avg/min/max/lost_count`.
2. **`rolling_avg` letto dopo il ping**, dentro il secondo lock: il thread
   usa sempre il valore corrente.
3. **`app_remove_target`**: ferma anche i thread con indice superiore, poi
   rimuove, poi li riavvia agli indici aggiornati. Evita associazione errata
   tra thread, target e grafico.
4. **`chart_compact` restituisce `int`**; in caso di fallimento della
   compattazione non cancella i dati. `app_remove_target` abortisce la
   rimozione se la compattazione fallisce.
5. **`chart_init`/`app_init` restituiscono `int`**; gestito il fallimento
   di `SDL_CreateMutex` e di `chart_init` in `main()`.
6. **`SDL_CreateThread` failure**: `ctx` liberato e `t->running` ripristinato.
7. **`MAX_SAMPLE_VALUES` rimosso**: si usa `MAX_RAW_PINGS` (definito in
   `config.h`) come unico limite condiviso.

## Ottimizzazioni applicate (fase 3, su indicazione ChatGPT)
1. **`chart_compact` restituisce `int`**; in caso di fallimento della
   compattazione non cancella i dati. `app_remove_target` abortisce la
   rimozione se la compattazione fallisce.
2. **`chart_init`/`app_init` restituiscono `int`**; gestito il fallimento
   di `SDL_CreateMutex` e di `chart_init` in `main()`.
3. **`SDL_CreateThread` failure**: `ctx` liberato e `t->running` ripristinato.
4. **`MAX_SAMPLE_VALUES` rimosso**: si usa `MAX_RAW_PINGS` (definito in
   `config.h`) come unico limite condiviso.

## Ottimizzazioni applicate (fase 4, code review ChatGPT)
1. **Thread safety su campi GUI**: `ip`, `interval`, `use_icmp`, `rolling_avg`
   e `running` ora usano copie locali e commit sotto mutex.
2. **`use_icmp` copiato nel thread** sotto mutex insieme a `ip` e `interval`.
3. **`chart_init`/`chart_resize`** con controllo iniziale su oggetto non
   inizializzato.
4. **`chart_add_point`** valida `raw_values` e `raw_count`.
5. **`app_init`** valida `cfg` e `target_count`.
6. **Commento** in `chart.h` corretto.

## Ottimizzazioni suggerite ma non applicate
- Riutilizzo dell'handle ICMP per thread (priorità bassa).
- Limite a 30 FPS (non necessario: CPU ~5% di un core).
- Passaggio da double a float (guadagno ~130 KB, non prioritario).

## Note di discussione con ChatGPT
- Il suggerimento di compattazione in-place con copia in avanti è stato
  corretto: non è sicuro con buffer circolare avvolto (controesempio
  documentato). Mantenuto il buffer temporaneo.
- Confermato che il lock attorno a `chart_render` non è sufficiente per
  `target_stats`: spostata la costruzione sotto mutex.

## Note di discussione con ChatGPT
- Il suggerimento di compattazione in-place con copia in avanti è stato
  corretto: non è sicuro con buffer circolare avvolto (controesempio
  documentato). Mantenuto il buffer temporaneo.
- Confermato che il lock attorno a `chart_render` non è sufficiente per
  `target_stats`: spostata la costruzione sotto mutex.

## Prossimi passi consigliati
1. Misurare CPU con ping attivi su 4 target.
2. Confrontare Private Working Set prima/dopo su macchina pulita.
3. Valutare allocazione dinamica solo se si usa max_points molto basso.

## Prompt usato per ChatGPT
"Analizza il progetto Ping Monitor (C/SDL2/ImGui, Windows). Misurazione
empirica: ~85 MB RAM a riposo vs 10-20 MB dichiarati nel README. VSync on.
Trova ottimizzazioni concrete per RAM/CPU, priorità e stima impatto."
