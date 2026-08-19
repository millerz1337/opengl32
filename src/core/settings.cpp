#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include "core/settings.h"
#include "core/globals.h"

static void GetConfigPath(char* out, size_t sz) {
    if (GetEnvironmentVariableA("LOCALAPPDATA", out, (DWORD)sz) == 0) {
        out[0] = '\0';
        return;
    }
    size_t used = strlen(out);
    if (used + 16 >= sz) { out[0] = '\0'; return; }
    strcat_s(out, sz, "\\Leet\\config.ini");
}

static void EnsureConfigDir() {
    char lp[MAX_PATH];
    if (GetEnvironmentVariableA("LOCALAPPDATA", lp, MAX_PATH) == 0) return;
    char dir[MAX_PATH];
    sprintf_s(dir, "%s\\Leet", lp);
    CreateDirectoryA(dir, nullptr);
}

void SettingsLoad() {
    char path[MAX_PATH];
    GetConfigPath(path, sizeof(path));
    if (!path[0]) return;
    fullbright_enabled  = GetPrivateProfileIntA("Settings", "Fullbright", 0, path) != 0;
    nohurtcam_enabled   = GetPrivateProfileIntA("Settings", "NoHurtCam", 0, path) != 0;
    xray_enabled        = GetPrivateProfileIntA("Settings", "Xray", 0, path) != 0;
    viewmodel_enabled   = GetPrivateProfileIntA("Settings", "ViewModel", 0, path) != 0;
    xray_key            = (int)GetPrivateProfileIntA("Settings", "XrayKey", 'X', path);
    menu_key            = (int)GetPrivateProfileIntA("Settings", "MenuKey", 0, path);
    g_win_pos_x         = (float)GetPrivateProfileIntA("Settings", "PosX", 60, path);
    g_win_pos_y         = (float)GetPrivateProfileIntA("Settings", "PosY", 60, path);
}

void SettingsSave() {
    EnsureConfigDir();
    char path[MAX_PATH];
    GetConfigPath(path, sizeof(path));
    if (!path[0]) return;
    char buf[32];
    WritePrivateProfileStringA("Settings", "Fullbright", fullbright_enabled ? "1" : "0", path);
    WritePrivateProfileStringA("Settings", "NoHurtCam", nohurtcam_enabled ? "1" : "0", path);
    WritePrivateProfileStringA("Settings", "Xray", xray_enabled ? "1" : "0", path);
    WritePrivateProfileStringA("Settings", "ViewModel", viewmodel_enabled ? "1" : "0", path);
    sprintf_s(buf, "%d", xray_key);
    WritePrivateProfileStringA("Settings", "XrayKey", buf, path);
    sprintf_s(buf, "%d", menu_key);
    WritePrivateProfileStringA("Settings", "MenuKey", buf, path);
    sprintf_s(buf, "%d", (int)g_win_pos_x);
    WritePrivateProfileStringA("Settings", "PosX", buf, path);
    sprintf_s(buf, "%d", (int)g_win_pos_y);
    WritePrivateProfileStringA("Settings", "PosY", buf, path);
}