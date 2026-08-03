#ifndef PING_H
#define PING_H

int do_ping_icmp(const char* ip, double* ms_out);
int do_ping_legacy(const char* ip, double* ms_out);

#endif /* PING_H */
