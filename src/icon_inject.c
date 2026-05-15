#include <windows.h>
#include <stdio.h>

static BOOL AddIconToExe(const char* exePath, const char* iconPath) {
    FILE* f = fopen(iconPath, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", iconPath); return FALSE; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    HANDLE hRes = BeginUpdateResourceA(exePath, FALSE);
    if (!hRes) { fprintf(stderr, "BeginUpdateResource failed: %lu\n", GetLastError()); free(data); return FALSE; }

    if (!UpdateResourceA(hRes, RT_ICON, MAKEINTRESOURCE(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), data, size)) {
        fprintf(stderr, "UpdateResource failed: %lu\n", GetLastError());
        EndUpdateResourceA(hRes, TRUE);
        free(data);
        return FALSE;
    }

    EndUpdateResourceA(hRes, FALSE);
    free(data);
    return TRUE;
}

int main(int argc, char* argv[]) {
    if (argc < 3) { fprintf(stderr, "Usage: icon_inject <exe> <ico>\n"); return 1; }
    return AddIconToExe(argv[1], argv[2]) ? 0 : 1;
}
