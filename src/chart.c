#include "chart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TARGETS 4

static const ImU32 TRACE_COLORS_DARK[] = {
    IM_COL32(80, 200, 80, 255),
    IM_COL32(80, 150, 240, 255),
    IM_COL32(240, 170, 50, 255),
    IM_COL32(200, 100, 240, 255),
};

static const ImU32 TRACE_COLORS_LIGHT[] = {
    IM_COL32(30, 160, 30, 255),
    IM_COL32(30, 100, 200, 255),
    IM_COL32(200, 130, 20, 255),
    IM_COL32(160, 50, 200, 255),
};

static ImU32 trace_color(int dark, int tidx) {
    if (dark) return TRACE_COLORS_DARK[tidx % 4];
    return TRACE_COLORS_LIGHT[tidx % 4];
}

static void draw_x(ImDrawList* dl, float x, float y, float size, ImU32 col, float thickness) {
    float hs = size * 0.5f;
    dl->AddLine(ImVec2(x - hs, y - hs), ImVec2(x + hs, y + hs), col, thickness);
    dl->AddLine(ImVec2(x + hs, y - hs), ImVec2(x - hs, y + hs), col, thickness);
}

static int cmp_double(const void* a, const void* b) {
    double da = *(const double*)a, db = *(const double*)b;
    return (da > db) - (da < db);
}

static ImU32 ping_color(double val, ImU32 trace_col) {
    if (val <= 100.0) return trace_col;
    if (val < 300.0) return IM_COL32(230, 150, 0, 255);
    return IM_COL32(220, 0, 0, 255);
}

void chart_init(ChartData* chart, int max_points) {
    memset(chart, 0, sizeof(ChartData));
    chart->max_points = max_points;
    chart->count = 0;
}

int chart_add_point(ChartData* chart, int target_index, double avg_ping,
                    const double* raw_values, int raw_count, const char* timestamp) {
    if (chart->count >= chart->max_points) {
        memmove(&chart->points[0], &chart->points[1],
                sizeof(ChartPoint) * (chart->count - 1));
        chart->count--;
    }
    ChartPoint* p = &chart->points[chart->count];
    p->target_index = target_index;
    p->avg_ping = avg_ping;
    p->history_count = raw_count < MAX_HISTORY_VALUES ? raw_count : MAX_HISTORY_VALUES;
    memcpy(p->history_raw, raw_values, sizeof(double) * p->history_count);
    if (timestamp) {
        strncpy(p->timestamp, timestamp, sizeof(p->timestamp) - 1);
        p->timestamp[sizeof(p->timestamp) - 1] = '\0';
    } else {
        p->timestamp[0] = '\0';
    }
    chart->count++;
    return 1;
}

void chart_clear(ChartData* chart) {
    chart->count = 0;
}

