#include <Windows.h>
#include <GL/gl.h>
#include <cmath>
#include "vendor/MinHook.h"
#include "vendor/imgui.h"
#include "vendor/imgui_impl_opengl3.h"
#include "vendor/imgui_impl_win32.h"
#include "core/globals.h"
#include "ui/ui.h"

void UpdateAllHooks();

typedef void (APIENTRY* glRotatef_t)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* glTranslatef_t)(GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* glPushMatrix_t)(void);
typedef void (APIENTRY* glPopMatrix_t)(void);
typedef void (APIENTRY* glEnable_t)(GLenum);
typedef void (APIENTRY* glDepthFunc_t)(GLenum);
typedef void (APIENTRY* glDepthRange_t)(GLdouble, GLdouble);
typedef void (APIENTRY* glOrtho_t)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef BOOL (WINAPI* wglSwapBuffers_t)(HDC);

static glRotatef_t o_glRotatef = nullptr;
static glTranslatef_t o_glTranslatef = nullptr;
static glPushMatrix_t o_glPushMatrix = nullptr;
static glPopMatrix_t o_glPopMatrix = nullptr;
static glEnable_t o_glEnable = nullptr;
static glDepthFunc_t o_glDepthFunc = nullptr;
static glDepthRange_t o_glDepthRange = nullptr;
static glOrtho_t o_glOrtho = nullptr;
static wglSwapBuffers_t o_wglSwapBuffers = nullptr;

static bool isViewModelActive = false;
static bool isGUIMode = false;
static int  matrixStackDepth = 0;
static int  viewmodelMatrixDepth = 0;
static bool hooks_removed = false;

static bool imguiInit = false;
static bool imgui_rendering = false;
static HWND g_hwnd = nullptr;
static WNDPROC o_WndProc = nullptr;
static HGLRC g_ctx = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

static void ReinitImGui(HWND newHwnd) {
    if (imguiInit) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        if (o_WndProc && g_hwnd)
            SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)o_WndProc);
        o_WndProc = nullptr;
        ImGui::DestroyContext();
        imguiInit = false;
    }
    g_hwnd = newHwnd;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplOpenGL3_Init();
    o_WndProc = (WNDPROC)SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    imguiInit = true;
    ResetUIFont();
    InitializeUI();
    LoadUIFont();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (show_menu) {
        if (msg == WM_MOUSEMOVE) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        }
        else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
                 msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
                 msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {
            POINT pt;
            pt.x = (short)LOWORD(lp);
            pt.y = (short)HIWORD(lp);
            ClientToScreen(hwnd, &pt);
            bool in_menu = pt.x >= g_menu_min_x && pt.x <= g_menu_max_x &&
                pt.y >= g_menu_min_y && pt.y <= g_menu_max_y;
            bool in_panel = pt.x >= g_panel_min_x && pt.x <= g_panel_max_x &&
                pt.y >= g_panel_min_y && pt.y <= g_panel_max_y;
            if (in_menu || in_panel) {
                return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
            }
        }
    }
    return o_WndProc ? CallWindowProc(o_WndProc, hwnd, msg, wp, lp)
                     : DefWindowProc(hwnd, msg, wp, lp);
}

void APIENTRY hooked_glPushMatrix() {
    o_glPushMatrix();
    matrixStackDepth++;
}

void APIENTRY hooked_glPopMatrix() {
    if (isViewModelActive && matrixStackDepth == viewmodelMatrixDepth)
        isViewModelActive = false;
    o_glPopMatrix();
    matrixStackDepth--;
    if (matrixStackDepth < 0) matrixStackDepth = 0;
}

void APIENTRY hooked_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    if (viewmodel_enabled && !isViewModelActive) {
        if (matrixStackDepth >= 1 && matrixStackDepth <= 4 &&
            x > 0.3f && x < 1.2f &&
            y > -1.2f && y < -0.1f &&
            z > -1.5f && z < -0.3f) {
            isViewModelActive = true;
            viewmodelMatrixDepth = matrixStackDepth;
        }
    }
    o_glTranslatef(x, y, z);
}

