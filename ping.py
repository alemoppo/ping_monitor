import tkinter as tk
from tkinter import ttk
import threading
import time
import subprocess
import platform
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation
import re
import numpy as np

class PingApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Ping Monitor")

        self.running = False
        self.ping_thread = None

        self.max_points = 30
        self.rolling_avg = 2

        self.raw_pings = []
        self.ping_data = deque(maxlen=self.max_points)
        self.timestamps = deque(maxlen=self.max_points)
        self.history_map = deque(maxlen=self.max_points)
        self.points = []  # Coordinate dei punti sul grafico
        
        # Statistiche
        self.all_valid_pings = []  # Per calcolare statistiche globali
        
        # Variabile per tracciare il punto attualmente evidenziato
        self.current_highlight = -1

        # Variabili per monitorare cambiamenti parametri
        self.last_params = {}

        self.root.option_add("*Font", "Helvetica 12")

        # UI
        input_frame = ttk.Frame(root, padding=20)
        input_frame.pack()

        # Prima riga di input con pulsante Avvia/Ferma a destra
        input_row1 = ttk.Frame(input_frame)
        input_row1.pack(pady=5, fill=tk.X)

        ttk.Label(input_row1, text="IP da pingare:").pack(side=tk.LEFT, padx=5)
        self.ip_entry = ttk.Entry(input_row1, width=20)
        self.ip_entry.insert(0, "8.8.8.8")
        self.ip_entry.pack(side=tk.LEFT, padx=10)
        self.ip_entry.bind('<KeyRelease>', self.on_param_change)

        ttk.Label(input_row1, text="Intervallo (s):").pack(side=tk.LEFT, padx=5)
        self.interval_entry = ttk.Entry(input_row1, width=10)
        self.interval_entry.insert(0, "0.5")
        self.interval_entry.pack(side=tk.LEFT, padx=10)
        self.interval_entry.bind('<KeyRelease>', self.on_param_change)

        # Pulsante Avvia/Ferma
        self.start_button = ttk.Button(input_row1, text="Avvia", command=self.toggle_ping)
        self.start_button.pack(side=tk.RIGHT, padx=10)

        # Seconda riga di input
        input_row2 = ttk.Frame(input_frame)
        input_row2.pack(pady=5, fill=tk.X)

        ttk.Label(input_row2, text="N. massimo punti:").pack(side=tk.LEFT, padx=5)
        self.maxpoints_entry = ttk.Entry(input_row2, width=10)
        self.maxpoints_entry.insert(0, str(self.max_points))
        self.maxpoints_entry.pack(side=tk.LEFT, padx=10)
        self.maxpoints_entry.bind('<KeyRelease>', self.on_param_change)

        ttk.Label(input_row2, text="Media su N ping:").pack(side=tk.LEFT, padx=5)
        self.avg_entry = ttk.Entry(input_row2, width=10)
        self.avg_entry.insert(0, "4")
        self.avg_entry.pack(side=tk.LEFT, padx=10)
        self.avg_entry.bind('<KeyRelease>', self.on_param_change)

        # Pulsante Reset Statistiche nella seconda riga
        self.reset_button = ttk.Button(input_row2, text="Reset Stats", command=self.reset_stats)
        self.reset_button.pack(side=tk.RIGHT, padx=10)

        # Grafico
        self.figure, self.ax = plt.subplots(figsize=(9, 5))
        self.figure.subplots_adjust(left=0.1, right=0.9, top=0.9, bottom=0.15)
        self.ax.set_title("Ping (ms)")
        self.ax.set_xlabel("Tempo")
        self.ax.set_ylabel("ms")
        
        self.canvas = FigureCanvasTkAgg(self.figure, master=root)
        self.canvas_widget = self.canvas.get_tk_widget()
        self.canvas_widget.pack(fill=tk.BOTH, expand=True)
        
        # Connessione eventi con binding Tkinter
        root.bind("<Configure>", self.on_resize)
        
        # Inizializza parametri precedenti
        self.update_last_params()
        
        # Aggiunto save_count per evitare il warning
        self.ani = animation.FuncAnimation(self.figure, self.update_plot, 
                                        interval=800, save_count=100)

    def get_current_params(self):
        """Restituisce i parametri correnti come dizionario"""
        return {
            'ip': self.ip_entry.get(),
            'interval': self.interval_entry.get(),
            'max_points': self.maxpoints_entry.get(),
            'avg': self.avg_entry.get()
        }

    def update_last_params(self):
        """Aggiorna i parametri precedenti"""
        self.last_params = self.get_current_params()

    def params_changed(self):
        """Controlla se i parametri sono cambiati"""
        return self.get_current_params() != self.last_params

    def on_param_change(self, event=None):
        """Gestisce il cambio di parametri durante l'esecuzione"""
        if self.running and self.params_changed():
            # Riavvia automaticamente con i nuovi parametri
            self.restart_with_new_params()

    def restart_with_new_params(self):
        """Riavvia il ping con i nuovi parametri mantenendo i dati esistenti"""
        # Ferma temporaneamente
        self.running = False
        if self.ping_thread and self.ping_thread.is_alive():
            self.ping_thread.join(timeout=1)
        
        # Aggiorna i parametri
        try:
            new_max = int(self.maxpoints_entry.get())
            if new_max > 0 and new_max != self.max_points:
                self.max_points = new_max
                # Ridimensiona le deque esistenti mantenendo i dati
                self.ping_data = deque(self.ping_data, maxlen=self.max_points)
                self.timestamps = deque(self.timestamps, maxlen=self.max_points)
                self.history_map = deque(self.history_map, maxlen=self.max_points)
        except ValueError:
            pass

        try:
            self.rolling_avg = max(1, int(self.avg_entry.get()))
        except ValueError:
            self.rolling_avg = 1

        # Aggiorna parametri precedenti
        self.update_last_params()
        
        # Riavvia
        self.running = True
        self.ping_thread = threading.Thread(target=self.ping_loop, daemon=True)
        self.ping_thread.start()

    def reset_stats(self):
        """Reset delle statistiche"""
        self.all_valid_pings = []

    def get_stats_text(self):
        """Restituisce il testo delle statistiche per il grafico"""
        if self.all_valid_pings:
            avg_value = sum(self.all_valid_pings) / len(self.all_valid_pings)
            min_value = min(self.all_valid_pings)
            max_value = max(self.all_valid_pings)
            
            return f"Media: {avg_value:.2f}ms\nMin: {min_value:.2f}ms\nMax: {max_value:.2f}ms"
        else:
            return "Media: N/A\nMin: N/A\nMax: N/A"

    def update_stats_labels(self):
        """Aggiorna le etichette delle statistiche (ora non più utilizzata)"""
        pass

    def on_resize(self, event):
        self.canvas_widget.config(width=event.width, height=event.height)

    def toggle_ping(self):
        if not self.running:
            try:
                new_max = int(self.maxpoints_entry.get())
                if new_max > 0:
                    self.max_points = new_max
                    self.ping_data = deque(maxlen=self.max_points)
                    self.timestamps = deque(maxlen=self.max_points)
                    self.history_map = deque(maxlen=self.max_points)
            except ValueError:
                pass

            try:
                self.rolling_avg = max(1, int(self.avg_entry.get()))
            except ValueError:
                self.rolling_avg = 1

            # Aggiorna parametri precedenti
            self.update_last_params()

            self.running = True
            self.start_button.config(text="Ferma")
            self.ping_thread = threading.Thread(target=self.ping_loop, daemon=True)
            self.ping_thread.start()
        else:
            self.running = False
            self.start_button.config(text="Avvia")

    def ping_loop(self):
        ip = self.ip_entry.get()
        try:
            interval = float(self.interval_entry.get())
        except ValueError:
            interval = 1

        while self.running:
            ping = self.do_ping(ip)
            self.raw_pings.append(ping)

            # Aggiungi alle statistiche globali se valido
            if ping != -1:
                self.all_valid_pings.append(ping)

            if len(self.raw_pings) >= self.rolling_avg:
                values_used = self.raw_pings[:self.rolling_avg]
                valid_pings = [p for p in values_used if p != -1]
                avg_ping = sum(valid_pings) / len(valid_pings) if valid_pings else -1

                timestamp = time.strftime("%H:%M:%S")
                self.ping_data.append(avg_ping)
                self.timestamps.append(timestamp)
                self.history_map.append(values_used.copy())
                self.raw_pings = self.raw_pings[self.rolling_avg:]

            time.sleep(interval)

    def do_ping(self, ip):
        system = platform.system()
        if system == "Windows":
            command = ["ping", "-n", "1", ip]
        else:
            command = ["ping", "-c", "1", ip]
        try:
            output = subprocess.check_output(command, stderr=subprocess.STDOUT, universal_newlines=True, timeout=3)
            if "Richiesta scaduta" in output or "Request timed out" in output:
                return -1
            match = re.search(r"durata[=<]\s*(\d+)", output)
            if not match:
                match = re.search(r"time[=<]\s*(\d+\.?\d*)", output)
            if match:
                return float(match.group(1))
        except Exception:
            return -1
        return -1

    def update_plot(self, i=0):
        self.ax.clear()
        self.ax.grid(True)
        self.ax.set_title("Ping (ms)")
        self.ax.set_xlabel("Tempo")
        self.ax.set_ylabel("ms")

        if len(self.ping_data) == 0:
            self.points = []
            return

        x = list(range(len(self.ping_data)))
        y_raw = list(self.ping_data)
        y_plot = [0 if v == -1 else v for v in y_raw]

        self.points = list(zip(x, y_plot))

        # Array per tenere traccia dei timeout per ogni punto
        has_timeout = []

        # Disegna boxplot per ogni punto
        for i, (x_pos, y) in enumerate(self.points):
            # Assicurati che i dati esistano per l'indice
            if i < len(self.history_map):
                raw_vals = self.history_map[i]
                # Controlla se ci sono richieste scadute
                timeout = any(v == -1 for v in raw_vals)
                has_timeout.append(timeout)
                self.draw_boxplot(i, x_pos, raw_vals, timeout)

        # Disegna segmenti colorati tra i punti, considerando i timeout
        self.draw_colored_segments(y_raw, has_timeout)

        # Assicurati che il grafico mostri tutto il range necessario
        self.ax.set_xlim(-0.5, max(9.5, len(x) - 0.5))
        
        # Calcolo migliore del limite massimo dell'asse Y per valori alti
        # Modifica per garantire che tutti i valori siano visibili
        valid_y = [y for y in y_plot if y > 0]
        
        # Trova il massimo considerando anche i valori nei boxplot
        max_in_boxplots = 0
        for raw_vals in self.history_map:
            valid_vals = [v for v in raw_vals if v != -1]
            if valid_vals:
                max_in_boxplots = max(max_in_boxplots, max(valid_vals))
        
        max_y = max(max(valid_y) if valid_y else 10, max_in_boxplots)
        self.ax.set_ylim(-5, max(10, max_y * 1.2))

        # Etichette dinamiche asse X
        try:
            width_px = self.canvas_widget.winfo_width()
            approx_labels = max(2, width_px // 100)
        except:
            approx_labels = 10

        step = max(1, len(self.timestamps) // approx_labels)
        xticks = list(range(0, len(self.timestamps), step))
        if len(self.timestamps) > 0 and len(self.timestamps) - 1 not in xticks:
            xticks.append(len(self.timestamps) - 1)
        xticklabels = [self.timestamps[i] for i in xticks]

        self.ax.set_xticks(xticks)
        self.ax.set_xticklabels(xticklabels, rotation=45, ha='right', fontsize=8)
        
        # Reset highlight elements
        self.current_highlight = -1

        # Aggiungi le statistiche sul grafico in alto a sinistra
        stats_text = self.get_stats_text()
        self.ax.text(0.02, 0.98, stats_text, transform=self.ax.transAxes, 
                    verticalalignment='top', horizontalalignment='left',
                    bbox=dict(boxstyle='round', facecolor='white', alpha=0.8),
                    fontsize=10)

    def draw_colored_segments(self, y_raw, has_timeout):
        """Disegna segmenti colorati tra i punti, rossi se collegano un timeout"""
        if len(y_raw) < 2:
            return
            
        for i in range(len(y_raw) - 1):
            x1, y1 = i, y_raw[i] if y_raw[i] != -1 else 0
            x2, y2 = i+1, y_raw[i+1] if y_raw[i+1] != -1 else 0
            
            # Determina il colore del segmento
            # Rosso se uno dei due punti è un timeout o se il ping è molto alto
            if has_timeout[i+1]:
                color = 'red'
            elif y2 >= 200:
                color = 'red'
            elif y2 >= 100:
                color = 'orange'
            else:
                color = 'green'
                
            self.ax.plot([x1, x2], [y1, y2], color=color, linestyle='-', linewidth=1.5, zorder=1)
    
    def draw_boxplot(self, index, x_pos, raw_vals, has_timeout=False):
        """Disegna un boxplot per il punto specificato, aggiungendo una X rossa in caso di timeout"""
        
        valid_vals = [v for v in raw_vals if v != -1]
        
        # Determina il valore y medio per posizionare la X in caso di timeout
        if valid_vals:
            y_center = np.mean(valid_vals)
        else:
            y_center = 0  # Default se non ci sono valori validi
        
        if not valid_vals:
            # Caso solo timeout
            timeout_marker = plt.Line2D([x_pos], [y_center], marker='x', 
                                       markersize=10, markeredgewidth=2,
                                       markerfacecolor='none', markeredgecolor='red', 
                                       alpha=1.0,
                                       zorder=5)
            self.ax.add_line(timeout_marker)
            return
            
        elif len(valid_vals) == 1:
            # Caso singolo valore
            y = valid_vals[0]
            box_width = 0.2
            
            # Box semplice
            color = 'skyblue'
            alpha = 0.5
            zorder = 2
            linewidth = 1
            
            # Disegna un piccolo box attorno al punto
            box = plt.Rectangle((x_pos - box_width/2, y - 2), box_width, 4, 
                              fill=True, alpha=alpha, color=color, zorder=zorder,
                              linewidth=linewidth)
            self.ax.add_patch(box)
            
            # Punto centrale
            point = plt.Line2D([x_pos], [y], marker='o', markersize=4,
                             markerfacecolor='blue', markeredgecolor='blue',
                             alpha=0.8,
                             zorder=zorder+1)
            self.ax.add_line(point)
            
        else:
            # Caso con più valori: mini-boxplot
            min_val = min(valid_vals)
            max_val = max(valid_vals)
            median = np.median(valid_vals)
            q1 = np.percentile(valid_vals, 25)
            q3 = np.percentile(valid_vals, 75)
            
            # Calcola le dimensioni del boxplot
            box_width = 0.3
            x1, x2 = x_pos - box_width/2, x_pos + box_width/2
            
            # Colori e stili
            box_color = 'skyblue'
            line_color = 'blue'
            alpha = 0.5
            zorder = 2
            linewidth = 1
            
            # Disegna il boxplot personalizzato
            # 1. Box dal Q1 al Q3
            box = plt.Rectangle((x1, q1), box_width, q3-q1, fill=True, 
                              alpha=alpha, color=box_color, zorder=zorder,
                              linewidth=linewidth)
            self.ax.add_patch(box)
            
            # 2. Linea mediana
            median_line = plt.Line2D([x1, x2], [median, median], 
                                   color='black', linewidth=linewidth+1, 
                                   zorder=zorder+1,
                                   alpha=0.7)
            self.ax.add_line(median_line)
            
            # 3. Linea verticale dal min al max (whisker)
            vline = plt.Line2D([x_pos, x_pos], [min_val, max_val], 
                             color=line_color, linewidth=linewidth,
                             zorder=zorder,
                             alpha=0.6)
            self.ax.add_line(vline)
            
            # 4. Whiskers orizzontali
            min_line = plt.Line2D([x1, x2], [min_val, min_val], 
                                color=line_color, linewidth=linewidth,
                                zorder=zorder,
                                alpha=0.6)
            max_line = plt.Line2D([x1, x2], [max_val, max_val], 
                                color=line_color, linewidth=linewidth,
                                zorder=zorder,
                                alpha=0.6)
            
            self.ax.add_line(min_line)
            self.ax.add_line(max_line)

        # Aggiungi X rossa se c'è almeno un timeout
        if has_timeout:
            timeout_marker = plt.Line2D([x_pos], [y_center], marker='x', 
                                      markersize=10, markeredgewidth=2,
                                      markerfacecolor='none', markeredgecolor='red',
                                      alpha=1.0,
                                      zorder=5)  # Alto zorder per renderlo sopra tutto
            self.ax.add_line(timeout_marker)

# Avvio
if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("1100x800")
    app = PingApp(root)
    root.mainloop()