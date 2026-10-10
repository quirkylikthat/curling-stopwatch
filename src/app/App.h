// App: wires everything together and owns the main loop.
//
//   Hardware --InputEvents--> App --taps--> SequenceTimer --Measurement--> History
//                              |                                      |
//                              +--> Haptics            ViewBuilder <--+
//                                                          |
//                                                      Renderer
//
// Rule of thumb: App decides *what happens*; core/ decides *how timing works*;
// hal/ and ui/ decide *how the device does it*.
#pragma once

#include "../Config.h"
#include "../core/PowerPolicy.h"
#include "../core/SequenceTimer.h"
#include "../hal/Hardware.h"
#include "../hal/Haptics.h"
#include "../hal/Settings.h"
#include "../ui/Renderer.h"
#include "../ui/ViewModel.h"
#include "ViewBuilder.h"

namespace curl {

class App {
public:
    App();
    void begin();
    void loop();

private:
    void handle(const InputEvent& e, Millis nowMs);
    void onMeasurement(const Measurement& m);
    void selectProfile(uint8_t index);
    void noteActivity(Millis nowMs);
    void updatePower(Millis nowMs);
    void enterLightSleep(Millis budgetMs);
    [[noreturn]] void enterDeepSleep();
    void updateDisplay(Micros now, Millis nowMs);
    const TimingProfile& profile() const { return config::kProfiles[profileIndex_]; }

    Hardware hw_;
    Haptics haptics_;
    Settings settings_;
    Renderer renderer_;

    SequenceTimer timer_;
    History history_;
    uint8_t profileIndex_ = 0;
    bool timedOut_ = false;
    bool showingHistory_ = false;  // blue double-click toggles the "last 10" screen

    ViewModel lastView_ = {};
    bool haveView_ = false;
    Millis lastFrameMs_ = 0;
    Millis lastActivityMs_ = 0;
    bool dimmed_ = false;
};

}  // namespace curl
