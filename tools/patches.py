"""Sky Recomp's code changes, applied by nxrt/tools/gen.py (--game tools): Skyward Sword HD 1.0.0, main module.

The 120 FPS mode: the game stores its shared world step (in 30 Hz units) and the matching duration in milliseconds
from s0 / s1; at those two stores the generated code asks the runtime instead (runtime/sky_frame.c rt_sky_frame_step /
rt_sky_frame_ms), and stores what it returns.
"""
import struct

# address -> (vector register, store offset from x8, the original instruction word)
TIMING_STORES = {
    0x700104a1ac: (0, 0xd38, 0xbd0d3900),
    0x700104a1d0: (1, 0xd3c, 0xbd0d3d01),
}


def _timing_store(gen, a):
    reg, offset, word = TIMING_STORES[a]
    assert struct.unpack_from('<I', gen.nso.image, a - gen.base)[0] == word, hex(a)
    # scale the register too: the following accumulator derives the original 30 Hz integer ticks from it
    hook = 'rt_sky_frame_step' if reg == 0 else 'rt_sky_frame_ms'
    # (lane 0 through may_alias pointers, as simd.h's VSF32 / VS32: simd.h is only included with --simd c)
    return (f'{{ extern float {hook}(float); typedef float __attribute__((may_alias)) f32a; '
            f'typedef uint32_t __attribute__((may_alias)) u32a; '
            f'*(f32a *)&c->v[{reg}] = {hook}(*(f32a *)&c->v[{reg}]); '
            f'ST32(c->x[8] + {hex(offset)}ull, *(u32a *)&c->v[{reg}]); }}  /* {a:#x}: str s{reg}, [x8, #{hex(offset)}] */')


PATCHES = {a: _timing_store for a in TIMING_STORES}
VARIANTS = {}
