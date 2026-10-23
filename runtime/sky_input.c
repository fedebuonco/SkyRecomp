// Skyward Sword's free camera look and mouse camera / combat, on nx-input's mouse (nx_input.h) through the game's
// pad hooks (runtime/sky_game.c): mouse motion is the right stick, clicks are sword slashes / thrusts / shield
// bashes, and the right stick turns the camera without holding L.
#include <SDL3/SDL.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "hle.h"
#include "menu.h"
#include "nx_input.h"
#include "sky_settings.h"

static int32_t clamp_axis(int32_t v) { return v > 32767 ? 32767 : v < -32767 ? -32767 : v; }

// ------------------------------------------------------------------ mouse
static _Atomic int32_t mouse_rx, mouse_ry;  // this frame's mouse motion as a right stick
static _Atomic int32_t mouse_slash_x, mouse_slash_y;
static _Atomic uint64_t mouse_slash_until, mouse_shield_until;
static uint64_t mouse_frame_ns;

static bool unbound(int button) { return !nxin_mouse_bound(button); }
static bool item_mode(void) {
    const bool *k = SDL_GetKeyboardState(NULL);
    return (k && k[SDL_SCANCODE_LALT]) || (nxin_mouse_button(SDL_BUTTON_X1) && unbound(SDL_BUTTON_X1));
}

// once per presented frame: the motion since the last frame, scaled so a given hand speed turns alike at any frame rate
void sky_mouse_frame(void) {
    uint64_t now = SDL_GetTicksNS();
    double dt = mouse_frame_ns && now > mouse_frame_ns ? (now - mouse_frame_ns) * 1e-9 : 1.0 / 60;
    mouse_frame_ns = now;
    dt = fmax(1.0 / 240, fmin(.1, dt));
    double gain = 1400.0 / (60 * dt * 256) * g_sky.mouse_sens;
    int32_t dx, dy;
    nxin_mouse_take(&dx, &dy);
    mouse_rx = (int32_t)fmax(-32767, fmin(32767, dx * gain));
    mouse_ry = (int32_t)fmax(-32767, fmin(32767, -dy * gain));
}

bool sky_mouse_wanted(void) { return g_sky.free_cam && g_sky.mouse_controls; }

// SDL events the game sees: a click starts a slash / shield bash (time-based pulses: 60 and 120 FPS alike)
void sky_input_event(const SDL_Event *e) {
    if (e->type != SDL_EVENT_MOUSE_BUTTON_DOWN || !nxin_mouse_captured() || menu_input_captured()) return;
    int btn = e->button.button;
    if (!unbound(btn)) return;
    uint64_t now = SDL_GetTicks();
    if (btn == SDL_BUTTON_LEFT && !(input_buttons_now(false) & (1ull << NX_BTN_ZR)) && !item_mode()) {
        int32_t x = mouse_rx, y = mouse_ry;
        if (abs(x) + abs(y) < 4000) x = 32767, y = 0;
        mouse_slash_x = x < -4000 ? -32767 : x > 4000 ? 32767 : 0;
        mouse_slash_y = y < -4000 ? -32767 : y > 4000 ? 32767 : 0;
        mouse_slash_until = now + 100;
    }
    if (btn == SDL_BUTTON_RIGHT && !item_mode()) mouse_shield_until = now + 80;
}

static bool g_mouse_sword;  // (this poll: the mouse swings the sword / aims: the free camera stays out of it)
static bool free_cam_on = true;

