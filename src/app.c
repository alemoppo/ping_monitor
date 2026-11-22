#include "app.h"
#include "proc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    App* app;
    int index;
} ThreadCtx;

static int ping_thread_func(void* data) {
    ThreadCtx* ctx = (ThreadCtx*)data;
    App* app = ctx->app;
    int idx = ctx->index;
    PingTarget* t = &app->targets[idx];
    SDL_free(ctx);

    char ip[128];
    float interval;
    int use_icmp;

    while (1) {
        SDL_LockMutex(app->mutex);
        if (t->pending_stop) {
            t->pending_stop = 0;
            t->running = 0;
            SDL_UnlockMutex(app->mutex);
            return 0;
        }
        memcpy(ip, t->ip, sizeof(ip));
        interval = t->interval;
        use_icmp = t->use_icmp;
        SDL_UnlockMutex(app->mutex);

        double ms = 0;
        int ret;
        if (use_icmp)
            ret = do_ping_icmp(ip, &ms);
        else
            ret = do_ping_legacy(ip, &ms);

        SDL_LockMutex(app->mutex);
        if (t->pending_stop) {
            SDL_UnlockMutex(app->mutex);
            return 0;
        }

        int rolling_avg = app->rolling_avg;

        if (t->raw_count < MAX_RAW_PINGS) {
            t->raw_pings[t->raw_count++] = (ret == 0) ? ms : -1.0;
        }

        if (ret == 0) {
            t->valid_count++;
            if (t->has_stats) {
                if (ms < t->stats_min) t->stats_min = ms;
                if (ms > t->stats_max) t->stats_max = ms;
                t->stats_avg = (t->stats_avg * (t->valid_count - 1) + ms) / t->valid_count;
            } else {
                t->stats_min = ms;
                t->stats_max = ms;
                t->stats_avg = ms;
                t->has_stats = 1;
            }
        } else if (ret == -1) {
            t->lost_count++;
        }

        if (t->raw_count >= rolling_avg) {
            double sum = 0;
            int count = 0;
            double raw_buf[MAX_RAW_PINGS];
            int raw_buf_count = 0;

            for (int i = 0; i < rolling_avg; i++) {
                double v = t->raw_pings[i];
                raw_buf[raw_buf_count++] = v;
                if (v >= 0) { sum += v; count++; }
            }

            double avg = (count > 0) ? sum / count : -1.0;

            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char ts[16];
            strftime(ts, sizeof(ts), "%H:%M:%S", tm_info);

            chart_add_point(&app->chart, idx, avg, raw_buf, raw_buf_count, ts);

            t->raw_count -= rolling_avg;
            memmove(t->raw_pings, t->raw_pings + rolling_avg, t->raw_count * sizeof(double));
        }
        SDL_UnlockMutex(app->mutex);

        SDL_Delay((Uint32)(interval * 1000.0f));
    }
    return 0;
}

static void start_target_no_reset(App* app, int idx) {
    PingTarget* t = &app->targets[idx];
    if (t->running) return;

    t->pending_stop = 0;
    t->running = 1;

    ThreadCtx* ctx = (ThreadCtx*)SDL_malloc(sizeof(ThreadCtx));
    if (!ctx) {
        SDL_LockMutex(app->mutex);
        t->running = 0;
        SDL_UnlockMutex(app->mutex);
        return;
    }
    ctx->app = app;
    ctx->index = idx;
    t->thread = SDL_CreateThread(ping_thread_func, "ping_thread", ctx);
    if (!t->thread) {
        SDL_free(ctx);
        SDL_LockMutex(app->mutex);
        t->running = 0;
        SDL_UnlockMutex(app->mutex);
    }
}

static void start_target(App* app, int idx) {
    PingTarget* t = &app->targets[idx];

    SDL_LockMutex(app->mutex);
    if (t->running) {
        SDL_UnlockMutex(app->mutex);
        return;
    }
    t->pending_stop = 0;
    t->raw_count = 0;
    t->valid_count = 0;
    t->has_stats = 0;
    t->lost_count = 0;
    t->stats_avg = 0; t->stats_min = 0; t->stats_max = 0;
    t->running = 1;
    int compact_ok = chart_remove_points_for_target(&app->chart, idx);
    SDL_UnlockMutex(app->mutex);

    if (!compact_ok) {
        SDL_LockMutex(app->mutex);
        t->running = 0;
        SDL_UnlockMutex(app->mutex);
        return;
    }

    ThreadCtx* ctx = (ThreadCtx*)SDL_malloc(sizeof(ThreadCtx));
    if (!ctx) {
        SDL_LockMutex(app->mutex);
        t->running = 0;
        SDL_UnlockMutex(app->mutex);
        return;
    }
    ctx->app = app;
    ctx->index = idx;
    t->thread = SDL_CreateThread(ping_thread_func, "ping_thread", ctx);
    if (!t->thread) {
        SDL_free(ctx);
        SDL_LockMutex(app->mutex);
        t->running = 0;
        SDL_UnlockMutex(app->mutex);
    }
}

