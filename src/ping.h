#ifndef PING_H
#define PING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double avg_ms;
    int timeout;
    int count;
} PingResult;

int do_ping(const char* ip, double* ms_out);

#ifdef __cplusplus
}
#endif

#endif /* PING_H */
