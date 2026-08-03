#ifndef PROC_H
#define PROC_H

#ifdef __cplusplus
extern "C" {
#endif

#define PROC_MAX_IPS 8
#define PROC_IP_LEN 128

int is_valid_ip(const char* s);
int resolve_process_ips(const char* process_name, char ips[PROC_MAX_IPS][PROC_IP_LEN], int* count);

#ifdef __cplusplus
}
#endif

#endif /* PROC_H */
