#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_FILE "ping_monitor.ini"
#define MAX_TARGETS 4

typedef struct {
    char ip[128];
    float interval;
} TargetConfig;

typedef struct {
    TargetConfig targets[MAX_TARGETS];
    int target_count;
    int max_points;
    int rolling_avg;
    int dark_mode;
    int use_mean;
} AppConfig;

void config_load(AppConfig* cfg);
void config_save(const AppConfig* cfg);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
