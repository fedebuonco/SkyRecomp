// Sky Recomp's own settings, in the same ini file as the runtime's (nx_game hooks settings_*: runtime/sky_settings.c).
#pragma once
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool free_cam;        // the right stick looks around without holding L (runtime/sky_input.c)
    float free_cam_sens;  // camera-only stick speed, 0.25x to 2x
    bool mouse_controls;  // mouse camera and combat while free camera look is on
    float mouse_sens;     // relative mouse speed, 0.25x to 4x
} SkySettings;

extern SkySettings g_sky;

void sky_settings_defaults(void);
bool sky_settings_key(const char *key, const char *val);
void sky_settings_write(FILE *f);