static void stop_target(App* app, int idx) {
    PingTarget* t = &app->targets[idx];
    SDL_LockMutex(app->mutex);
    if (!t->running) {
        SDL_UnlockMutex(app->mutex);
        return;
    }
    t->pending_stop = 1;
    SDL_UnlockMutex(app->mutex);
    SDL_WaitThread(t->thread, NULL);
    SDL_LockMutex(app->mutex);
    t->thread = NULL;
    t->running = 0;
    SDL_UnlockMutex(app->mutex);
}

static void start_or_resolve_target(App* app, int idx) {
    PingTarget* t = &app->targets[idx];

    SDL_LockMutex(app->mutex);
    if (t->running) {
        SDL_UnlockMutex(app->mutex);
        return;
    }
    char ip[128];
    int use_icmp;
    strncpy(ip, t->ip, sizeof(ip) - 1);
    ip[sizeof(ip) - 1] = '\0';
    use_icmp = t->use_icmp;
    SDL_UnlockMutex(app->mutex);

    if (is_valid_ip(ip)) {
        start_target(app, idx);
        return;
    }

    char ips[PROC_MAX_IPS][PROC_IP_LEN];
    int n = 0;
    resolve_process_ips(ip, ips, &n);

    if (n > 0) {
        int original_count = app->target_count;
        SDL_LockMutex(app->mutex);
        strncpy(t->ip, ips[0], sizeof(t->ip) - 1);
        t->ip[sizeof(t->ip) - 1] = '\0';
        for (int k = 1; k < n; k++) {
            if (app->target_count >= MAX_TARGETS) break;
            app_add_target(app);
            int ni = app->target_count - 1;
            strncpy(app->targets[ni].ip, ips[k], sizeof(app->targets[ni].ip) - 1);
            app->targets[ni].ip[sizeof(app->targets[ni].ip) - 1] = '\0';
            app->targets[ni].use_icmp = use_icmp;
        }
        SDL_UnlockMutex(app->mutex);
        app->config_dirty = 1;
        start_target(app, idx);
        for (int k = original_count; k < app->target_count; k++) {
            start_target(app, k);
        }
    } else {
        SDL_LockMutex(app->mutex);
        t->use_icmp = 0;
        SDL_UnlockMutex(app->mutex);
        app->config_dirty = 1;
        start_target(app, idx);
    }
}

int app_init(App* app, const AppConfig* cfg) {
    if (!app || !cfg) return 0;
    if (cfg->target_count < 1 || cfg->target_count > MAX_TARGETS) return 0;
    if (cfg->rolling_avg < 1 || cfg->rolling_avg > MAX_RAW_PINGS) return 0;

    memset(app, 0, sizeof(App));
    app->target_count = cfg->target_count;
    app->max_points = cfg->max_points;
    app->rolling_avg = cfg->rolling_avg;
    app->dark_mode = cfg->dark_mode;
    app->use_mean = cfg->use_mean;
    app->config_dirty = 0;
    app->mutex = SDL_CreateMutex();
    if (!app->mutex) return 0;

    if (!chart_init(&app->chart, app->max_points)) {
        SDL_DestroyMutex(app->mutex);
        app->mutex = NULL;
        return 0;
    }
    app->max_points = app->chart.max_points;

    for (int i = 0; i < app->target_count; i++) {
        PingTarget* t = &app->targets[i];
        strncpy(t->ip, cfg->targets[i].ip[0] ? cfg->targets[i].ip : (i == 0 ? "8.8.8.8" : "1.1.1.1"), 127);
        t->ip[127] = '\0';
        t->interval = cfg->targets[i].interval > 0 ? cfg->targets[i].interval : 0.5f;
        t->use_icmp = cfg->targets[i].use_icmp;
    }
    return 1;
}

void app_shutdown(App* app) {
    for (int i = 0; i < app->target_count; i++) {
        PingTarget* t = &app->targets[i];
        if (t->thread) {
            SDL_LockMutex(app->mutex);
            t->pending_stop = 1;
            SDL_UnlockMutex(app->mutex);
            SDL_WaitThread(t->thread, NULL);
            t->thread = NULL;
        }
    }
    chart_shutdown(&app->chart);

    if (app->mutex) {
        SDL_DestroyMutex(app->mutex);
        app->mutex = NULL;
    }
}

void app_toggle_theme(App* app) {
    app->dark_mode = !app->dark_mode;
    app->config_dirty = 1;
}

