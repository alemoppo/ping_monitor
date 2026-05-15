#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char* trim(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' '))
        s[--len] = '\0';
    return s;
}

void config_load(AppConfig* cfg) {
    memset(cfg, 0, sizeof(AppConfig));
    cfg->target_count = 1;
    cfg->max_points = 30;
    cfg->rolling_avg = 4;
    cfg->dark_mode = 1;
    cfg->use_mean = 0;
    cfg->targets[0].interval = 0.5f;

    FILE* f = fopen(CONFIG_FILE, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char* key = trim(line);
        char* val = trim(eq + 1);

        if (strcmp(key, "target_count") == 0) {
            int n = atoi(val);
            if (n >= 1 && n <= MAX_TARGETS) cfg->target_count = n;
        } else if (strncmp(key, "ip", 2) == 0) {
            int idx = atoi(key + 2);
            if (idx >= 0 && idx < MAX_TARGETS) {
                strncpy(cfg->targets[idx].ip, val, 127);
                cfg->targets[idx].ip[127] = '\0';
            }
        } else if (strncmp(key, "interval", 8) == 0) {
            int idx = atoi(key + 8);
            if (idx >= 0 && idx < MAX_TARGETS) {
                cfg->targets[idx].interval = (float)atof(val);
                if (cfg->targets[idx].interval <= 0.0f) cfg->targets[idx].interval = 0.5f;
            }
        } else if (strcmp(key, "max_points") == 0) {
            cfg->max_points = atoi(val);
            if (cfg->max_points < 5) cfg->max_points = 30;
            if (cfg->max_points > 512) cfg->max_points = 512;
        } else if (strcmp(key, "rolling_avg") == 0) {
            cfg->rolling_avg = atoi(val);
            if (cfg->rolling_avg < 1) cfg->rolling_avg = 4;
        } else if (strcmp(key, "dark_mode") == 0) {
            cfg->dark_mode = atoi(val);
            if (cfg->dark_mode != 0 && cfg->dark_mode != 1) cfg->dark_mode = 1;
        } else if (strcmp(key, "use_mean") == 0) {
            cfg->use_mean = atoi(val) ? 1 : 0;
        }
    }
    fclose(f);
}

void config_save(const AppConfig* cfg) {
    FILE* f = fopen(CONFIG_FILE, "w");
    if (!f) return;

    fprintf(f, "target_count=%d\n", cfg->target_count);
    for (int i = 0; i < cfg->target_count; i++) {
        fprintf(f, "ip%d=%s\n", i, cfg->targets[i].ip);
        fprintf(f, "interval%d=%.1f\n", i, cfg->targets[i].interval);
    }
    fprintf(f, "max_points=%d\n", cfg->max_points);
    fprintf(f, "rolling_avg=%d\n", cfg->rolling_avg);
    fprintf(f, "dark_mode=%d\n", cfg->dark_mode);
    fprintf(f, "use_mean=%d\n", cfg->use_mean);
    fclose(f);
}