void APIENTRY hooked_glDepthRange(GLdouble zNear, GLdouble zFar) {
    if (viewmodel_enabled || xray_enabled) {
        if (zNear == 0.0 && zFar == 0.1) {
            isViewModelActive = true;
        }
        else if (zNear == 0.0 && zFar == 1.0) {
            isViewModelActive = false;
        }
    }
    o_glDepthRange(zNear, zFar);
}

void APIENTRY hooked_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    if (xray_enabled && fabs(zNear - 1000.0) < 0.1 && fabs(zFar - 3000.0) < 0.1) {
        isGUIMode = true;
    }
    o_glOrtho(left, right, bottom, top, zNear, zFar);
}

void APIENTRY hooked_glDepthFunc(GLenum func) {
    if (xray_enabled && !isGUIMode && !isViewModelActive) {
        if (func == GL_LEQUAL || func == GL_LESS) {
            o_glDepthFunc(GL_ALWAYS);
            return;
        }
    }
    o_glDepthFunc(func);
}

void APIENTRY hooked_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    if (nohurtcam_enabled && angle < 0.0f && x == 0.0f && y == 0.0f && z == 1.0f &&
        angle > -14.0f)
    {
        return;
    }

    if (viewmodel_enabled && angle < 0.0f && x == 0.0f && y == 0.0f && z == 1.0f &&
        angle <= -14.0f)
    {
        return;
    }

    if (o_glRotatef) o_glRotatef(angle, x, y, z);
}

void APIENTRY hooked_glEnable(GLenum cap)
{
    if (imgui_rendering) {
        if (o_glEnable) o_glEnable(cap);
        return;
    }
    if (fullbright_enabled && cap == GL_LIGHTING) return;
    
    if (o_glEnable) o_glEnable(cap);
}

BOOL WINAPI hooked_wglSwapBuffers(HDC hdc) {
    HWND curHwnd = WindowFromDC(hdc);
    HGLRC curCtx = wglGetCurrentContext();

    UpdateAllHooks();

isGUIMode = false;

    if (o_glDepthFunc) {
        if (xray_enabled) o_glDepthFunc(GL_ALWAYS);
        else o_glDepthFunc(GL_LEQUAL);
    }

    if (imguiInit) {
        if (curHwnd && curHwnd != g_hwnd) {
            ReinitImGui(curHwnd);
        }
        else if (curCtx && g_ctx && curCtx != g_ctx) {
            ReinitImGui(g_hwnd);
        }
    }
    g_ctx = curCtx;

    if (!imguiInit) {
        if (curHwnd) {
            ReinitImGui(curHwnd);
        }
    }

    if (imguiInit) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        imgui_rendering = true;
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        imgui_rendering = false;
    }

return o_wglSwapBuffers(hdc);
}

enum {
    HK_SWAP    = 1 << 0,
    HK_ROTATE  = 1 << 1,
    HK_TRANS   = 1 << 2,
    HK_PUSH    = 1 << 3,
    HK_POP     = 1 << 4,
    HK_ENABLE  = 1 << 5,
    HK_DEPTHF  = 1 << 6,
    HK_DEPTHR  = 1 << 7,
    HK_ORTHO   = 1 << 8,
};

static PVOID p_glRotatef = nullptr;
static PVOID p_glTranslatef = nullptr;
static PVOID p_glPushMatrix = nullptr;
static PVOID p_glPopMatrix = nullptr;
static PVOID p_glEnable = nullptr;
static PVOID p_glDepthFunc = nullptr;
static PVOID p_glDepthRange = nullptr;
static PVOID p_glOrtho = nullptr;
static PVOID p_wglSwapBuffers = nullptr;

static int currentHookMask = 0;

