// Host stand-in for <M5Unified.h>, used ONLY by tools/screenshots.
//
// It lets the real src/ui/Renderer.cpp compile unchanged on a PC: M5.Display
// becomes an in-memory 466x466 sprite and M5Canvas is a plain LGFX_Sprite, so
// "pushSprite to the display" just copies pixels we can then save as a PNG.
// Nothing here is used by the firmware build.
#pragma once

#include <stdio.h>

#include <lgfx/v1/LGFX_Sprite.hpp>

using M5Canvas = lgfx::LGFX_Sprite;

namespace host {

// Same size as the StopWatch panel (see ui/Theme.h kSize).
constexpr int kPanelSize = 466;

struct M5Stub {
    M5Stub() {
        Display.setColorDepth(16);
        Display.createSprite(kPanelSize, kPanelSize);
        Display.fillScreen(0);
    }
    lgfx::LGFX_Sprite Display;
};

struct SerialStub {
    void println(const char* s) { fprintf(stderr, "%s\n", s); }
};

}  // namespace host

inline host::M5Stub M5;
inline host::SerialStub Serial;
