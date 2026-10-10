#include "Renderer.h"

#include <stdio.h>

#include "Theme.h"

namespace curl {

using namespace theme;

bool Renderer::begin() {
    canvas_.setColorDepth(16);
    canvas_.setPsram(true);
    useCanvas_ = canvas_.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
    if (!useCanvas_) Serial.println("[ui] canvas alloc failed; drawing direct (may flicker)");
    return useCanvas_;
}

LovyanGFX* Renderer::target() {
    if (useCanvas_) return &canvas_;
    return &M5.Display;
}

void Renderer::present() {
    if (useCanvas_) canvas_.pushSprite(&M5.Display, 0, 0);
}

void Renderer::splash(const char* version) {
    LovyanGFX& g = *target();
    g.startWrite();
    g.fillScreen(kBackground);
    g.setTextDatum(middle_center);
    g.setTextSize(1);
    g.setFont(&fonts::FreeSansBold24pt7b);
    g.setTextColor(kAccent);
    g.drawString("CURLING", kCenter, kCenter - 40);
    g.setTextColor(kText);
    g.drawString("TIMER", kCenter, kCenter + 10);
    g.setFont(&fonts::FreeSans12pt7b);
    g.setTextColor(kTextDim);
    g.drawString(version, kCenter, kCenter + 70);
    g.endWrite();
    present();
}

void Renderer::render(const ViewModel& vm) {
    LovyanGFX& g = *target();
    g.startWrite();
    g.fillScreen(kBackground);
    g.setTextDatum(middle_center);
    g.setTextSize(1);

    drawBattery(g, vm);

    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(kTextDim);
    g.drawString(vm.modeName, kCenter, kModeY);

    if (vm.screen == ViewModel::Screen::History) {
        drawHistoryScreen(g, vm);
        g.endWrite();
        present();
        return;
    }

    drawDots(g, vm);

    // Hint line doubles as the caption for the main readout when there is no hint.
    const char* line = vm.hint[0] ? vm.hint : vm.main.caption;
    if (line[0]) {
        g.setFont(&fonts::FreeSans12pt7b);
        g.setTextColor(strcmp(vm.hint, "TIMED OUT") == 0 ? kWarn : kTextDim);
        g.drawString(line, kCenter, kHintY);
    }

    drawMain(g, vm);
    drawSecondary(g, vm);
    drawHistory(g, vm);

    g.endWrite();
    present();
}

void Renderer::drawBattery(LovyanGFX& g, const ViewModel& vm) {
    g.fillArc(kCenter, kCenter, kBatteryR0, kBatteryR1, kBatteryArcStart,
              kBatteryArcStart + kBatteryArcSpan, kTrack);
    if (vm.batteryPercent < 0) return;
    const float span = kBatteryArcSpan * vm.batteryPercent / 100.0f;
    const uint16_t color =
        vm.charging ? kBattCharging : (vm.batteryPercent <= 15 ? kBattLow : kBattOk);
    if (span > 0.5f) {
        g.fillArc(kCenter, kCenter, kBatteryR0, kBatteryR1, kBatteryArcStart,
                  kBatteryArcStart + span, color);
    }
}

void Renderer::drawDots(LovyanGFX& g, const ViewModel& vm) {
    constexpr int kSpacing = 24;
    constexpr int kRadius = 6;
    const int x0 = kCenter - (vm.gateCount - 1) * kSpacing / 2;
    for (uint8_t i = 0; i < vm.gateCount; ++i) {
        const int x = x0 + i * kSpacing;
        if (i < vm.gatesDone) {
            g.fillCircle(x, kDotsY, kRadius,
                         vm.screen == ViewModel::Screen::Running ? kRunning : kAccent);
        } else {
            g.drawCircle(x, kDotsY, kRadius, kTextFaint);
        }
    }
}

void Renderer::drawMain(LovyanGFX& g, const ViewModel& vm) {
    g.setFont(&fonts::Font7);  // 7-segment digits: '0'-'9', '.', ':', '-'
    g.setTextSize(kMainTextSize);
    uint16_t color = kText;
    if (vm.screen == ViewModel::Screen::Running) color = kRunning;
    if (vm.screen == ViewModel::Screen::Ready) color = kTextFaint;
    g.setTextColor(color);
    g.drawString(vm.main.time, kCenter, kMainY);
    g.setTextSize(1);

    if (vm.main.band[0]) {
        g.setFont(&fonts::FreeSansBold24pt7b);
        g.setTextColor(vm.main.bandColor);
        g.drawString(vm.main.band, kCenter, kBandY);
    }
}

void Renderer::drawSecondary(LovyanGFX& g, const ViewModel& vm) {
    if (!vm.hasSecondary) return;
    char line[40];
    snprintf(line, sizeof(line), "%s %s %s", vm.secondary.caption, vm.secondary.time,
             vm.secondary.band);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(vm.secondary.band[0] ? vm.secondary.bandColor : kText);
    g.drawString(line, kCenter, kSecondaryY);
}

void Renderer::drawHistory(LovyanGFX& g, const ViewModel& vm) {
    if (vm.historyCount == 0) return;
    char line[48] = "";
    size_t used = 0;
    for (uint8_t i = 0; i < vm.historyCount; ++i) {
        used += snprintf(line + used, sizeof(line) - used, i ? "   %s" : "%s", vm.history[i]);
        if (used >= sizeof(line)) break;
    }
    g.setFont(&fonts::FreeSans12pt7b);
    g.setTextColor(kTextDim);
    g.drawString(line, kCenter, kHistoryY);
}

void Renderer::drawHistoryScreen(LovyanGFX& g, const ViewModel& vm) {
    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(kTextDim);
    g.setTextDatum(middle_center);
    g.drawString("SPLIT", kHistSplitX, kHistHeaderY);
    g.drawString("HOG-HOG", kHistHogX, kHistHeaderY);
    g.drawFastHLine(kCenter - 150, kHistHeaderY + 14, 300, kTrack);

    if (vm.rowCount == 0) {
        g.setFont(&fonts::FreeSans12pt7b);
        g.drawString("NO THROWS YET", kCenter, kCenter);
    }

    for (uint8_t i = 0; i < vm.rowCount; ++i) {
        const HistoryRow& r = vm.rows[i];
        const int y = kHistFirstRowY + i * kHistRowStep;

        g.setFont(&fonts::FreeSans9pt7b);
        g.setTextColor(kTextFaint);
        g.setTextDatum(middle_right);
        g.drawString(r.number, kHistNumberX, y);

        g.setFont(&fonts::FreeSansBold12pt7b);
        g.setTextDatum(middle_center);
        g.setTextColor(r.splitColor ? r.splitColor : kTextFaint);
        g.drawString(r.split, kHistSplitX, y);
        g.setTextColor(r.hogToHog[0] == '-' ? kTextFaint : kText);
        g.drawString(r.hogToHog, kHistHogX, y);
    }

    g.setFont(&fonts::FreeSans9pt7b);
    g.setTextColor(kTextFaint);
    g.setTextDatum(middle_center);
    g.drawString("BLUE: BACK", kCenter, kHistFooterY);
}

}  // namespace curl
