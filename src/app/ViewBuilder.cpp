#include "ViewBuilder.h"

#include <stdio.h>
#include <string.h>

#include "../core/Classifier.h"
#include "../core/Format.h"

namespace curl {

namespace {

template <size_t N>
void copyText(char (&dst)[N], const char* src) {
    snprintf(dst, N, "%s", src ? src : "");
}

void fillReadout(Readout& r, IntervalKind kind, Micros duration, int decimals, bool withCaption,
                 bool withBand, const Band* splitBands) {
    formatSeconds(duration, decimals, r.time, sizeof(r.time));
    copyText(r.caption, withCaption ? intervalName(kind) : "");
    if (withBand) {
        if (const Band* b = bandFor(kind, static_cast<float>(duration) / 1e6f, splitBands)) {
            copyText(r.band, b->label);
            r.bandColor = b->color;
        }
    }
}

void fillHistory(ViewModel& out, const History& history, size_t skip) {
    for (size_t i = skip; i < history.size() && out.historyCount < 3; ++i) {
        const Measurement& m = history[i];
        if (m.intervalCount == 0) continue;
        formatSeconds(m.durations[m.intervalCount - 1], 2, out.history[out.historyCount],
                      sizeof(out.history[0]));
        ++out.historyCount;
    }
}

static_assert(config::kHistoryScreenRows <= ViewModel::kMaxHistoryRows,
              "kHistoryScreenRows exceeds ViewModel::kMaxHistoryRows");
static_assert(config::kHistoryScreenRows <= config::kHistorySize,
              "kHistoryScreenRows exceeds the history kept in RAM");

// History screen: one row per throw, split on the left, hog-to-hog on the
// right -- on the same row when both were timed on that throw (SPLIT+H2H).
void fillHistoryScreen(ViewModel& out, const History& history, const Band* splitBands) {
    out.screen = ViewModel::Screen::History;
    char title[sizeof(out.modeName)];
    snprintf(title, sizeof(title), "LAST %u", static_cast<unsigned>(config::kHistoryScreenRows));
    copyText(out.modeName, title);

    for (size_t i = 0; i < history.size() && out.rowCount < config::kHistoryScreenRows; ++i) {
        const Measurement& m = history[i];
        if (m.intervalCount == 0) continue;
        HistoryRow& row = out.rows[out.rowCount++];
        snprintf(row.number, sizeof(row.number), "#%lu", static_cast<unsigned long>(m.sequence));

        const int s = m.find(IntervalKind::Split);
        if (s >= 0) {
            formatSeconds(m.durations[s], 2, row.split, sizeof(row.split));
            const Band* b =
                bandFor(IntervalKind::Split, m.seconds(static_cast<uint8_t>(s)), splitBands);
            row.splitColor = b ? b->color : 0;
        } else {
            copyText(row.split, "-");
        }

        const int h = m.find(IntervalKind::HogToHog);
        if (h >= 0) {
            formatSeconds(m.durations[h], 2, row.hogToHog, sizeof(row.hogToHog));
        } else {
            copyText(row.hogToHog, "-");
        }
    }
}

static_assert(kSplitLimitCount <= MenuView::kMaxLimits, "too many split limits for MenuView");

void fillMenu(ViewModel& out, const Menu& menu, const UserSettings& user, const History& history) {
    out.screen = ViewModel::Screen::Menu;
    out.gateCount = 0;
    out.modeName[0] = '\0';  // each menu page draws its own title
    MenuView& m = out.menu;
    m.page = menu.page();
    m.bandsScroll = static_cast<int16_t>(menu.bandsScroll());
    m.brightness = user.brightness;
    m.brightnessPercent = settings::brightnessPercent(user.brightness);
    m.limitCount = static_cast<uint8_t>(kSplitLimitCount);
    for (size_t i = 0; i < kSplitLimitCount; ++i) m.limitsCs[i] = user.splitLimitsCs[i];
    m.historyCount = static_cast<uint8_t>(history.size());
}

}  // namespace

const char* intervalName(IntervalKind kind) {
    switch (kind) {
        case IntervalKind::Split:
            return "SPLIT";
        case IntervalKind::HogToHog:
            return "HOG-HOG";
    }
    return "";
}

const Band* bandFor(IntervalKind kind, float seconds, const Band* splitBands) {
    switch (kind) {
        case IntervalKind::Split:
            return splitBands ? classify(splitBands, kSplitBandCount, seconds)
                              : classify(config::kSplitBands, seconds);
        case IntervalKind::HogToHog:
            return classify(config::kHogToHogBands, seconds);
    }
    return nullptr;
}

void buildView(const ViewInputs& in, ViewModel& out) {
    memset(&out, 0, sizeof(out));  // required for ViewModel::operator==

    const TimingProfile& p = in.profile;
    const bool multi = p.gateCount > 2;

    copyText(out.modeName, p.shortName);
    out.gateCount = p.gateCount;
    out.batteryPercent = in.batteryPercent;
    out.charging = in.charging;

    if (in.menu && in.user) {
        fillMenu(out, *in.menu, *in.user, in.history);
        return;
    }

    if (in.showHistory) {
        out.gateCount = 0;  // no progress dots on the history screen
        fillHistoryScreen(out, in.history, in.splitBands);
        return;
    }

    switch (in.timer.state()) {
        case SequenceTimer::State::Running: {
            out.screen = ViewModel::Screen::Running;
            const uint8_t done = in.timer.completedIntervals();
            out.gatesDone = done + 1;
            // Live value: tenths only -- hundredths flicker unreadably.
            fillReadout(out.main, in.timer.currentIntervalKind(),
                        in.timer.currentIntervalElapsed(in.now), 1, multi, false, in.splitBands);
            char hint[sizeof(out.hint)];
            snprintf(hint, sizeof(hint), "NEXT: %s", p.gateNames[out.gatesDone]);
            copyText(out.hint, hint);
            if (done > 0) {
                out.hasSecondary = true;
                fillReadout(out.secondary, p.intervals[0].kind, in.timer.completedDuration(0), 2,
                            true, true, in.splitBands);
            }
            fillHistory(out, in.history, 0);
            break;
        }

        case SequenceTimer::State::Finished: {
            out.screen = ViewModel::Screen::Result;
            const Measurement& m = in.timer.lastMeasurement();
            out.gatesDone = m.intervalCount + 1;
            if (m.intervalCount > 0) {
                const uint8_t last = m.intervalCount - 1;
                fillReadout(out.main, m.kinds[last], m.durations[last], 2, multi, true,
                            in.splitBands);
                if (m.intervalCount > 1) {
                    out.hasSecondary = true;
                    fillReadout(out.secondary, m.kinds[0], m.durations[0], 2, true, true,
                                in.splitBands);
                }
            }
            copyText(out.hint, m.complete ? "" : "TIMED OUT");
            // history[0] is this measurement (already on screen as `main`).
            const bool newestIsThis = !in.history.empty() && in.history[0].sequence == m.sequence;
            fillHistory(out, in.history, newestIsThis ? 1 : 0);
            break;
        }

        case SequenceTimer::State::Idle:
        default: {
            out.screen = ViewModel::Screen::Ready;
            copyText(out.main.time, "0.00");
            char hint[sizeof(out.hint)];
            snprintf(hint, sizeof(hint), "TAP AT %s", p.gateNames[0]);
            copyText(out.hint, in.timedOut ? "TIMED OUT" : hint);
            fillHistory(out, in.history, 0);
            break;
        }
    }
}

}  // namespace curl
