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

## Ottimizzazioni suggerite ma non applicate (per scelta di ChatGPT: misurare prima)
- Allocazione dinamica di `ChartData` in base a max_points.
- Riutilizzo dell'handle ICMP per thread (priorità bassa).
- Limite a 30 FPS (solo se VSync non effettivo).
- Riduzione ulteriore delle scansioni con cache persistenti.

## Prossimi passi consigliati
1. Misurare CPU con ping attivi su 4 target.
2. Confrontare Private Working Set prima/dopo su macchina pulita.
3. Valutare allocazione dinamica solo se si usa max_points molto basso.

## Prompt usato per ChatGPT
"Analizza il progetto Ping Monitor (C/SDL2/ImGui, Windows). Misurazione
empirica: ~85 MB RAM a riposo vs 10-20 MB dichiarati nel README. VSync on.
Trova ottimizzazioni concrete per RAM/CPU, priorità e stima impatto."
