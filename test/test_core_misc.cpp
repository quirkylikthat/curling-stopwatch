// Tests for ButtonLogic, Classifier, Format, RingBuffer, EventFifo.
#include "../src/Config.h"
#include "../src/core/ButtonLogic.h"
#include "../src/core/Classifier.h"
#include "../src/core/EventFifo.h"
#include "../src/core/Format.h"
#include "../src/core/RingBuffer.h"
#include "TestFramework.h"

using namespace curl;

// ---- ButtonLogic ------------------------------------------------------------

TEST(button_bounce_shorter_than_debounce_is_ignored) {
    ButtonLogic b(25, 800);
    CHECK_EQ(b.update(0, false), ButtonLogic::None);
    CHECK_EQ(b.update(10, true), ButtonLogic::None);
    CHECK_EQ(b.update(20, false), ButtonLogic::None);  // bounced back before 25 ms
    CHECK_EQ(b.update(60, false), ButtonLogic::None);
    CHECK(!b.isPressed());
}

TEST(button_click) {
    ButtonLogic b(25, 800);
    b.update(0, false);
    b.update(100, true);
    CHECK_EQ(b.update(130, true), ButtonLogic::Pressed);
    CHECK(b.isPressed());
    b.update(300, false);
    CHECK_EQ(b.update(330, false), ButtonLogic::Released | ButtonLogic::Click);
}

TEST(button_hold_fires_once_and_suppresses_click) {
    ButtonLogic b(25, 800);
    b.update(0, true);
    CHECK_EQ(b.update(30, true), ButtonLogic::Pressed);
    CHECK_EQ(b.update(700, true), ButtonLogic::None);
    CHECK_EQ(b.update(800, true), ButtonLogic::Hold);  // 800 ms after the raw press at t=0
    CHECK_EQ(b.update(2000, true), ButtonLogic::None);
    b.update(2100, false);
    CHECK_EQ(b.update(2130, false), ButtonLogic::Released);
}

TEST(button_survives_millis_wraparound) {
    ButtonLogic b(25, 800);
    const Millis base = 0xFFFFFFF0u;
    b.update(base, true);
    CHECK_EQ(b.update(base + 30, true), ButtonLogic::Pressed);  // wraps past 0
    b.update(base + 100, false);
    CHECK_EQ(b.update(base + 130, false), ButtonLogic::Released | ButtonLogic::Click);
}

// ---- Classifier -------------------------------------------------------------

TEST(split_band_boundaries) {
    CHECK_STR(classify(config::kSplitBands, 2.89f)->label, "PEEL");
    CHECK_STR(classify(config::kSplitBands, 2.90f)->label, "TAKEOUT");
    CHECK_STR(classify(config::kSplitBands, 3.80f)->label, "DRAW");
    CHECK_STR(classify(config::kSplitBands, 4.15f)->label, "GUARD");
    CHECK_STR(classify(config::kSplitBands, 99.0f)->label, "GUARD");
}

TEST(classify_empty_table_returns_null) { CHECK(classify(nullptr, 0, 3.0f) == nullptr); }

TEST(split_bands_are_ascending) {
    const size_t n = sizeof(config::kSplitBands) / sizeof(config::kSplitBands[0]);
    for (size_t i = 1; i < n; ++i) {
        CHECK(config::kSplitBands[i - 1].maxSeconds < config::kSplitBands[i].maxSeconds);
    }
}

// ---- Format -----------------------------------------------------------------

TEST(format_seconds) {
    char buf[12];
    CHECK_STR(formatSeconds(3820000, 2, buf, sizeof(buf)), "3.82");
    CHECK_STR(formatSeconds(3829999, 2, buf, sizeof(buf)), "3.82");  // truncates
    CHECK_STR(formatSeconds(14330000, 1, buf, sizeof(buf)), "14.3");
    CHECK_STR(formatSeconds(0, 2, buf, sizeof(buf)), "0.00");
    CHECK_STR(formatSeconds(50000, 2, buf, sizeof(buf)), "0.05");
    CHECK_STR(formatSeconds(5000000000ULL, 2, buf, sizeof(buf)), "999.99");
}

TEST(format_small_buffer_is_safe) {
    char buf[3];
    formatSeconds(14330000, 2, buf, sizeof(buf));
    CHECK_EQ(buf[2], '\0');
}

TEST(result_csv_truncates_like_the_screen) {
    Measurement m;
    m.sequence = 12;
    m.intervalCount = 2;
    m.kinds[0] = IntervalKind::Split;
    m.durations[0] = 3456000;  // printf("%.2f") would round this to 3.46
    m.kinds[1] = IntervalKind::HogToHog;
    m.durations[1] = 14219999;
    m.complete = true;
    char buf[64];
    CHECK_STR(formatResultCsv(m, "SPLIT+H2H", buf, sizeof(buf)), "csv,12,SPLIT+H2H,3.45,14.21,1");
}

TEST(result_csv_leaves_untimed_interval_empty) {
    Measurement h2h;
    h2h.sequence = 3;
    h2h.intervalCount = 1;
    h2h.kinds[0] = IntervalKind::HogToHog;
    h2h.durations[0] = 14330000;
    h2h.complete = true;
    char buf[64];
    CHECK_STR(formatResultCsv(h2h, "HOG-HOG", buf, sizeof(buf)), "csv,3,HOG-HOG,,14.33,1");

    // Split timed, then the run timed out before the far hog line.
    Measurement partial;
    partial.sequence = 4;
    partial.intervalCount = 1;
    partial.kinds[0] = IntervalKind::Split;
    partial.durations[0] = 3800000;
    partial.complete = false;
    CHECK_STR(formatResultCsv(partial, "SPLIT+H2H", buf, sizeof(buf)), "csv,4,SPLIT+H2H,3.80,,0");
}

TEST(result_csv_small_buffer_is_safe) {
    Measurement m;
    m.sequence = 1;
    char buf[6];
    formatResultCsv(m, "SPLIT", buf, sizeof(buf));
    CHECK_EQ(buf[5], '\0');
}

// ---- RingBuffer / EventFifo ---------------------------------------------------

TEST(ring_buffer_newest_first_and_wraps) {
    RingBuffer<int, 3> r;
    CHECK(r.empty());
    r.push(1);
    r.push(2);
    CHECK_EQ(r.size(), 2u);
    CHECK_EQ(r[0], 2);
    CHECK_EQ(r[1], 1);
    r.push(3);
    r.push(4);  // overwrites 1
    CHECK_EQ(r.size(), 3u);
    CHECK_EQ(r[0], 4);
    CHECK_EQ(r[2], 2);
    r.clear();
    CHECK(r.empty());
}

TEST(event_fifo_order_and_capacity) {
    EventFifo<2> f;
    CHECK(f.push({InputType::ControlClick, 1}));
    CHECK(f.push({InputType::ControlHold, 2}));
    CHECK(!f.push({InputType::TimingTap, 3}));  // full
    InputEvent e;
    CHECK(f.pop(e));
    CHECK(e.type == InputType::ControlClick);
    CHECK(f.pop(e));
    CHECK_EQ(e.timestamp, 2u);
    CHECK(!f.pop(e));
}