// pad hook, after the d-pad walk: the mouse's buttons and motion into the pad
void sky_pad_filter(NxPad *p) {
    g_mouse_sword = false;
    if (!nxin_mouse_captured() || menu_input_captured() || !g_sky.free_cam || !g_sky.mouse_controls) return;
    uint64_t b = p->buttons, now = SDL_GetTicks();
    bool im = item_mode();
    if (im) mouse_slash_until = 0;
    if (nxin_mouse_button(SDL_BUTTON_RIGHT) && unbound(SDL_BUTTON_RIGHT)) b |= 1ull << NX_BTN_ZL;
    if (!im && now < mouse_shield_until && unbound(SDL_BUTTON_RIGHT)) b |= 1ull << NX_BTN_LSTICK;
    if (nxin_mouse_button(SDL_BUTTON_X2) && unbound(SDL_BUTTON_X2)) b |= 1ull << NX_BTN_R;
    bool thrust = !im && nxin_mouse_button(SDL_BUTTON_MIDDLE) && unbound(SDL_BUTTON_MIDDLE);
    if (thrust) b |= 1ull << NX_BTN_RSTICK;
    bool item = im || (b & (1ull << NX_BTN_ZR)) != 0;
    if (im && nxin_mouse_button(SDL_BUTTON_LEFT) && unbound(SDL_BUTTON_LEFT)) b |= 1ull << NX_BTN_ZR;
    bool slash = !item && now < mouse_slash_until && unbound(SDL_BUTTON_LEFT);
    // a click starts at neutral, then flicks the stick
    if (slash) {
        bool neutral = mouse_slash_until - now > 80;
        p->rx = neutral ? 0 : mouse_slash_x;
        p->ry = neutral ? 0 : mouse_slash_y;
    } else if (item || (b & (1ull << NX_BTN_R)) || (free_cam_on && !(b & (1ull << NX_BTN_ZL)))) {
        p->rx = clamp_axis(p->rx + mouse_rx), p->ry = clamp_axis(p->ry + mouse_ry);
    }
    if (slash || thrust || im) b &= ~(1ull << NX_BTN_L);
    p->buttons = b;
    g_mouse_sword = slash || thrust || im;
}

// ------------------------------------------------------------------ free camera look
// A short L press switches it off / on for the session (the setting stays); holding L longer keeps L's own game
// function. free_cam_l_down / free_cam_l_ms: the press edge and its time.
enum { FREE_CAM_TAP_MS = 300 };
static bool free_cam_active, free_cam_l_down;
static uint64_t free_cam_l_ms;

bool sky_free_cam_active(void) { return free_cam_active; }
RT_ON_RESET(sky_free_camera_reset) { free_cam_on = true; free_cam_l_down = false; free_cam_active = false; mouse_frame_ns = 0; }

// pad hook, last: moving the right stick acts as if L were held too, so the camera turns without holding L first.
//  - Holding L while it is on turns the free camera off for the hold and withholds L itself, so the right stick swings
//    the sword; releasing L brings it back.
//  - While using items (ZR), targeting (ZL) or holding the pouch wheel (R) it is off: the stick keeps its game action.
//  - free_cam_sens scales the stick for the camera only (1.00x = the game's own speed).
void sky_pad_final(NxPad *p) {
    bool captured = menu_input_captured() || g_mouse_sword;
    if (captured) { free_cam_l_down = false; free_cam_active = false; return; }
    uint64_t b = p->buttons, now = SDL_GetTicks();
    int32_t rx = p->rx, ry = p->ry;
    if (g_sky.free_cam) {
        const bool l_now = (b & (1ull << NX_BTN_L)) != 0;
        if (l_now && !free_cam_l_down) {
            free_cam_l_down = true;
            free_cam_l_ms = now;
        } else if (!l_now && free_cam_l_down) {
            free_cam_l_down = false;
            if (now - free_cam_l_ms <= FREE_CAM_TAP_MS) {
                free_cam_on = !free_cam_on;
                LOG("[freecam] free camera %s (short L press)\n", free_cam_on ? "on" : "off");
            }
        }
        const bool hold_off = l_now && free_cam_on;
        if (hold_off) b &= ~(1ull << NX_BTN_L);
        const bool busy = b & ((1ull << NX_BTN_R) | (1ull << NX_BTN_ZL) | (1ull << NX_BTN_ZR));
        if (free_cam_on && !hold_off && !busy && (rx || ry)) {
            const float sens = g_sky.free_cam_sens;
            if (sens > 0.0f && sens != 1.0f) rx = clamp_axis((int32_t)((float)rx * sens)), ry = clamp_axis((int32_t)((float)ry * sens));
            b |= 1ull << NX_BTN_L;
        }
    } else {
        free_cam_on = true;  // (switching the setting back on starts with the free camera active)
        free_cam_l_down = false;
    }
    free_cam_active = g_sky.free_cam && free_cam_on && !(b & ((1ull << NX_BTN_R) | (1ull << NX_BTN_ZL) | (1ull << NX_BTN_ZR))) &&
                      !free_cam_l_down;
    // the right stick's directions as buttons again, for the stick as it now is
    b &= ~(0xfull << 20);
    if (rx < -16000) b |= 1ull << 20;
    if (ry > 16000) b |= 1ull << 21;
    if (rx > 16000) b |= 1ull << 22;
    if (ry < -16000) b |= 1ull << 23;
    p->buttons = b, p->rx = rx, p->ry = ry;
}
