#ifndef CHART_H
#define CHART_H

#include "imgui.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Upper bound for the number of raw samples used to build one chart point
   (must match MAX_RAW_PINGS). */
#define MAX_POINTS 512

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
    int round;
    double avg_ping;
    char timestamp[16];

    /* Precomputed aggregates (updated once per inserted point, not per frame) */
    double median_ping;
    double q1_ping;
    double q3_ping;
    double valid_min;
    double valid_max;
    double sum_ping;
    int valid_count;
    int timeout_count;
    int has_timeout;
} ChartPoint;

typedef struct {
    ChartPoint* points;
    int count;
    int max_points;
    int head;
    int next_round;
} ChartData;

int chart_init(ChartData* chart, int max_points);
void chart_shutdown(ChartData* chart);
int chart_resize(ChartData* chart, int new_max_points);
int chart_add_point(ChartData* chart, int target_index, double avg_ping,
                    const double* raw_values, int raw_count, const char* timestamp);
void chart_clear(ChartData* chart);
/* Remove all points belonging to a target. Caller must hold the app mutex.
   Returns 1 on success, 0 if memory allocation failed (caller should abort). */
int chart_remove_points_for_target(ChartData* chart, int target_index);
/* Remove a target's points and shift higher target indices down by one.
   Returns 1 on success, 0 if memory allocation failed. */
int chart_remove_target_and_shift(ChartData* chart, int target_index);
void chart_render(const ChartData* chart, int target_count, const char* const* target_ips,
                  const TargetStats* stats, ImVec2 size, int dark, int use_mean);

#ifdef __cplusplus
}
#endif

#endif /* CHART_H */
