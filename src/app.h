#ifndef APP_H
#define APP_H

#include "chart.h"
#include "ping.h"
#include "config.h"
#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RAW_PINGS 512
#define MAX_STATS_ENTRIES 100000

typedef struct {
    char ip[128];
    float interval;
    int running;
    int pending_stop;

    double raw_pings[MAX_RAW_PINGS];
    int raw_count;

    double all_valid_pings[MAX_STATS_ENTRIES];
    int valid_count;

    double stats_avg;
    double stats_min;
    double stats_max;
    int has_stats;
    int lost_count;

    SDL_Thread* thread;
    char timestamp_buffers[MAX_POINTS][16];
} PingTarget;

typedef struct {
    SDL_Window* window;

    PingTarget targets[MAX_TARGETS];
    int target_count;
    int max_points;
    int rolling_avg;
    int dark_mode;
    int use_mean;

    ChartData chart;

    SDL_mutex* mutex;
    int config_dirty;
} App;

void app_init(App* app, const AppConfig* cfg);
void app_shutdown(App* app);
void app_render(App* app);
void app_toggle_theme(App* app);
void app_add_target(App* app);
void app_remove_target(App* app, int idx);

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
