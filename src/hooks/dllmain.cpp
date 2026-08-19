#include "pch.h"
#include <Windows.h>
#include "hooks/hooks.h"
#include "core/globals.h"
#include <thread>
#include <atomic>

HMODULE g_hModule = nullptr;

static std::atomic<bool> g_hooks_ready = false;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        std::thread([]() {
            InitializeHooks();
            g_hooks_ready = true;
        }).detach();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (!g_hooks_ready) return TRUE;
        RemoveHooks();
    }
    return TRUE;
}