void app_add_target(App* app) {
    if (app->target_count >= MAX_TARGETS) return;
    int idx = app->target_count;
    PingTarget* t = &app->targets[idx];
    memset(t, 0, sizeof(PingTarget));
    strncpy(t->ip, "1.1.1.1", 127);
    t->interval = 0.5f;
    t->use_icmp = 1;
    app->target_count++;
    app->config_dirty = 1;
}

void app_remove_target(App* app, int idx) {
    if (app->target_count <= 1 || idx < 0 || idx >= app->target_count) return;

    int running_before[MAX_TARGETS];
    SDL_LockMutex(app->mutex);
    for (int i = idx; i < app->target_count; i++) {
        running_before[i] = app->targets[i].running;
    }
    SDL_UnlockMutex(app->mutex);
    for (int i = idx; i < app->target_count; i++) {
        if (running_before[i]) stop_target(app, i);
    }

    SDL_LockMutex(app->mutex);
    int ok = chart_remove_target_and_shift(&app->chart, idx);
    SDL_UnlockMutex(app->mutex);
    if (!ok) {
        for (int i = idx; i < app->target_count; i++) {
            if (running_before[i]) start_target(app, i);
        }
        return;
    }

    for (int i = idx; i < app->target_count - 1; i++) {
        memcpy(&app->targets[i], &app->targets[i + 1], sizeof(PingTarget));
    }
    memset(&app->targets[app->target_count - 1], 0, sizeof(PingTarget));
    app->target_count--;

    for (int i = idx; i < app->target_count; i++) {
        if (running_before[i + 1]) start_target_no_reset(app, i);
    }

    app->config_dirty = 1;
}

