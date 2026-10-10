#include "App.h"

#include <Arduino.h>

#include "../hal/SleepMemory.h"

namespace curl {

using namespace config;

namespace {
// NVS blob for the user's menu settings. Bump kUserVersion whenever the
// UserSettings layout changes: an old blob then fails to load (size/version
// mismatch) and the defaults are used instead of misread bytes.
constexpr const char* kUserKey = "user";
constexpr uint16_t kUserVersion = 1;
struct StoredUser {
    uint16_t version;
    UserSettings settings;
};
}  // namespace

App::App() : timer_(kMinTapGapUs) {}

void App::begin() {
    hw_.begin();
    settings_.begin();
    loadUserSettings();
    renderer_.begin();
    hw_.setBrightness(user_.brightness);

    const bool resumed = hw_.wokeFromDeepSleep();
    if (!resumed) renderer_.splash(kFirmwareVersion);

    const uint8_t saved = settings_.profileIndex();
    selectProfile(saved < kProfileCount ? saved : 0);

    if (resumed) {
        // Woke from deep sleep (= reboot): bring back this session's history.
        uint32_t next = 1;
        if (sleepmem::restore(history_, next)) timer_.setNextSequence(next);
        Serial.printf("[app] resumed from deep sleep, %u results restored\n",
                      static_cast<unsigned>(history_.size()));
    } else {
        sleepmem::discard();  // power-on / reset: never resurrect old history
    }

    Serial.printf("[app] curling timer %s ready, profile=%s\n", kFirmwareVersion, profile().name);
    Serial.println("csv,seq,profile,split_s,hog_to_hog_s,complete");

    if (!resumed) delay(800);  // let the splash show; the only blocking delay in the program
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

    // Let the CPU core idle instead of spinning flat out. Taps are timestamped
    // in the ISR, so this adds no timing error; it only delays *handling* by ~1 ms.
    if (kLoopIdleMs) delay(kLoopIdleMs);
}

void App::handle(const InputEvent& e, Millis nowMs) {
    // Any input wakes the screen -- but a timing tap is ALWAYS processed too,
    // because the stone is already moving.
    noteActivity(nowMs);

    switch (e.type) {
        case InputType::TimingTap:
            // From the history screen or the menu: jump back AND time -- the
            // tap is never lost.
            showTiming();
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
            if (view_ == View::Menu) {
                onMenuAction(menu_.back(), nowMs);  // back one page (or out of the menu)
            } else if (view_ == View::History) {
                view_ = View::Timing;  // "back" -- doesn't also change mode
                haptics_.tap(nowMs);
            } else if (timer_.isRunning()) {
                timer_.cancel();  // false start: abandon this run
                haptics_.cancel(nowMs);
            } else {
                selectProfile((profileIndex_ + 1) % kProfileCount);
                haptics_.tap(nowMs);
            }
            break;

        case InputType::ControlHold:  // 2 s: open the menu, or leave it
            if (view_ == View::Menu) {
                showTiming();
            } else {
                menu_.open();
                view_ = View::Menu;
            }
            haptics_.result(nowMs);
            break;

        case InputType::ControlDoubleClick:
            if (view_ == View::Menu) break;  // no meaning inside the menu
            view_ = (view_ == View::History) ? View::Timing : View::History;
            haptics_.tap(nowMs);
            break;

        case InputType::TouchTap:
            if (view_ == View::Menu) onMenuAction(menu_.tap(e.x, e.y, user_), nowMs);
            break;  // touch is ignored outside the menu (in-game screens)

        case InputType::TouchDrag:
            if (view_ == View::Menu) onMenuAction(menu_.drag(e.x, e.y, user_), nowMs);
            break;
    }
}

void App::onMenuAction(Menu::Action a, Millis nowMs) {
    switch (a) {
        case Menu::Action::None:
            return;
        case Menu::Action::Navigated:
            saveUserSettingsIfChanged();  // leaving an editor page
            haptics_.tap(nowMs);
            return;
        case Menu::Action::SettingsChanged:
            splitBands_ = settings::makeSplitBands(user_);
            userDirty_ = true;
            haptics_.tap(nowMs);
            return;
        case Menu::Action::BrightnessChanged:
            hw_.setBrightness(user_.brightness);  // live preview while dragging
            userDirty_ = true;
            return;  // no buzz: it would fire on every drag sample
        case Menu::Action::ClearHistory:
            timer_.cancel();
            history_.clear();
            timedOut_ = false;
            haptics_.cancel(nowMs);
            Serial.println("[app] history cleared");
            return;
        case Menu::Action::Exit:
            showTiming();
            haptics_.tap(nowMs);
            return;
    }
}

void App::showTiming() {
    if (view_ == View::Menu) saveUserSettingsIfChanged();
    view_ = View::Timing;
}

void App::loadUserSettings() {
    StoredUser stored{};
    if (settings_.loadBlob(kUserKey, &stored, sizeof(stored)) && stored.version == kUserVersion &&
        settings::isValid(stored.settings)) {
        user_ = stored.settings;
    } else {
        user_ = settings::defaults();
    }
    splitBands_ = settings::makeSplitBands(user_);
}

void App::saveUserSettingsIfChanged() {
    if (!userDirty_) return;
    StoredUser stored{};
    stored.version = kUserVersion;
    stored.settings = user_;
    settings_.saveBlob(kUserKey, &stored, sizeof(stored));
    userDirty_ = false;
    Serial.println("[app] settings saved");
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
        hw_.setBrightness(user_.brightness);
    }
}

