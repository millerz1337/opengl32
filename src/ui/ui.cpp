#include <Windows.h>
#include <GL/gl.h>
#include <unordered_map>
#include "vendor/imgui.h"
#include "vendor/imgui_impl_opengl3.h"
#include "vendor/imgui_impl_win32.h"
#include "core/globals.h"
#include "core/settings.h"
#include "ui/ui.h"

bool fullbright_enabled = false;
bool nohurtcam_enabled = false;
bool xray_enabled = false;
int xray_key = 0;
bool viewmodel_enabled = false;
bool show_menu = true;
int menu_key = 0;

float g_menu_min_x = 0.0f;
float g_menu_min_y = 0.0f;
float g_menu_max_x = 0.0f;
float g_menu_max_y = 0.0f;
float g_panel_min_x = 0.0f;
float g_panel_min_y = 0.0f;
float g_panel_max_x = 0.0f;
float g_panel_max_y = 0.0f;

float g_win_pos_x = 60.0f;
float g_win_pos_y = 60.0f;

static bool toggle_was_down = false;
static bool rbtn_was_down = false;
static bool xray_bind_was_down = false;
static ImFont* f_main = nullptr;

static bool is_binding_xray = false;
static bool is_binding_menu = false;
static bool g_suppress_toggle = false;
static bool show_settings = false;
static float menu_alpha = 1.0f;
static float bg_alpha = 1.0f;

struct ToggleAnim {
    float progress = 0.0f;
    bool  last_state = false;
};
static std::unordered_map<ImGuiID, ToggleAnim> g_toggle_anims;

#define C_ACCENT         IM_COL32(138, 125, 179, 255)
#define C_TEXT           IM_COL32(238, 238, 238, 255)

const char* GetKeyName(int key) {
        static char buf[12];
        if (key >= 'A' && key <= 'Z') { buf[0] = (char)key; buf[1] = '\0'; return buf; }
        if (key >= '0' && key <= '9') { buf[0] = (char)key; buf[1] = '\0'; return buf; }
        switch (key) {
            case VK_XBUTTON1: return "MB1";
            case VK_XBUTTON2: return "MB2";
            case VK_MBUTTON: return "MB3";
            case VK_SHIFT: return "SHIFT";
            case VK_CONTROL: return "CTRL";
            case VK_MENU: return "ALT";
            case VK_CAPITAL: return "CAPS";
            case VK_DELETE: return "DELETE";
            case VK_ESCAPE: return "ESC";
            case VK_TAB: return "TAB";
            case VK_SPACE: return "SPACE";
            case VK_RETURN: return "ENTER";
            case VK_BACK: return "BACKSPACE";
            case VK_UP: return "UP";
            case VK_DOWN: return "DOWN";
            case VK_LEFT: return "LEFT";
            case VK_RIGHT: return "RIGHT";
            case VK_HOME: return "HOME";
            case VK_END: return "END";
            case VK_PRIOR: return "PGUP";
            case VK_NEXT: return "PGDN";
            case VK_INSERT: return "INS";
            default:
                if (key >= VK_F1 && key <= VK_F12) {
                    static const char* names[] = { "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12" };
                    return names[key - VK_F1];
                }
                return "None";
        }
    }
static bool CustomCheckbox(const char* label, bool* val)
{
    float avail = ImGui::GetContentRegionAvail().x;
    float rowH = 28.f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGuiID animId = ImGui::GetID(label);
    ToggleAnim& anim = g_toggle_anims[animId];

    ImGui::PushID(label);
    
    float boxSize = 18.f;
    ImVec2 boxMin(pos.x + avail - boxSize - 2, pos.y + (rowH - boxSize) * 0.5f);
    
    ImGui::SetCursorScreenPos(boxMin);
    bool clicked = ImGui::InvisibleButton("##toggle", ImVec2(boxSize, boxSize));
    if (clicked) *val = !*val;

    float target = *val ? 1.0f : 0.0f;
    anim.progress += (target - anim.progress) * ImGui::GetIO().DeltaTime * 6.0f;
    if (anim.progress < 0.001f) anim.progress = 0.0f;

    int text_alpha = (int)(255.0f * menu_alpha);
    if (f_main) ImGui::PushFont(f_main, f_main->LegacySize);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(pos.x, pos.y + (rowH - ts.y) * 0.5f),
        IM_COL32(255, 255, 255, text_alpha), label);
    if (f_main) ImGui::PopFont();

    ImVec2 boxMax(boxMin.x + boxSize, boxMin.y + boxSize);

    dl->AddRectFilled(boxMin, boxMax, IM_COL32(30, 30, 35, (int)(255.0f * menu_alpha)), 4.f);
    dl->AddRect(boxMin, boxMax, IM_COL32(90, 90, 100, (int)(255.0f * menu_alpha)), 4.f, 0, 1.5f);

    if (anim.progress > 0.01f) {
        float f = anim.progress;
        ImVec2 center((boxMin.x + boxMax.x) * 0.5f, (boxMin.y + boxMax.y) * 0.5f);
        float half = boxSize * 0.5f * f;
        dl->AddRectFilled(
            ImVec2(center.x - half, center.y - half),
            ImVec2(center.x + half, center.y + half),
            IM_COL32(255, 255, 255, (int)(255.0f * menu_alpha)), 4.f);
    }

    ImGui::PopID();
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + rowH + ImGui::GetStyle().ItemSpacing.y));
    return clicked;
}