void app_render(App* app) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBackground;

    ImGui::Begin("Ping Monitor", NULL, flags);

    float gap = 14.0f;
    float theme_btn_w = 30.0f;

    const char* theme_label = app->dark_mode ? "D" : "L";
    if (ImGui::Button(theme_label, ImVec2(theme_btn_w, 0))) app_toggle_theme(app);
    ImGui::SameLine(0, gap);

    ImGui::Text("Media su N ping:");
    ImGui::SameLine(0, gap);
    ImGui::SetNextItemWidth(50.0f);
    int rolling_avg_val;
    SDL_LockMutex(app->mutex);
    rolling_avg_val = app->rolling_avg;
    SDL_UnlockMutex(app->mutex);
    ImGui::InputInt("##rolling_avg", &rolling_avg_val, 0, 0);
    if (rolling_avg_val < 1) rolling_avg_val = 1;
    if (rolling_avg_val > MAX_RAW_PINGS) rolling_avg_val = MAX_RAW_PINGS;
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        SDL_LockMutex(app->mutex);
        app->rolling_avg = rolling_avg_val;
        for (int i = 0; i < app->target_count; i++) app->targets[i].raw_count = 0;
        SDL_UnlockMutex(app->mutex);
        app->config_dirty = 1;
    }

    ImGui::SameLine(0, gap);
    ImGui::Text("Max campioni:");
    ImGui::SameLine(0, gap);
    ImGui::SetNextItemWidth(50.0f);
    int max_points_val;
    SDL_LockMutex(app->mutex);
    max_points_val = app->max_points;
    SDL_UnlockMutex(app->mutex);
    ImGui::InputInt("##max_points", &max_points_val, 0, 0);
    if (max_points_val < 5) max_points_val = 5;
    if (max_points_val > MAX_POINTS) max_points_val = MAX_POINTS;
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        SDL_LockMutex(app->mutex);
        int resized = chart_resize(&app->chart, max_points_val);
        if (resized) {
            app->max_points = max_points_val;
            app->config_dirty = 1;
        } else {
            max_points_val = app->max_points;
        }
        SDL_UnlockMutex(app->mutex);
    }

    ImGui::SameLine(0, gap);
    bool use_mean_val = (app->use_mean != 0);
    if (ImGui::Checkbox("Media##use_mean", &use_mean_val)) {
        app->use_mean = use_mean_val ? 1 : 0;
        app->config_dirty = 1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Se attivo usa la media, altrimenti la mediana");

    ImGui::Separator();

    float item_w = 140.0f;
    float btn_w = 80.0f;

    for (int i = 0; i < app->target_count; i++) {
        PingTarget* t = &app->targets[i];
        char id_buf[32];

        ImGui::PushID(i);

        snprintf(id_buf, sizeof(id_buf), "##ip%d", i);
        ImGui::SetNextItemWidth(item_w);
        char ip_buf[128];
        int use_icmp_val;
        float interval_val;
        SDL_LockMutex(app->mutex);
        strncpy(ip_buf, t->ip, sizeof(ip_buf) - 1);
        ip_buf[sizeof(ip_buf) - 1] = '\0';
        use_icmp_val = t->use_icmp;
        interval_val = t->interval;
        SDL_UnlockMutex(app->mutex);
        ImGui::InputText(id_buf, ip_buf, sizeof(ip_buf), 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            SDL_LockMutex(app->mutex);
            strncpy(t->ip, ip_buf, sizeof(t->ip) - 1);
            t->ip[sizeof(t->ip) - 1] = '\0';
            SDL_UnlockMutex(app->mutex);
            app->config_dirty = 1;
        }
        ImGui::SameLine(0, 4.0f);

        bool icmp_val = (use_icmp_val != 0);
        snprintf(id_buf, sizeof(id_buf), "ICMP##icmp%d", i);
        if (ImGui::Checkbox(id_buf, &icmp_val)) {
            SDL_LockMutex(app->mutex);
            t->use_icmp = icmp_val ? 1 : 0;
            SDL_UnlockMutex(app->mutex);
            app->config_dirty = 1;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("ICMP API (piu veloce) / Processo ping.exe (compatibile)");
        ImGui::SameLine(0, gap);

        snprintf(id_buf, sizeof(id_buf), "##int%d", i);
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputFloat(id_buf, &interval_val, 0.1f, 0.5f, "%.1f");
        if (interval_val < 0.1f) interval_val = 0.1f;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            SDL_LockMutex(app->mutex);
            t->interval = interval_val;
            SDL_UnlockMutex(app->mutex);
            app->config_dirty = 1;
        }
        ImGui::SameLine(0, gap);

        int is_running;
        SDL_LockMutex(app->mutex);
        is_running = t->running;
        SDL_UnlockMutex(app->mutex);
        if (is_running) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 50, 50, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(220, 70, 70, 255));
            if (ImGui::Button("Stop", ImVec2(btn_w, 0))) stop_target(app, i);
            ImGui::PopStyleColor(2);
        } else {
            if (ImGui::Button("Go", ImVec2(btn_w, 0))) start_or_resolve_target(app, i);
        }
        ImGui::SameLine(0, gap);

        if (ImGui::Button("Reset", ImVec2(btn_w, 0))) {
            SDL_LockMutex(app->mutex);

            int ok = chart_remove_points_for_target(&app->chart, i);
            if (ok) {
                t->raw_count = 0;
                t->valid_count = 0; t->has_stats = 0; t->lost_count = 0;
                t->stats_avg = 0; t->stats_min = 0; t->stats_max = 0;
            }

            SDL_UnlockMutex(app->mutex);
        }
        ImGui::SameLine(0, gap);

        if (app->target_count > 1) {
            if (ImGui::Button("X", ImVec2(24.0f, 0))) app_remove_target(app, i);
        }

        ImGui::PopID();
    }

    if (app->target_count < MAX_TARGETS) {
        if (ImGui::Button("+")) app_add_target(app);
    }

    ImGui::Separator();

    float chart_h = ImGui::GetWindowHeight() - ImGui::GetCursorPosY() - 20.0f;
    if (chart_h < 80) chart_h = 80;

    const char* target_ips[MAX_TARGETS];
    TargetStats target_stats[MAX_TARGETS];
    SDL_LockMutex(app->mutex);
    for (int t = 0; t < app->target_count; t++) {
        target_ips[t] = app->targets[t].ip;
        target_stats[t].avg = app->targets[t].stats_avg;
        target_stats[t].min = app->targets[t].stats_min;
        target_stats[t].max = app->targets[t].stats_max;
        target_stats[t].lost = app->targets[t].lost_count;
        target_stats[t].total = app->targets[t].valid_count + app->targets[t].lost_count;
        target_stats[t].active = app->targets[t].has_stats;
    }
    ImVec2 chart_size = ImVec2(ImGui::GetContentRegionAvail().x, chart_h);
    chart_render(&app->chart, app->target_count, target_ips, target_stats, chart_size, app->dark_mode, app->use_mean);
    SDL_UnlockMutex(app->mutex);

    ImGui::End();

    if (app->config_dirty) {
        AppConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.target_count = app->target_count;
        for (int i = 0; i < app->target_count; i++) {
            strncpy(cfg.targets[i].ip, app->targets[i].ip, 127);
            cfg.targets[i].ip[127] = '\0';
            cfg.targets[i].interval = app->targets[i].interval;
            cfg.targets[i].use_icmp = app->targets[i].use_icmp;
        }
        cfg.max_points = app->max_points;
        cfg.rolling_avg = app->rolling_avg;
        cfg.dark_mode = app->dark_mode;
        cfg.use_mean = app->use_mean;
        config_save(&cfg);
        app->config_dirty = 0;
    }
}
