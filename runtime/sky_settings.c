#include "sky_settings.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "nx_ui.h"
#include "settings.h"

SkySettings g_sky;

// Sky Recomp's defaults, its own and its choices for the runtime's: the original 16:9 framing, fullscreen, no
// first-launch questions
void sky_settings_defaults(void) {
    g_sky = (SkySettings){.free_cam = false, .free_cam_sens = 1.0f, .mouse_controls = true, .mouse_sens = 1.0f};
    g_settings.aspect = ASPECT_ORIGINAL;
    g_settings.aspect_asked = true;
    g_ui_settings.fullscreen = true;
    g_ui_settings.menu_tip_seen = true;
}

static float clampf(float v, float lo, float hi, float bad) { return !isfinite(v) ? bad : v < lo ? lo : v > hi ? hi : v; }

bool sky_settings_key(const char *key, const char *val) {
    if (!strcmp(key, "free_cam")) g_sky.free_cam = atoi(val) != 0;
    else if (!strcmp(key, "free_cam_sens")) g_sky.free_cam_sens = clampf((float)atof(val), 0.25f, 2.0f, 1.0f);
    else if (!strcmp(key, "mouse_controls")) g_sky.mouse_controls = atoi(val) != 0;
    else if (!strcmp(key, "mouse_sens")) g_sky.mouse_sens = clampf((float)atof(val), 0.25f, 4.0f, 1.0f);
    else return false;
    return true;
}

void sky_settings_write(FILE *f) {
    fprintf(f, "free_cam = %d\nfree_cam_sens = %.2f\n", g_sky.free_cam, g_sky.free_cam_sens);
    fprintf(f, "mouse_controls = %d\nmouse_sens = %.2f\n", g_sky.mouse_controls, g_sky.mouse_sens);
}
