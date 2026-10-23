# Sky Recomp build: the shared runtime (nxrt/, the submodule: nxrt.mk) + Skyward Sword HD's own sources (runtime/).
# macOS: Homebrew clang/SDL3/FFmpeg, Metal renderer. Windows / Linux: clang + Vulkan renderer.
NXRT     := nxrt
GAME_ID  := sky
GAME_EXE := skyrecomp
GAME_SRC := $(wildcard runtime/*.c) $(wildcard runtime/nvn/*.c) $(wildcard runtime/ui/*.mm)
GAME_INC := -I$(NXRT)/third_party/imgui  # (nvn/texpatch.c draws keyboard glyphs with imstb_truetype)
# the four code modules of Skyward Sword HD 1.0.0, as nx_game.info.modules (runtime/sky_game.c)
GAME_MODULES := main:0x7000000000 sdk:0x7030000000 subsdk0:0x7032000000 subsdk1:0x7034000000
WINDOWS_ICON := docs/icon/icon.ico

include $(NXRT)/nxrt.mk
