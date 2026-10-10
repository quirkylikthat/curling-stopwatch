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
                 bool withBand) {
    formatSeconds(duration, decimals, r.time, sizeof(r.time));
    copyText(r.caption, withCaption ? intervalName(kind) : "");
    if (withBand) {
        if (const Band* b = bandFor(kind, static_cast<float>(duration) / 1e6f)) {
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

}  // namespace

const char* intervalName(IntervalKind kind) {
    switch (kind) {
        case IntervalKind::Split: return "SPLIT";
        case IntervalKind::HogToHog: return "HOG-HOG";
    }
    return "";
}

const Band* bandFor(IntervalKind kind, float seconds) {
    switch (kind) {
        case IntervalKind::Split: return classify(config::kSplitBands, seconds);
        case IntervalKind::HogToHog: return classify(config::kHogToHogBands, seconds);
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

    switch (in.timer.state()) {
        case SequenceTimer::State::Running: {
            out.screen = ViewModel::Screen::Running;
            const uint8_t done = in.timer.completedIntervals();
            out.gatesDone = done + 1;
            // Live value: tenths only -- hundredths flicker unreadably.
            fillReadout(out.main, in.timer.currentIntervalKind(),
                        in.timer.currentIntervalElapsed(in.now), 1, multi, false);
            char hint[sizeof(out.hint)];
            snprintf(hint, sizeof(hint), "NEXT: %s", p.gateNames[out.gatesDone]);
            copyText(out.hint, hint);
            if (done > 0) {
                out.hasSecondary = true;
                fillReadout(out.secondary, p.intervals[0].kind, in.timer.completedDuration(0), 2, true,
                            true);
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
                fillReadout(out.main, m.kinds[last], m.durations[last], 2, multi, true);
                if (m.intervalCount > 1) {
                    out.hasSecondary = true;
                    fillReadout(out.secondary, m.kinds[0], m.durations[0], 2, true, true);
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
