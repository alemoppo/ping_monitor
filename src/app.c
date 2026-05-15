#include "app.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

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
        int rolling_avg = app->rolling_avg;
        SDL_UnlockMutex(app->mutex);

        double ms = 0;
        int ret = do_ping(ip, &ms);

        SDL_LockMutex(app->mutex);
        if (t->pending_stop) {
            SDL_UnlockMutex(app->mutex);
            return 0;
        }

        if (t->raw_count < MAX_RAW_PINGS) {
            t->raw_pings[t->raw_count++] = (ret == 0) ? ms : -1.0;
        }

        if (ret == 0 && t->valid_count < MAX_STATS_ENTRIES) {
            t->all_valid_pings[t->valid_count++] = ms;
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
                if (v > 0) { sum += v; count++; }
            }

            double avg = (count > 0) ? sum / count : -1.0;

            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            int ts_idx = app->chart.count % MAX_POINTS;
            strftime(t->timestamp_buffers[ts_idx], 16, "%H:%M:%S", tm_info);

            chart_add_point(&app->chart, idx, avg, raw_buf, raw_buf_count, t->timestamp_buffers[ts_idx]);

            t->raw_count -= rolling_avg;
            memmove(t->raw_pings, t->raw_pings + rolling_avg, t->raw_count * sizeof(double));

            if (t->valid_count > 0) {
                double s_min = t->all_valid_pings[0], s_max = t->all_valid_pings[0], s_sum = 0;
                for (int i = 0; i < t->valid_count; i++) {
                    double v = t->all_valid_pings[i];
                    s_sum += v;
                    if (v < s_min) s_min = v;
                    if (v > s_max) s_max = v;
                }
                t->stats_avg = s_sum / t->valid_count;
                t->stats_min = s_min;
                t->stats_max = s_max;
                t->has_stats = 1;
            }
        }
        SDL_UnlockMutex(app->mutex);

        SDL_Delay((Uint32)(interval * 1000.0f));
    }
    return 0;
}

static void start_target(App* app, int idx) {
    PingTarget* t = &app->targets[idx];
    if (t->running) return;

    int points_before = 0;
    for (int i = 0; i < app->chart.count; i++) {
        if (app->chart.points[i].target_index == idx) points_before++;
    }

    t->pending_stop = 0;
    t->raw_count = 0;
    t->valid_count = 0;
    t->has_stats = 0;
    t->lost_count = 0;
    t->stats_avg = 0; t->stats_min = 0; t->stats_max = 0;
    t->running = 1;

    if (points_before > 0) {
        int write = 0;
        for (int i = 0; i < app->chart.count; i++) {
            if (app->chart.points[i].target_index != idx) {
                if (write != i) app->chart.points[write] = app->chart.points[i];
                write++;
            }
        }
        app->chart.count = write;
    }

    ThreadCtx* ctx = (ThreadCtx*)SDL_malloc(sizeof(ThreadCtx));
    ctx->app = app;
    ctx->index = idx;
    t->thread = SDL_CreateThread(ping_thread_func, "ping_thread", ctx);
}

static void stop_target(App* app, int idx) {
    PingTarget* t = &app->targets[idx];
    if (!t->running) return;
    SDL_LockMutex(app->mutex);
    t->pending_stop = 1;
    SDL_UnlockMutex(app->mutex);
    SDL_WaitThread(t->thread, NULL);
    t->thread = NULL;
    t->running = 0;
}

void app_init(App* app, const AppConfig* cfg) {
    memset(app, 0, sizeof(App));
    app->target_count = cfg->target_count;
    app->max_points = cfg->max_points;
    app->rolling_avg = cfg->rolling_avg;
    app->dark_mode = cfg->dark_mode;
    app->use_mean = cfg->use_mean;
    app->config_dirty = 0;
    app->mutex = SDL_CreateMutex();

    chart_init(&app->chart, app->max_points);

    for (int i = 0; i < app->target_count; i++) {
        PingTarget* t = &app->targets[i];
        strncpy(t->ip, cfg->targets[i].ip[0] ? cfg->targets[i].ip : (i == 0 ? "8.8.8.8" : "1.1.1.1"), 127);
        t->ip[127] = '\0';
        t->interval = cfg->targets[i].interval > 0 ? cfg->targets[i].interval : 0.5f;
    }
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
    app->target_count++;
    app->config_dirty = 1;
}

