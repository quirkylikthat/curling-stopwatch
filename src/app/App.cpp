#include "App.h"

#include <Arduino.h>

namespace curl {

using namespace config;

App::App() : timer_(kMinTapGapUs) {}

void App::begin() {
    hw_.begin();
    settings_.begin();
    renderer_.begin();
    hw_.setBrightness(kBrightnessActive);
    renderer_.splash(kFirmwareVersion);

    const uint8_t saved = settings_.profileIndex();
    selectProfile(saved < kProfileCount ? saved : 0);

    Serial.printf("[app] curling timer %s ready, profile=%s\n", kFirmwareVersion, profile().name);
    Serial.println("csv,seq,profile,split_s,hog_to_hog_s,complete");

    delay(800);  // let the splash show; the only blocking delay in the program
    lastActivityMs_ = hw_.nowMillis();
}

void App::loop() {
    hw_.update();
    const Micros now = hw_.nowMicros();
    const Millis nowMs = hw_.nowMillis();

    InputEvent e;
    while (hw_.pollEvent(e)) handle(e, nowMs);

    switch (timer_.tick(now)) {
        case SequenceTimer::TickResult::TimedOutEmpty:
            timedOut_ = true;
            haptics_.cancel(nowMs);
            break;
        case SequenceTimer::TickResult::TimedOutPartial:
            haptics_.cancel(nowMs);
            break;
        case SequenceTimer::TickResult::None:
            break;
    }

    if (timer_.hasMeasurement()) onMeasurement(timer_.takeMeasurement());

    haptics_.update(nowMs);
    updatePower(nowMs);
    updateDisplay(now, nowMs);
}

void App::handle(const InputEvent& e, Millis nowMs) {
    // Any input wakes the screen -- but a timing tap is ALWAYS processed too,
    // because the stone is already moving.
    noteActivity(nowMs);

    switch (e.type) {
        case InputType::TimingTap:
            switch (timer_.onTap(e.timestamp)) {
                case SequenceTimer::TapResult::Started:
                    timedOut_ = false;
                    haptics_.tap(nowMs);
                    hw_.beep();
                    break;
                case SequenceTimer::TapResult::GateRecorded:
                    haptics_.tap(nowMs);
                    hw_.beep();
                    break;
                case SequenceTimer::TapResult::Finished:
                    haptics_.result(nowMs);
                    hw_.beep();
                    break;
                case SequenceTimer::TapResult::Ignored:
                    break;
            }
            break;

        case InputType::ControlClick:
            if (timer_.isRunning()) {
                timer_.cancel();  // false start: abandon this run
                haptics_.cancel(nowMs);
            } else {
                selectProfile((profileIndex_ + 1) % kProfileCount);
                haptics_.tap(nowMs);
            }
            break;

        case InputType::ControlHold:
            timer_.cancel();
            history_.clear();
            timedOut_ = false;
            haptics_.cancel(nowMs);
            Serial.println("[app] history cleared");
            break;
    }
}

void App::onMeasurement(const Measurement& m) {
    history_.push(m);

    // Machine-readable line for later export / analysis over USB serial.
    char split[12] = "";
    char h2h[12] = "";
    const int s = m.find(IntervalKind::Split);
    const int h = m.find(IntervalKind::HogToHog);
    if (s >= 0) snprintf(split, sizeof(split), "%.2f", m.seconds(s));
    if (h >= 0) snprintf(h2h, sizeof(h2h), "%.2f", m.seconds(h));
    Serial.printf("csv,%lu,%s,%s,%s,%d\n", static_cast<unsigned long>(m.sequence),
                  kProfiles[m.profileIndex].shortName, split, h2h, m.complete ? 1 : 0);
}

void App::selectProfile(uint8_t index) {
    profileIndex_ = index;
    timer_.setProfile(profile(), profileIndex_);
    timedOut_ = false;
    settings_.setProfileIndex(index);
}

void App::noteActivity(Millis nowMs) {
    lastActivityMs_ = nowMs;
    if (dimmed_) {
        dimmed_ = false;
        hw_.setBrightness(kBrightnessActive);
    }
}

void App::updatePower(Millis nowMs) {
    if (timer_.isRunning()) {
        lastActivityMs_ = nowMs;
        return;
    }
    if (!dimmed_ && nowMs - lastActivityMs_ >= kDimAfterMs) {
        dimmed_ = true;
        hw_.setBrightness(kBrightnessDim);
    }
    // TODO(power): after a longer idle period, deep-sleep and wake on a button
    // via M5PM1 (see docs/HARDWARE.md, "Wake-up sources").
}

void App::updateDisplay(Micros now, Millis nowMs) {
    if (haveView_ && nowMs - lastFrameMs_ < kFrameIntervalMs) return;
    lastFrameMs_ = nowMs;

    ViewModel vm;
    buildView({timer_, profile(), history_, now, hw_.batteryPercent(), hw_.isCharging(), timedOut_}, vm);
    if (haveView_ && vm == lastView_) return;  // nothing changed: skip the ~20 ms push

    renderer_.render(vm);
    lastView_ = vm;
    haveView_ = true;
}

}  // namespace curl
