#include "ping.h"
#include <windows.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static IPAddr parse_ip(const char* ip) {
    unsigned int a, b, c, d;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    return (IPAddr)((a << 24) | (b << 16) | (c << 8) | d);
}

int do_ping_icmp(const char* ip, double* ms_out) {
    IPAddr ip_addr = parse_ip(ip);
    if (ip_addr == 0) return -1;

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return -1;

    union {
        ICMP_ECHO_REPLY reply;
        char raw[sizeof(ICMP_ECHO_REPLY) + 8];
    } buf;
    memset(&buf, 0, sizeof(buf));

    DWORD dwRetVal = IcmpSendEcho(hIcmp, ip_addr, NULL, 0, NULL,
                                   &buf, sizeof(buf), 3000);

    IcmpCloseHandle(hIcmp);

    if (dwRetVal == 0) return -1;
    if (buf.reply.Status != IP_SUCCESS) return -1;

    *ms_out = (double)buf.reply.RoundTripTime;
    return 0;
}

int do_ping_legacy(const char* ip, double* ms_out) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ping -n 1 -w 3000 %s", ip);

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return -1;

    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return -1;
    }

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return -1;
    }

    CloseHandle(hWrite);

    char buf[4096];
    memset(buf, 0, sizeof(buf));
    DWORD totalRead = 0;
    while (totalRead < sizeof(buf) - 1) {
        DWORD bytesRead;
        if (!ReadFile(hRead, buf + totalRead, sizeof(buf) - 1 - totalRead, &bytesRead, NULL))
            break;
        if (bytesRead == 0) break;
        totalRead += bytesRead;
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (strstr(buf, "Richiesta scaduta") || strstr(buf, "Request timed out"))
        return -1;

    char* t = strstr(buf, "time=");
    if (!t) t = strstr(buf, "time<");
    if (!t) t = strstr(buf, "durata=");
    if (!t) t = strstr(buf, "durata<");
    if (!t) return -1;

    char* sep = strchr(t, '=');
    if (!sep) sep = strchr(t, '<');
    if (!sep) return -1;
    sep++;
    while (*sep == ' ') sep++;
    double val = atof(sep);
    if (val > 0) {
        *ms_out = val;
        return 0;
    }
    return -1;
}