void UpdateAllHooks()
{
    if (hooks_removed) return;

    int newMask = HK_SWAP;
    if (viewmodel_enabled || nohurtcam_enabled) newMask |= HK_ROTATE;
    if (viewmodel_enabled || xray_enabled) newMask |= HK_TRANS | HK_PUSH | HK_POP | HK_DEPTHR;
    if (xray_enabled) newMask |= HK_DEPTHF | HK_ORTHO;
    if (xray_enabled || fullbright_enabled) newMask |= HK_ENABLE;

    if (newMask == currentHookMask) return;

    struct HookEntry { PVOID target; int bit; };
    HookEntry table[] = {
        { p_glRotatef,      HK_ROTATE },
        { p_glTranslatef,   HK_TRANS  },
        { p_glPushMatrix,   HK_PUSH   },
        { p_glPopMatrix,    HK_POP    },
        { p_glEnable,       HK_ENABLE },
        { p_glDepthFunc,    HK_DEPTHF },
        { p_glDepthRange,   HK_DEPTHR },
        { p_glOrtho,        HK_ORTHO  },
        { p_wglSwapBuffers, HK_SWAP   },
    };

    for (auto& h : table) {
        if (!h.target) continue;
        bool want = (newMask & h.bit) != 0;
        bool have = (currentHookMask & h.bit) != 0;
        if (want && !have) MH_EnableHook(h.target);
        else if (!want && have) MH_DisableHook(h.target);
    }
    currentHookMask = newMask;
}

void InitializeHooks()
{
    HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");

    if (!hOpenGL) {
        return;
    }

    p_glRotatef      = GetProcAddress(hOpenGL, "glRotatef");
    p_glTranslatef   = GetProcAddress(hOpenGL, "glTranslatef");
    p_glPushMatrix   = GetProcAddress(hOpenGL, "glPushMatrix");
    p_glPopMatrix    = GetProcAddress(hOpenGL, "glPopMatrix");
    p_glEnable       = GetProcAddress(hOpenGL, "glEnable");
    p_glDepthFunc    = GetProcAddress(hOpenGL, "glDepthFunc");
    p_glDepthRange   = GetProcAddress(hOpenGL, "glDepthRange");
    p_glOrtho        = GetProcAddress(hOpenGL, "glOrtho");
    p_wglSwapBuffers = GetProcAddress(hOpenGL, "wglSwapBuffers");

    MH_Initialize();

    if (p_glRotatef) MH_CreateHook(p_glRotatef, hooked_glRotatef, (LPVOID*)&o_glRotatef);
    if (p_glTranslatef) MH_CreateHook(p_glTranslatef, hooked_glTranslatef, (LPVOID*)&o_glTranslatef);
    if (p_glPushMatrix) MH_CreateHook(p_glPushMatrix, hooked_glPushMatrix, (LPVOID*)&o_glPushMatrix);
    if (p_glPopMatrix) MH_CreateHook(p_glPopMatrix, hooked_glPopMatrix, (LPVOID*)&o_glPopMatrix);
    if (p_glEnable) MH_CreateHook(p_glEnable, hooked_glEnable, (LPVOID*)&o_glEnable);
    if (p_glDepthFunc) MH_CreateHook(p_glDepthFunc, hooked_glDepthFunc, (LPVOID*)&o_glDepthFunc);
    if (p_glDepthRange) MH_CreateHook(p_glDepthRange, hooked_glDepthRange, (LPVOID*)&o_glDepthRange);
    if (p_glOrtho) MH_CreateHook(p_glOrtho, hooked_glOrtho, (LPVOID*)&o_glOrtho);
    if (p_wglSwapBuffers) MH_CreateHook(p_wglSwapBuffers, hooked_wglSwapBuffers, (LPVOID*)&o_wglSwapBuffers);

    UpdateAllHooks();
}

void RemoveHooks()
{
    if (hooks_removed) return;
    hooks_removed = true;

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    currentHookMask = 0;

    if (imguiInit) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (o_WndProc) SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)o_WndProc);
        imguiInit = false;
    }
}