static bool PanelRow(ImDrawList* dl, float px, float py, float pw, float rowH,
    const char* label, const char* keyText) {
    int alpha = (int)(255.0f * menu_alpha);
    ImVec2 mp = ImGui::GetIO().MousePos;
    bool hover = mp.x > px && mp.x < px + pw && mp.y > py && mp.y < py + rowH;
    if (f_main) ImGui::PushFont(f_main, 20.0f);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(px + 8.f, py + (rowH - ts.y) * 0.5f),
        IM_COL32(255, 255, 255, alpha), label);
    if (f_main) ImGui::PopFont();
    if (f_main) ImGui::PushFont(f_main, 18.0f);
    ImVec2 kts = ImGui::CalcTextSize(keyText);
    if (f_main) ImGui::PopFont();
    float kcH = 24.f;
    float keyW = kts.x + 14.f;
    ImVec2 kpos(px + pw - 8.f - keyW, py + (rowH - kcH) * 0.5f);
    int kc = hover ? 255 : 190;
    float rad = kcH * 0.5f;
    dl->AddRectFilled(kpos, ImVec2(kpos.x + keyW, kpos.y + kcH),
        IM_COL32(30, 30, 35, (int)(60.0f * menu_alpha)), rad);
    dl->AddRect(kpos, ImVec2(kpos.x + keyW, kpos.y + kcH),
        IM_COL32(kc, kc, kc, (int)(90.0f * menu_alpha)), rad, 0, 1.f);
    if (f_main) ImGui::PushFont(f_main, 18.0f);
    dl->AddText(ImVec2(kpos.x + (keyW - kts.x) * 0.5f, kpos.y + (kcH - kts.y) * 0.5f),
        IM_COL32(255, 255, 255, alpha), keyText);
    if (f_main) ImGui::PopFont();
    return hover;
}

static void HandleBinding() {
    if (!is_binding_xray && !is_binding_menu) return;
    for (int i = 8; i < 256; i++) {
        if (GetAsyncKeyState(i) & 0x8000) {
            if (i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON) continue;
            if (is_binding_xray) {
                xray_key = i;
                is_binding_xray = false;
                xray_bind_was_down = true;
            }
            if (is_binding_menu) {
                menu_key = i;
                is_binding_menu = false;
                g_suppress_toggle = true;
            }
            SettingsSave();
            return;
        }
    }
}

