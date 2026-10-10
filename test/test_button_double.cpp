// ButtonLogic double-click behaviour (the blue button opens the history screen
// with a double-click).
#include "../src/core/ButtonLogic.h"
#include "TestFramework.h"

using namespace curl;

namespace {
// debounce 25 ms, hold 800 ms, double-click window 350 ms
ButtonLogic make() { return ButtonLogic(25, 800, 350); }

// Press at `down`, release at `up` (raw), feeding updates at the debounce edges.
uint8_t pressRelease(ButtonLogic& b, Millis down, Millis up) {
    uint8_t ev = 0;
    ev |= b.update(down, true);
    ev |= b.update(down + 30, true);
    ev |= b.update(up, false);
    ev |= b.update(up + 30, false);
    return ev;
}
}  // namespace

TEST(double_click_single_click_is_delayed_until_window_passes) {
    auto b = make();
    b.update(0, false);
    const uint8_t ev = pressRelease(b, 100, 200);
    CHECK(ev & ButtonLogic::Released);
    CHECK(!(ev & ButtonLogic::Click));                   // not yet: a second click may follow
    CHECK_EQ(b.update(500, false), ButtonLogic::None);   // 300 ms after release
    CHECK_EQ(b.update(560, false), ButtonLogic::Click);  // 360 ms: window passed
    CHECK_EQ(b.update(900, false), ButtonLogic::None);   // reported once
}

TEST(double_click_two_quick_clicks) {
    auto b = make();
    b.update(0, false);
    pressRelease(b, 100, 200);
    const uint8_t ev = pressRelease(b, 400, 480);  // second press 200 ms after release
    CHECK(ev & ButtonLogic::DoubleClick);
    CHECK(!(ev & ButtonLogic::Click));
    CHECK_EQ(b.update(2000, false), ButtonLogic::None);  // no stray Click afterwards
}

TEST(double_click_slow_second_click_is_two_singles) {
    auto b = make();
    b.update(0, false);
    pressRelease(b, 100, 200);
    CHECK_EQ(b.update(600, false), ButtonLogic::Click);  // first click resolved
    const uint8_t ev = pressRelease(b, 700, 800);
    CHECK(!(ev & ButtonLogic::DoubleClick));
    CHECK_EQ(b.update(1200, false), ButtonLogic::Click);
}

TEST(double_click_click_then_hold_is_only_hold) {
    auto b = make();
    b.update(0, false);
    pressRelease(b, 100, 200);
    b.update(400, true);
    CHECK_EQ(b.update(430, true), ButtonLogic::Pressed);
    CHECK_EQ(b.update(1200, true), ButtonLogic::Hold);  // 800 ms after the second press
    b.update(1300, false);
    CHECK_EQ(b.update(1330, false), ButtonLogic::Released);
    CHECK_EQ(b.update(2000, false), ButtonLogic::None);
}

TEST(double_click_late_press_with_no_debounce_resolves_first_click) {
    ButtonLogic b(0, 800, 350);
    b.update(0, false);
    b.update(100, true);
    b.update(200, false);  // first click released; pending
    // Next update only arrives 360 ms later (busy loop) and the button is down.
    const uint8_t ev = b.update(560, true);
    CHECK(ev & ButtonLogic::Click);  // first click reported as a single
    CHECK(ev & ButtonLogic::Pressed);
    CHECK(!(b.update(600, false) & ButtonLogic::DoubleClick));
}

TEST(double_click_suppress_clears_pending_click) {
    auto b = make();
    b.update(0, false);
    pressRelease(b, 100, 200);
    b.suppressUntilRelease();
    CHECK_EQ(b.update(1000, false), ButtonLogic::None);  // pending click discarded
}
