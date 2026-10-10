// Renderer: draws a ViewModel on the round AMOLED.
//
// Draws into a full-screen off-screen canvas (466x466x16bit = ~425 KB, in
// PSRAM) and pushes it in one go, so there is no flicker. Falls back to
// drawing straight to the display if the canvas can't be allocated.
#pragma once

#include <M5Unified.h>

#include "ViewModel.h"

namespace curl {

class Renderer {
public:
    bool begin();
    void splash(const char* version);
    void render(const ViewModel& vm);

private:
    LovyanGFX* target();
    void present();

    void drawBattery(LovyanGFX& g, const ViewModel& vm);
    void drawDots(LovyanGFX& g, const ViewModel& vm);
    void drawMain(LovyanGFX& g, const ViewModel& vm);
    void drawSecondary(LovyanGFX& g, const ViewModel& vm);
    void drawHistory(LovyanGFX& g, const ViewModel& vm);

    M5Canvas canvas_;
    bool useCanvas_ = false;
};

}  // namespace curl
