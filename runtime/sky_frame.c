// The 120 FPS mode (Video > Frame rate: nx_game hooks max_frame_rate = 120). Skyward Sword advances its world by a
// shared step in 30 Hz units, and keeps the matching duration in milliseconds; the generated code calls these at the
// two stores of them (tools/patches.py TIMING_STORES), so at 120 FPS each frame advances by the time really passed.
#include "frame_rate.h"
#include "hle.h"

static uint64_t last_step_ns;
static float sky_step;

float rt_sky_frame_step(float original) {
    if (rt_frame_rate() != 120) { last_step_ns = 0; return original; }
    uint64_t now = plat_now_ns(), elapsed = now - last_step_ns;
    sky_step = original * 0.5f;
    // real-time speed below the target too; a loading stall must not become a giant physics step
    if (last_step_ns && elapsed < 100000000ull)
        sky_step = elapsed >= 33333333ull ? 1.0f : (float)((double)elapsed * 30.0 / 1e9);
    last_step_ns = now;
    static unsigned calls;
    if (ENV("NXRT_LOG_FRAMEPACE") && ++calls % 120 == 0)
        LOG("[frame-rate] target 120 step %.6f ms %.3f\n", sky_step, sky_step * (1000.0f / 30));
    return sky_step;
}

float rt_sky_frame_ms(float original) { return rt_frame_rate() == 120 ? sky_step * (1000.0f / 30) : original; }
