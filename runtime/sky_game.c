// Sky Recomp on the shared runtime (nxrt): who the game is - Skyward Sword HD 1.0.0, four code modules - and its hooks.
// Its own code: texture restyling (nvn/texpatch.c), free camera and mouse controls (sky_input.c), the 120 FPS step
// (sky_frame.c), its settings and page (sky_settings.c, ui/sky_menu.mm); the patches for the recompiler in
// tools/patches.py.
#include <string.h>

#include "nvn/texpatch.h"
#include "nxgame.h"
#include "rt.h"
#include "sky_settings.h"

// The release tag this build is: beta_<n> until 1.0, then v<major>.<minor>.<patch>.
#define SKY_VERSION "v1.0.6"

// sky_input.c
void sky_mouse_frame(void);
bool sky_mouse_wanted(void);
void sky_input_event(const SDL_Event *e);
void sky_pad_filter(NxPad *p);
void sky_pad_final(NxPad *p);
// ui/sky_menu.mm
bool sky_tab_controls(float scale);
bool sky_overlay_wanted(void);
void sky_overlay_draw(float w, float h, void *font);

static void present_tick(uint64_t frame) { (void)frame; sky_mouse_frame(); }

// the boot movies A / B skip
static bool movie_skippable(const char *path) {
    return strstr(path, "Movie/Startup.webm") || strstr(path, "Movie/Title.webm") || strstr(path, "THP/Demo01_01.webm");
}

// the system language as the game takes it: Chinese as zh-Hans / zh-Hant
static const char *language_code(const char *code) {
    return !strcmp(code, "zh-CN") ? "zh-Hans" : !strcmp(code, "zh-TW") ? "zh-Hant" : NULL;
}

// guest code the game branches into in the middle of a function: those run that function
static const NxCodeRange inner_entries[] = {
    {0x7000ef6f28ull, 0x7000ef7194ull}, {0x7000ef7324ull, 0x7000ef7538ull}, {0x7000ef76b4ull, 0x7000ef784cull},
    {0x7000ef7b80ull, 0x7000ef7e38ull}, {0x7000ef7fb8ull, 0x7000ef8078ull},
};

static const NxMenuTab menu_pages[] = {
    {"Controls", 20, false, false, sky_tab_controls},
};

// The console's "attach the Joy-Con straps" system dialog (nn::hid::ShowControllerStrapGuide) has no meaning in a
// desktop port: nn::hid opens a library applet for it through nn::applet, and the runtime has no applet process to
// run it, so boot stopped there. Replace that one SDK function with a no-op (runtime/rt.h hle_override).
static void noop_void(Cpu *c) { (void)c; }
static void sky_module_overrides(void) {
    static bool done;
    if (done) return;
    done = true;
    hle_override(0x7030244c80ull, noop_void, "nn::hid::ShowControllerStrapGuide");
}

const NxGame nx_game = {
    .info = {
        .id = "sky",
        .exe = "skyrecomp",
        .title = "Sky Recomp",
        .game_name = "The Legend of Zelda: Skyward Sword HD",
        .game_version = "1.0.0",
        .title_id = 0x01002DA013484000ull,
        .build_id = {0xd8, 0x24, 0xd4, 0xb1, 0xbb, 0xd4, 0x5d, 0x03},
        .bundle_id = "com.skyrecomp.app",
        .data_dir = "skyrecomp",
        .ini = "skyrecomp.ini",
        .version = SKY_VERSION,
        .releases_api = "https://api.github.com/repos/Apolinario-coder/SkyRecomp/releases?per_page=100",
        .release_page = "https://github.com/Apolinario-coder/SkyRecomp/releases/tag/",
        .headline = "Sky Recomp",
        .romfs_probe = "Object",
        .title_movie = "THP/Demo01_01.webm",
        .nickname = "Link",
        .menu_name = "Skyward Sword",
        .modules = {{"main", 0x7000000000ull}, {"sdk", 0x7030000000ull}, {"subsdk0", 0x7032000000ull},
                    {"subsdk1", 0x7034000000ull}},
    },
    .hooks = {
        .modules_loaded = sky_module_overrides,
        .inner_entries = inner_entries,
        .ninner_entries = sizeof inner_entries / sizeof *inner_entries,
        .present_tick = present_tick,
        .movie_skippable = movie_skippable,
        .input_event = sky_input_event,
        .mouse_wanted = sky_mouse_wanted,
        .stick_deadzone = 8000,  // (a resting pad's drift must not become movement or sword input)
        .keyboard_prompts = true,
        .pad_filter = sky_pad_filter,
        .pad_final = sky_pad_final,
        .language_code = language_code,
        .max_frame_rate = 120,
        .settings_defaults = sky_settings_defaults,
        .settings_key = sky_settings_key,
        .settings_write = sky_settings_write,
        .tex_identify = texpatch_identify,
        .tex_generation = texpatch_generation,
        .tex_apply = texpatch_apply,
        .menu_pages = menu_pages,
        .menu_npages = sizeof menu_pages / sizeof *menu_pages,
        .overlay_wanted = sky_overlay_wanted,
        .overlay_draw = sky_overlay_draw,
    },
};