void app_remove_target(App* app, int idx) {
    if (app->target_count <= 1 || idx < 0 || idx >= app->target_count) return;

    if (app->targets[idx].running) {
        stop_target(app, idx);
    }

    int points_before = 0;
    for (int i = 0; i < app->chart.count; i++) {
        if (app->chart.points[i].target_index == idx) points_before++;
    }

    for (int i = idx; i < app->target_count - 1; i++) {
        memcpy(&app->targets[i], &app->targets[i + 1], sizeof(PingTarget));
    }
    memset(&app->targets[app->target_count - 1], 0, sizeof(PingTarget));
    app->target_count--;

    if (points_before > 0) {
        int write = 0;
        for (int i = 0; i < app->chart.count; i++) {
            int ti = app->chart.points[i].target_index;
            if (ti == idx) continue;
            if (ti > idx) app->chart.points[i].target_index--;
            if (write != i) app->chart.points[write] = app->chart.points[i];
            write++;
        }
        app->chart.count = write;
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
    if (ImGui::InputInt("##rolling_avg", &app->rolling_avg, 0, 0)) {
        if (app->rolling_avg < 1) app->rolling_avg = 1;
        app->config_dirty = 1;
    }

    ImGui::SameLine(0, gap);
    ImGui::Text("Max campioni:");
    ImGui::SameLine(0, gap);
    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::InputInt("##max_points", &app->max_points, 0, 0)) {
        if (app->max_points < 5) app->max_points = 5;
        if (app->max_points > 512) app->max_points = 512;
        app->chart.max_points = app->max_points;
        app->config_dirty = 1;
    }

    ImGui::SameLine(0, gap);
    ImGui::Checkbox("Media##use_mean", (bool*)&app->use_mean);
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
        ImGui::InputText(id_buf, t->ip, sizeof(t->ip), 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) app->config_dirty = 1;
        ImGui::SameLine(0, gap);

        snprintf(id_buf, sizeof(id_buf), "##int%d", i);
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputFloat(id_buf, &t->interval, 0.1f, 0.5f, "%.1f");
        if (t->interval < 0.1f) t->interval = 0.1f;
        if (ImGui::IsItemDeactivatedAfterEdit()) app->config_dirty = 1;
        ImGui::SameLine(0, gap);

        if (t->running) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 50, 50, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(220, 70, 70, 255));
            if (ImGui::Button("Stop", ImVec2(btn_w, 0))) stop_target(app, i);
            ImGui::PopStyleColor(2);
        } else {
            if (ImGui::Button("Go", ImVec2(btn_w, 0))) start_target(app, i);
        }
        ImGui::SameLine(0, gap);

        if (ImGui::Button("Reset", ImVec2(btn_w, 0))) {
            SDL_LockMutex(app->mutex);

            int write = 0;
            for (int j = 0; j < app->chart.count; j++) {
                if (app->chart.points[j].target_index != i) {
                    if (write != j) app->chart.points[write] = app->chart.points[j];
                    write++;
                }
            }
            app->chart.count = write;

            t->raw_count = 0;
            t->valid_count = 0; t->has_stats = 0; t->lost_count = 0;
            t->stats_avg = 0; t->stats_min = 0; t->stats_max = 0;

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

    ImGui::End();

    if (app->config_dirty) {
        AppConfig cfg;
        cfg.target_count = app->target_count;
        for (int i = 0; i < app->target_count; i++) {
            strncpy(cfg.targets[i].ip, app->targets[i].ip, 127);
            cfg.targets[i].ip[127] = '\0';
            cfg.targets[i].interval = app->targets[i].interval;
        }
        cfg.max_points = app->max_points;
        cfg.rolling_avg = app->rolling_avg;
        cfg.dark_mode = app->dark_mode;
        config_save(&cfg);
        app->config_dirty = 0;
    }
}