void App::updatePower(Millis nowMs) {
    if (timer_.isRunning()) lastActivityMs_ = nowMs;  // never sleep mid-run

    const PowerTimeouts t{kDimAfterMs, kLightSleepAfterMs, kDeepSleepAfterMs};
    const bool sleepAllowed = kSleepOnUsbPower || !hw_.onUsbPower();
    const Millis idle = nowMs - lastActivityMs_;

    switch (powerLevelFor(idle, t, sleepAllowed)) {
        case PowerLevel::Active:
            break;  // noteActivity() restores brightness
        case PowerLevel::Dim:
            if (!dimmed_) {
                dimmed_ = true;
                // Never "dim" to something brighter than the user's setting.
                hw_.setBrightness(kBrightnessDim < user_.brightness ? kBrightnessDim
                                                                    : user_.brightness);
            }
            break;
        case PowerLevel::LightSleep:
            enterLightSleep(lightSleepBudget(idle, t));
            break;
        case PowerLevel::DeepSleep:
            enterDeepSleep();
    }
}

void App::enterLightSleep(Millis budgetMs) {
    haptics_.stop();
    showTiming();  // always wake up on the timing screen (saves menu edits)
    const Hardware::WakeResult r = hw_.lightSleep(budgetMs);

    switch (r.reason) {
        case Hardware::WakeReason::Timer:
            // Still idle when deep sleep came due: go deeper without ever
            // turning the screen back on.
            enterDeepSleep();

        case Hardware::WakeReason::Other:
            // Not a button. Leave the screen off; the next loop() sees we're
            // still idle and goes straight back to sleep.
            return;

        case Hardware::WakeReason::TimingButton:
        case Hardware::WakeReason::ControlButton:
            break;
    }

    const Millis nowMs = hw_.nowMillis();
    hw_.wakeUi();
    dimmed_ = true;  // so noteActivity() restores full brightness
    noteActivity(nowMs);
    haveView_ = false;  // force a full redraw

    // Yellow: the stone may already be moving -- the wake press is the first
    // gate, timestamped ~1 ms after the press. Blue only wakes.
    if (r.reason == Hardware::WakeReason::TimingButton && kLightSleepWakeTapCounts) {
        handle({InputType::TimingTap, r.wokeAt, 0, 0}, nowMs);
    }
}

void App::enterDeepSleep() {
    haptics_.stop();
    saveUserSettingsIfChanged();
    sleepmem::save(history_, timer_.nextSequence());
    Serial.printf("[app] deep sleep, %u results saved\n", static_cast<unsigned>(history_.size()));
    hw_.deepSleep();
}

void App::updateDisplay(Micros now, Millis nowMs) {
    if (haveView_ && nowMs - lastFrameMs_ < kFrameIntervalMs) return;
    lastFrameMs_ = nowMs;

    ViewModel vm;
    const Menu::Page page = menu_.page();
    buildView(
        {timer_, profile(), history_, now, hw_.batteryPercent(), hw_.isCharging(), timedOut_,
         view_ == View::History, splitBands_.bands, view_ == View::Menu ? &page : nullptr, &user_},
        vm);
    if (haveView_ && vm == lastView_) return;  // nothing changed: skip the ~20 ms push

    renderer_.render(vm);
    lastView_ = vm;
    haveView_ = true;
}

}  // namespace curl
