#ifndef CHART_H
#define CHART_H

#include "imgui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HISTORY_VALUES 64
#define MAX_POINTS 2048

typedef struct {
    double avg;
    double min;
    double max;
    int lost;
    int total;
    int active;
} TargetStats;

typedef struct {
    int target_index;
    double avg_ping;
    double history_raw[MAX_HISTORY_VALUES];
    int history_count;
    char timestamp[16];
} ChartPoint;

typedef struct {
    ChartPoint points[MAX_POINTS];
    int count;
    int max_points;
} ChartData;

void chart_init(ChartData* chart, int max_points);
int chart_add_point(ChartData* chart, int target_index, double avg_ping,
                    const double* raw_values, int raw_count, const char* timestamp);
void chart_clear(ChartData* chart);
void chart_render(const ChartData* chart, int target_count, const char* const* target_ips,
                  const TargetStats* stats, ImVec2 size, int dark, int use_mean);

#ifdef __cplusplus
}
#endif

#endif /* CHART_H */
