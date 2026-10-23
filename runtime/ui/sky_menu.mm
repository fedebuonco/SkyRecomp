// Sky Recomp's page in the menu's game tab ("Skyward Sword": its Controls), and the free camera's eye indicator.
#include <math.h>

#include "imgui.h"
#include "option_rows.h"

extern "C" {
#include "../sky_settings.h"
#include "nx_settings.h"
bool sky_free_cam_active(void);  // sky_input.c
}

extern "C" bool sky_tab_controls(float scale) {
    opt_begin("##skycontrols", scale);
    opt_section("Camera");
    if (opt_toggle("Free camera look", &g_sky.free_cam, false,
                   "Moving the right stick looks around without having to hold L first. A short L press switches it "
                   "off / on for this session; holding L while the eye is on turns the free camera off for the hold, so "
                   "the right stick swings the sword. The eye at the bottom right shows the state. Targeting (ZL), items "
                   "(ZR) and the pouch (R) keep their normal right-stick behavior."))
        settings_save();
    if (!g_sky.free_cam) opt_disable_next();
    if (opt_slider("Free camera sensitivity", &g_sky.free_cam_sens, 0.25f, 2.0f, 0.05f, 1.0f, "%.2fx",
                   "How fast the right stick turns the camera while free camera look is active (1.00x = the game's speed)."))
        settings_save();
    opt_section("Mouse");
    if (!g_sky.free_cam) opt_disable_next();
    if (opt_toggle("Mouse camera and combat", &g_sky.mouse_controls, true,
                   "Move the mouse to look around. Left click: sword slash (move before clicking to choose direction). "
                   "Right click: target / shield bash. Middle click: sword thrust. Hold Left Alt or Mouse 4 for item "
                   "aim: left click readies / uses the selected item; hold and release to fire the bow. Hold 3: item "
                   "wheel. Mouse 5 or E: pouch. Esc releases the mouse. Explicit mouse button bindings take priority. "
                   "Needs free camera look."))
        settings_save();
    if (!g_sky.free_cam || !g_sky.mouse_controls) opt_disable_next();
    if (opt_slider("Mouse sensitivity", &g_sky.mouse_sens, 0.25f, 4.0f, 0.05f, 1.0f, "%.2fx", "How fast the mouse turns the camera."))
        settings_save();
    opt_end();
    return false;
}

// HUD: the free camera's state at the bottom right, no text - an eye while it is on, crossed out while L toggled it off
extern "C" bool sky_overlay_wanted(void) { return g_sky.free_cam; }
extern "C" void sky_overlay_draw(float w, float h, void *) {
    ImDrawList *dl = ImGui::GetForegroundDrawList();
    const float s = fmaxf(h / 1080.0f, 0.45f);
    const ImVec2 c(w - 30 * s, h - 28 * s);
    const float rx = 14 * s, ry = 8 * s;
    const bool on = sky_free_cam_active();
    const ImU32 line = on ? IM_COL32(240, 235, 220, 80) : IM_COL32(240, 235, 220, 38);
    dl->AddBezierQuadratic(ImVec2(c.x - rx, c.y), ImVec2(c.x, c.y - ry * 1.6f), ImVec2(c.x + rx, c.y), line, 1.5f * s);
    dl->AddBezierQuadratic(ImVec2(c.x - rx, c.y), ImVec2(c.x, c.y + ry * 1.6f), ImVec2(c.x + rx, c.y), line, 1.5f * s);
    dl->AddCircleFilled(c, 3.2f * s, on ? IM_COL32(240, 235, 220, 120) : IM_COL32(240, 235, 220, 55));
    if (!on) dl->AddLine(ImVec2(c.x - rx * 0.9f, c.y + ry * 1.2f), ImVec2(c.x + rx * 0.9f, c.y - ry * 1.2f), IM_COL32(240, 235, 220, 90), 1.5f * s);
}
