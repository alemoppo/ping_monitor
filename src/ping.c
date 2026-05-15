#include "ping.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int do_ping(const char* ip, double* ms_out) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ping -n 1 -w 3000 %s", ip);

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return -1;

    if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return -1;
    }

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return -1;
    }

    CloseHandle(hWrite);

    char buf[4096] = {0};
    DWORD totalRead = 0;
    while (totalRead < sizeof(buf) - 1) {
        DWORD bytesRead;
        if (!ReadFile(hRead, buf + totalRead, sizeof(buf) - 1 - totalRead, &bytesRead, NULL))
            break;
        totalRead += bytesRead;
        if (bytesRead == 0) break;
    }
    buf[totalRead] = '\0';

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (strstr(buf, "Request timed out") || strstr(buf, "Richiesta scaduta")) {
        return -1;
    }

    char* match = strstr(buf, "time");
    if (!match) {
        match = strstr(buf, "durata");
    }
    if (!match) return -1;

    char* eq = strchr(match, '=');
    if (!eq) return -1;
    eq++;
    while (*eq == ' ') eq++;

    double val = atof(eq);
    if (val > 0.0) {
        *ms_out = val;
        return 0;
    }

    return -1;
}