void RenderUI() {
    static bool prev_hovered_window = false;
    static bool prev_over_item = false;
    static bool prev_panel_hovered = false;
    static bool prev_panel_item = false;

    if (!f_main) {
        LoadUIFont();
    }

    bool toggle_pressed = false;
    if (menu_key == 0) {
        toggle_pressed = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
    } else {
        toggle_pressed = (GetAsyncKeyState(menu_key) & 0x8000) != 0;
    }
    if (toggle_pressed && !toggle_was_down && !is_binding_menu && !g_suppress_toggle) {
        show_menu = !show_menu;
        if (!show_menu) show_settings = false;
    }
    toggle_was_down = toggle_pressed;
    if (!toggle_pressed) g_suppress_toggle = false;
    HandleBinding();
    if (xray_key != 0 && !is_binding_xray) {
        bool xray_is_down = GetAsyncKeyState(xray_key) & 0x8000;
        if (xray_is_down && !xray_bind_was_down) xray_enabled = !xray_enabled;
        xray_bind_was_down = xray_is_down;
    }

    float target_alpha = show_menu ? 1.0f : 0.0f;
    menu_alpha += (target_alpha - menu_alpha) * 0.12f;
    bg_alpha += (target_alpha - bg_alpha) * 0.12f;
    if (!show_menu && bg_alpha < 0.01f) {
        bg_alpha = 0.0f;
        menu_alpha = 0.0f;
        return;
    }
    if (bg_alpha < 0.01f) bg_alpha = 0.0f;
    if (menu_alpha < 0.01f) menu_alpha = 0.0f;

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 10.f;
    s.WindowBorderSize = 0.f;
    s.FramePadding = ImVec2(4, 2);
    s.ItemSpacing = ImVec2(6, 4);
    s.WindowPadding = ImVec2(8, 5);
    s.Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 0);

    {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigWindowsMoveFromTitleBarOnly = false;
        ImVec2 win_pos = ImVec2(g_win_pos_x, g_win_pos_y);
        static ImVec2 drag_offset(0.0f, 0.0f);
        static bool dragging = false;
        static bool prev_dragging = false;
        ImVec2 mouse = io.MousePos;
        bool left_down = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

        if (dragging) {
            if (!left_down) dragging = false;
        }
        else {
            if (left_down && !prev_over_item && !prev_panel_item
                && (prev_hovered_window || prev_panel_hovered)) {
                dragging = true;
                drag_offset = ImVec2(mouse.x - win_pos.x, mouse.y - win_pos.y);
            }
        }

        ImVec2 target = win_pos;
        if (dragging) {
            target = ImVec2(mouse.x - drag_offset.x, mouse.y - drag_offset.y);
        }
        win_pos.x += (target.x - win_pos.x) * io.DeltaTime * 22.0f;
        win_pos.y += (target.y - win_pos.y) * io.DeltaTime * 22.0f;
        g_win_pos_x = win_pos.x;
        g_win_pos_y = win_pos.y;
        ImGui::SetNextWindowPos(win_pos, ImGuiCond_Always);

        static int s_save_fullbright = -1;
        static int s_save_nohurtcam = -1;
        static int s_save_xray = -1;
        static int s_save_viewmodel = -1;
        static int s_save_xray_key = -1;

        bool need_save = false;
        if (s_save_fullbright != (int)fullbright_enabled) { s_save_fullbright = (int)fullbright_enabled; need_save = true; }
        if (s_save_nohurtcam != (int)nohurtcam_enabled) { s_save_nohurtcam = (int)nohurtcam_enabled; need_save = true; }
        if (s_save_xray != (int)xray_enabled) { s_save_xray = (int)xray_enabled; need_save = true; }
        if (s_save_viewmodel != (int)viewmodel_enabled) { s_save_viewmodel = (int)viewmodel_enabled; need_save = true; }
        if (s_save_xray_key != xray_key) { s_save_xray_key = xray_key; need_save = true; }
        if (prev_dragging && !dragging) need_save = true;
        prev_dragging = dragging;
        if (need_save) SettingsSave();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_alpha);
    ImGui::SetNextWindowSize(ImVec2(250, 138), ImGuiCond_Always);
    if (ImGui::Begin("monarch", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsz = ImGui::GetWindowSize();
        g_menu_min_x = wpos.x;
        g_menu_min_y = wpos.y;
        g_menu_max_x = wpos.x + wsz.x;
        g_menu_max_y = wpos.y + wsz.y;


        ImDrawList* bgdl = ImGui::GetWindowDrawList();
        ImVec2 ccenter(wpos.x + wsz.x * 0.5f, wpos.y + wsz.y * 0.5f);
        float halfW = wsz.x * 0.5f * bg_alpha;
        float halfH = wsz.y * 0.5f * bg_alpha;
        bgdl->PushClipRect(ImVec2(ccenter.x - halfW, ccenter.y - halfH),
            ImVec2(ccenter.x + halfW, ccenter.y + halfH), true);
        bgdl->AddRectFilled(wpos, ImVec2(wpos.x + wsz.x, wpos.y + wsz.y),
            IM_COL32(15, 15, 18, (int)(128.0f * bg_alpha)), 10.f);

        CustomCheckbox("Fullbright", &fullbright_enabled);
        CustomCheckbox("NoHurtcam", &nohurtcam_enabled);
        CustomCheckbox("ViewModel", &viewmodel_enabled);
        CustomCheckbox("Xray", &xray_enabled);

        bool rbtn_down = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
        if (rbtn_down && !rbtn_was_down && ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
            && !ImGui::IsAnyItemHovered()) {
            show_settings = !show_settings;
        }
        rbtn_was_down = rbtn_down;

        prev_hovered_window = ImGui::IsWindowHovered();
        prev_over_item = ImGui::IsAnyItemHovered();

        bgdl->PopClipRect();
    }
    ImGui::End();
    ImGui::PopStyleVar();

    if (show_settings && show_menu) {
        ImDrawList* pdl = ImGui::GetForegroundDrawList();
        float pw = 165.f, ph = 98.f;
        ImVec2 p0(g_win_pos_x + 260.0f, g_win_pos_y);
        ImVec2 p1(p0.x + pw, p0.y + ph);
        g_panel_min_x = p0.x;
        g_panel_min_y = p0.y;
        g_panel_max_x = p1.x;
        g_panel_max_y = p1.y;
        int alpha = (int)(255.0f * menu_alpha);
        pdl->AddRectFilled(p0, p1, IM_COL32(15, 15, 18, (int)(128.0f * bg_alpha)), 10.f);
        pdl->AddRect(p0, p1, IM_COL32(30, 30, 35, (int)(128.0f * bg_alpha)), 10.f, 0, 1.f);

        ImVec2 mp = ImGui::GetIO().MousePos;
        bool in_panel = mp.x > p0.x && mp.x < p1.x && mp.y > p0.y && mp.y < p1.y;
        bool lbtn = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        static bool p_btn_down = false;
        static ImVec2 p_down_pos(0.f, 0.f);
        if (lbtn && !p_btn_down) {
            p_btn_down = true;
            p_down_pos = mp;
        }
        bool p_clicked = false;
        if (!lbtn && p_btn_down) {
            p_btn_down = false;
            float dx = mp.x - p_down_pos.x, dy = mp.y - p_down_pos.y;
            p_clicked = dx * dx + dy * dy < 36.f;
        }

        float ty = p0.y + 2.f;
        if (f_main) ImGui::PushFont(f_main, 20.0f);
        ImVec2 ts = ImGui::CalcTextSize("Settings");
        ImVec2 xw = ImGui::CalcTextSize("X");
        if (f_main) ImGui::PopFont();
        if (f_main) ImGui::PushFont(f_main, 20.0f);
        pdl->AddText(ImVec2(p0.x + (pw - ts.x) * 0.5f, ty),
            IM_COL32(255, 255, 255, alpha), "Settings");
        if (f_main) ImGui::PopFont();

        float xb = 20.f;
        ImVec2 xr(p1.x - 6.f - xb, p0.y + 2.f);
        bool xhover = in_panel && mp.x > xr.x && mp.x < xr.x + xb && mp.y > xr.y && mp.y < xr.y + xb;
        if (f_main) ImGui::PushFont(f_main, 20.0f);
        pdl->AddText(ImVec2(xr.x + (xb - xw.x) * 0.5f, ty),
            IM_COL32(255, 255, 255, alpha), "X");
        if (f_main) ImGui::PopFont();
        if (p_clicked && xhover) {
            show_settings = false;
            is_binding_xray = false;
            is_binding_menu = false;
        }

        char xrayText[16];
        if (is_binding_xray) strcpy(xrayText, "???");
        else if (xray_key == 0) strcpy(xrayText, "BIND");
        else sprintf(xrayText, "%s", GetKeyName(xray_key));
        char uiText[16];
        if (is_binding_menu) strcpy(uiText, "???");
        else if (menu_key == 0) strcpy(uiText, "DELETE");
        else sprintf(uiText, "%s", GetKeyName(menu_key));

        float ry = p0.y + 31.f;
        float rh = 30.f;
        bool r1hover = PanelRow(pdl, p0.x, ry, pw, rh, "Xray", xrayText);
        if (p_clicked && r1hover && !xhover) is_binding_xray = true;
        ry += rh + 4.f;
        bool r2hover = PanelRow(pdl, p0.x, ry, pw, rh, "UI", uiText);
        if (p_clicked && r2hover && !xhover) is_binding_menu = true;

        prev_panel_hovered = in_panel;
        prev_panel_item = xhover || r1hover || r2hover;
    }
    else {
        g_panel_min_x = g_panel_min_y = g_panel_max_x = g_panel_max_y = 0.0f;
        prev_panel_hovered = false;
        prev_panel_item = false;
    }
}

void InitializeUI() {
    ImGui::GetIO().IniFilename = nullptr;
    SettingsLoad();
}

void LoadUIFont() {
    if (f_main) return;
    ImGuiIO& io = ImGui::GetIO();
    f_main = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f);
    if (!f_main) f_main = io.Fonts->AddFontDefault();
}
void ResetUIFont() { f_main = nullptr; }