void chart_render(const ChartData* chart, int target_count, const char* const* target_ips,
                  const TargetStats* stats, ImVec2 size, int dark, int use_mean) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 canvas_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

    const float margin_left = 55.0f;
    const float margin_right = 15.0f;
    const float margin_top = 40.0f;
    const float margin_bottom = 80.0f;

    ImVec2 plot_min = ImVec2(p_min.x + margin_left, p_min.y + margin_top);
    ImVec2 plot_max = ImVec2(canvas_max.x - margin_right, canvas_max.y - margin_bottom);

    ImU32 plot_bg = dark ? IM_COL32(20, 20, 30, 255) : IM_COL32(245, 245, 250, 255);
    ImU32 plot_border = dark ? IM_COL32(60, 60, 80, 255) : IM_COL32(160, 160, 170, 255);
    ImU32 grid_line = dark ? IM_COL32(50, 50, 65, 120) : IM_COL32(190, 190, 200, 120);
    ImU32 text_color = dark ? IM_COL32(180, 180, 180, 200) : IM_COL32(80, 80, 80, 200);

    dl->AddRectFilled(plot_min, plot_max, plot_bg);
    dl->AddRect(plot_min, plot_max, plot_border);

    typedef struct {
        double sum;
        double min;
        double max;
        int lost;
        int count;
    } VisibleStats;
    VisibleStats vis[MAX_TARGETS] = {{0}};

    for (int i = 0; i < chart->count; i++) {
        int t = chart->points[i].target_index;
        if (t >= target_count) continue;
        const ChartPoint* pt = &chart->points[i];
        VisibleStats* v = &vis[t];
        for (int h = 0; h < pt->history_count; h++) {
            double val = pt->history_raw[h];
            if (val < 0) {
                v->lost++;
            } else {
                if (v->count == 0 || val < v->min) v->min = val;
                if (v->count == 0 || val > v->max) v->max = val;
                v->sum += val;
                v->count++;
            }
        }
    }
    float legend_y = p_min.y + 10.0f;
    float legend_x = p_min.x + margin_left;
    float legend_item_gap = 30.0f;
    ImU32 legend_text = dark ? IM_COL32(220, 220, 220, 255) : IM_COL32(30, 30, 30, 255);
    ImU32 legend_dim = dark ? IM_COL32(140, 140, 150, 200) : IM_COL32(100, 100, 110, 200);
    float cx = legend_x;
    for (int t = 0; t < target_count; t++) {
        const char* ip = target_ips[t];
        if (!ip || ip[0] == '\0') continue;
        dl->AddRectFilled(ImVec2(cx, legend_y), ImVec2(cx + 12.0f, legend_y + 10.0f), trace_color(dark, t));
        cx += 16.0f;
        ImVec2 tsize = ImGui::CalcTextSize(ip);
        dl->AddText(ImVec2(cx, legend_y), legend_text, ip);
        cx += tsize.x + 6.0f;
        if (vis[t].count > 0 || vis[t].lost > 0) {
            char buf[80];
            double avg = vis[t].count > 0 ? vis[t].sum / vis[t].count : 0;
            snprintf(buf, sizeof(buf), "%.0f %.0f %.0f; %d", vis[t].max, avg, vis[t].min, vis[t].lost);
            ImVec2 ssize = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(cx, legend_y), legend_dim, buf);
            char id[32];
            snprintf(id, sizeof(id), "##v%d", t);
            ImGui::SetCursorScreenPos(ImVec2(cx, legend_y));
            ImGui::InvisibleButton(id, ssize);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Massimo Media Minimo; Persi");
            cx += ssize.x;
        }
        cx += legend_item_gap;
    }

    float plot_w = plot_max.x - plot_min.x;
    float plot_h = plot_max.y - plot_min.y;

    int total_points[MAX_TARGETS] = {0};
    for (int i = 0; i < chart->count; i++) {
        int t = chart->points[i].target_index;
        if (t < MAX_TARGETS) total_points[t]++;
    }

    double global_max = 10.0;
    for (int i = 0; i < chart->count; i++) {
        const ChartPoint* pt = &chart->points[i];
        for (int j = 0; j < pt->history_count; j++) {
            double v = pt->history_raw[j];
            if (v > 0 && v > global_max) global_max = v * 1.1;
        }
    }
    if (global_max < 10.0) global_max = 10.0;

    auto val_to_y = [&](double v) -> float {
        if (v <= 0) v = 0;
        return plot_max.y - (float)((v / global_max) * plot_h);
    };

    for (int i = 0; i <= 5; i++) {
        double val = (global_max / 5.0) * i;
        float y = plot_max.y - (float)((val / global_max) * plot_h);
        dl->AddLine(ImVec2(plot_min.x, y), ImVec2(plot_max.x, y), grid_line, 1.0f);
        char label[32];
        snprintf(label, sizeof(label), "%.0f", val);
        ImVec2 tsize = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(plot_min.x - tsize.x - 8, y - tsize.y * 0.5f), text_color, label);
    }

    dl->PushClipRect(plot_min, plot_max, true);

    int total_rounds = 0;
    for (int t = 0; t < target_count; t++) {
        if (total_points[t] > total_rounds) total_rounds = total_points[t];
    }

    for (int t = 0; t < target_count; t++) {
        if (total_points[t] == 0) continue;

        ImU32 col = trace_color(dark, t);
        ImU32 timeout_col = dark ? IM_COL32(220, 50, 50, 255) : IM_COL32(200, 40, 40, 255);
        ImU32 timeout_seg_col = IM_COL32(140, 20, 20, 255);
        ImU32 box_fill = (col & 0x00FFFFFF) | 0x80000000;
        ImU32 whisker_line = (col & 0x00FFFFFF) | 0x90000000;
        ImU32 median_line = dark ? IM_COL32(255, 255, 255, 180) : IM_COL32(0, 0, 0, 180);

        int tp = total_points[t];
        float x_step = plot_w / (float)(total_rounds > 1 ? total_rounds - 1 : 1);
        int point_idx = 0;
        double prev_use_val = -1;
        float prev_x = 0;

        for (int i = 0; i < chart->count; i++) {
            if (chart->points[i].target_index != t) continue;
            const ChartPoint* pt = &chart->points[i];
            float x = plot_min.x + point_idx * x_step;

            int has_timeout = 0;
            double valid_vals[MAX_HISTORY_VALUES] = {0};
            int valid_count = 0;
            for (int h = 0; h < pt->history_count; h++) {
                double v = pt->history_raw[h];
                if (v < 0) { has_timeout = 1; }
                else { valid_vals[valid_count++] = v; }
            }

            double use_val = -1;
            if (valid_count == 1) {
                use_val = valid_vals[0];
            } else if (valid_count >= 2) {
                qsort(valid_vals, valid_count, sizeof(double), cmp_double);
                double med = valid_count % 2 == 1
                    ? valid_vals[valid_count / 2]
                    : (valid_vals[valid_count / 2 - 1] + valid_vals[valid_count / 2]) * 0.5;
                use_val = (use_mean && pt->avg_ping > 0) ? pt->avg_ping : med;
            }

            if (point_idx > 0 && prev_use_val > 0 && use_val > 0) {
                ImU32 line_col = has_timeout ? timeout_seg_col : ping_color(use_val, col);
                dl->AddLine(ImVec2(prev_x, val_to_y(prev_use_val)), ImVec2(x, val_to_y(use_val)), line_col, 2.0f);
            }
            prev_use_val = use_val;
            prev_x = x;

            float box_w = 0.25f * x_step;
            if (box_w > plot_w * 0.3f) box_w = plot_w * 0.3f;
            if (box_w < 3.0f) box_w = 3.0f;

            if (valid_count == 0 && has_timeout) {
                draw_x(dl, x, val_to_y(0), 12.0f, timeout_col, 2.0f);
            } else if (valid_count == 1) {
                float vy = val_to_y(valid_vals[0]);
                float half_w = box_w * 0.5f;
                dl->AddRectFilled(ImVec2(x - half_w, vy - 2.5f), ImVec2(x + half_w, vy + 2.5f), box_fill);
                if (has_timeout) {
                    draw_x(dl, x, vy, 12.0f, timeout_col, 2.0f);
                } else {
                    dl->AddCircleFilled(ImVec2(x, vy), 6.0f, ping_color(valid_vals[0], col));
                }
            } else if (valid_count >= 2) {
                double mn = valid_vals[0], mx = valid_vals[0];
                for (int v = 0; v < valid_count; v++) {
                    if (valid_vals[v] < mn) mn = valid_vals[v];
                    if (valid_vals[v] > mx) mx = valid_vals[v];
                }
                double median = valid_count % 2 == 1
                    ? valid_vals[valid_count / 2]
                    : (valid_vals[valid_count / 2 - 1] + valid_vals[valid_count / 2]) * 0.5;

                double q1_idx = valid_count * 0.25;
                int q1_low = (int)q1_idx;
                double q1_frac = q1_idx - q1_low;
                if (q1_low >= valid_count - 1) q1_low = valid_count - 2;
                double q1 = valid_vals[q1_low] * (1.0 - q1_frac) + valid_vals[q1_low + 1] * q1_frac;

                double q3_idx = valid_count * 0.75;
                int q3_low = (int)q3_idx;
                double q3_frac = q3_idx - q3_low;
                if (q3_low >= valid_count - 1) q3_low = valid_count - 2;
                double q3 = valid_vals[q3_low] * (1.0 - q3_frac) + valid_vals[q3_low + 1] * q3_frac;

                float y_q1 = val_to_y(q1), y_q3 = val_to_y(q3);
                float y_med = val_to_y(median), y_min = val_to_y(mn), y_max = val_to_y(mx);
                float y_dot = val_to_y(use_val);

                dl->AddRectFilled(ImVec2(x - box_w * 0.5f, y_q3), ImVec2(x + box_w * 0.5f, y_q1), box_fill);
                dl->AddLine(ImVec2(x - box_w * 0.5f, y_med), ImVec2(x + box_w * 0.5f, y_med), median_line, 2.0f);
                dl->AddLine(ImVec2(x, y_min), ImVec2(x, y_max), whisker_line, 1.5f);
                dl->AddLine(ImVec2(x - box_w * 0.5f, y_min), ImVec2(x + box_w * 0.5f, y_min), whisker_line, 1.5f);
                dl->AddLine(ImVec2(x - box_w * 0.5f, y_max), ImVec2(x + box_w * 0.5f, y_max), whisker_line, 1.5f);
                if (has_timeout) {
                    draw_x(dl, x, y_dot, 12.0f, timeout_col, 2.0f);
                } else {
                    dl->AddCircleFilled(ImVec2(x, y_dot), 6.0f, ping_color(use_val, col));
                }
            }

            point_idx++;
        }
    }

    dl->PopClipRect();

    int first_target = -1;
    for (int t = 0; t < target_count; t++) {
        if (total_points[t] == total_rounds) { first_target = t; break; }
    }

    if (total_rounds > 0 && first_target >= 0) {
        int label_count = (int)(plot_w / 100.0f);
        if (label_count < 2) label_count = 2;
        int round_step = total_rounds / label_count;
        if (round_step < 1) round_step = 1;

        float round_step_px = plot_w / (float)(total_rounds > 1 ? total_rounds - 1 : 1);

        int ri = 0;
        for (int i = 0; i < chart->count && ri < total_rounds; i++) {
            if (chart->points[i].target_index != first_target) continue;
            if (ri % round_step == 0 && chart->points[i].timestamp[0]) {
                float px = plot_min.x + ri * round_step_px;
                ImVec2 tsize = ImGui::CalcTextSize(chart->points[i].timestamp);
                dl->AddText(ImVec2(px - tsize.x * 0.5f, plot_max.y + 5.0f), text_color, chart->points[i].timestamp);
            }
            ri++;
        }
    }

    if (chart->count == 0) {
        dl->AddText(ImVec2(plot_min.x + plot_w * 0.5f, plot_min.y + plot_h * 0.5f - 8),
                    text_color, "In attesa di dati...");
    }

    float footer_y = plot_max.y + 25.0f;
    ImU32 footer_label = dark ? IM_COL32(200, 200, 200, 255) : IM_COL32(50, 50, 50, 255);
    float fx = legend_x;
    for (int t = 0; t < target_count; t++) {
        const char* ip = target_ips[t];
        if (!ip || ip[0] == '\0' || !stats || !stats[t].active) continue;
        dl->AddText(ImVec2(fx, footer_y), trace_color(dark, t), ip);
        ImVec2 ip_sz = ImGui::CalcTextSize(ip);
        fx += ip_sz.x + 8.0f;
        char buf[96];
        double pct = stats[t].total > 0 ? (double)stats[t].lost / stats[t].total * 100.0 : 0.0;
        snprintf(buf, sizeof(buf), "%.0f %.0f %.0f; %d/%d  %.2f%%",
                 stats[t].max, stats[t].avg, stats[t].min,
                 stats[t].lost, stats[t].total, pct);
        ImVec2 buf_sz = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(fx, footer_y), footer_label, buf);
        char id[32];
        snprintf(id, sizeof(id), "##g%d", t);
        ImGui::SetCursorScreenPos(ImVec2(fx, footer_y));
        ImGui::InvisibleButton(id, buf_sz);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Massimo Media Minimo; Persi/Totali  Percentuale");
        fx += buf_sz.x + 30.0f;
    }
}
