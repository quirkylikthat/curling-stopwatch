#include "Renderer.h"

#include <stdio.h>

#include "Layout.h"
#include "MenuLayout.h"
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

    if (vm.screen == ViewModel::Screen::Menu) {
        drawMenu(g, vm.menu);
        g.endWrite();
        present();
        return;
    }

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

    const layout::MidLines mid = layout::midLines(vm.main.band[0], vm.hasSecondary);
    if (mid.bandBaseline) {
        g.setFont(&fonts::FreeSansBold24pt7b);
        g.setTextColor(vm.main.bandColor);
        g.setTextDatum(baseline_center);
        g.drawString(vm.main.band, kCenter, mid.bandBaseline);
        g.setTextDatum(middle_center);
    }
}

void Renderer::drawSecondary(LovyanGFX& g, const ViewModel& vm) {
    if (!vm.hasSecondary) return;
    const layout::MidLines mid = layout::midLines(vm.main.band[0], true);
    char line[40];
    snprintf(line, sizeof(line), "%s %s %s", vm.secondary.caption, vm.secondary.time,
             vm.secondary.band);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(vm.secondary.band[0] ? vm.secondary.bandColor : kText);
    g.setTextDatum(baseline_center);
    g.drawString(line, kCenter, mid.secondaryBaseline);
    g.setTextDatum(middle_center);
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

// --- Menu ----------------------------------------------------------------------

namespace {

using menu_layout::Rect;

void title(LovyanGFX& g, const char* text, int y = menu_layout::kTitleY) {
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(kTextDim);
    g.setTextDatum(middle_center);
    g.drawString(text, kCenter, y);
}

void button(LovyanGFX& g, const Rect& r, const char* label, uint16_t fill = kButton) {
    g.fillRoundRect(r.left(), r.top(), r.w, r.h, r.h / 3, fill);
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextColor(kText);
    g.setTextDatum(middle_center);
    g.drawString(label, r.cx, r.cy);
}

void iconButton(LovyanGFX& g, const Rect& r, bool plus) {
    g.fillRoundRect(r.left(), r.top(), r.w, r.h, r.h / 3, kButton);
    g.fillRect(r.cx - 9, r.cy - 2, 18, 4, kText);  // "-"
    if (plus) g.fillRect(r.cx - 2, r.cy - 9, 4, 18, kText);
}

void cs(char* buf, size_t len, uint16_t centis) {
    snprintf(buf, len, "%u.%02u", centis / 100u, centis % 100u);
}

}  // namespace

void Renderer::drawMenu(LovyanGFX& g, const MenuView& m) {
    using namespace menu_layout;
    switch (m.page) {
        case MenuPage::Main:
            title(g, "MENU");
            button(g, kMainItems[0], "SETTINGS");
            button(g, kMainItems[1], "EXPORT");
            button(g, kMainItems[2], "CLEAR HISTORY");
            button(g, kBack, "BACK");
            break;

        case MenuPage::Settings:
            title(g, "SETTINGS");
            button(g, kSettingsItems[0], "BRIGHTNESS");
            button(g, kSettingsItems[1], "SPLIT BANDS");
            button(g, kBack, "BACK");
            break;

        case MenuPage::Brightness: {
            title(g, "BRIGHTNESS");
            char pct[8];
            snprintf(pct, sizeof(pct), "%u%%", static_cast<unsigned>(m.brightnessPercent));
            g.setFont(&fonts::FreeSansBold24pt7b);
            g.setTextColor(kText);
            g.drawString(pct, kCenter, kBrightnessValueY);
            const int knob = sliderX(m.brightness);
            g.fillRoundRect(kSliderX0, kSliderY - 4, kSliderX1 - kSliderX0, 8, 4, kTrack);
            g.fillRoundRect(kSliderX0, kSliderY - 4, knob - kSliderX0, 8, 4, kAccent);
            g.fillCircle(knob, kSliderY, kSliderKnobR, kText);
            button(g, kBack, "BACK");
            break;
        }

        case MenuPage::Bands: {
            title(g, "SPLIT BANDS", kBandsTitleY);
            g.setFont(&fonts::FreeSans9pt7b);
            g.setTextColor(kTextFaint);
            g.drawString("UP TO (S)", kBandValueX, kBandHeaderY);
            char value[8];
            for (uint8_t row = 0; row < m.limitCount; ++row) {
                const int y = bandRowY(row);
                g.setFont(&fonts::FreeSansBold12pt7b);
                g.setTextDatum(middle_left);
                g.setTextColor(config::kSplitBands[row].color);
                g.drawString(config::kSplitBands[row].label, kBandLabelX, y);
                g.setTextDatum(middle_center);
                g.setTextColor(kText);
                cs(value, sizeof(value), m.limitsCs[row]);
                g.drawString(value, kBandValueX, y);
                iconButton(g, bandMinus(row), false);
                iconButton(g, bandPlus(row), true);
            }
            const int shiftY = bandRowY(m.limitCount);
            g.setFont(&fonts::FreeSansBold12pt7b);
            g.setTextDatum(middle_left);
            g.setTextColor(kTextDim);
            g.drawString("SHIFT ALL", kBandLabelX, shiftY);
            g.setTextDatum(middle_center);
            iconButton(g, bandMinus(m.limitCount), false);
            iconButton(g, bandPlus(m.limitCount), true);
            button(g, kBandsReset, "RESET");
            button(g, kBandsBack, "BACK");
            break;
        }

        case MenuPage::Export:
            title(g, "EXPORT");
            g.setFont(&fonts::FreeSans12pt7b);
            g.setTextColor(kTextDim);
            g.drawString("COMING SOON", kCenter, kCenter);
            button(g, kBack, "BACK");
            break;

        case MenuPage::ConfirmClear: {
            title(g, "CLEAR HISTORY?");
            char line[32];
            snprintf(line, sizeof(line), "%u RESULTS WILL BE",
                     static_cast<unsigned>(m.historyCount));
            g.setFont(&fonts::FreeSans12pt7b);
            g.setTextColor(kText);
            g.drawString(line, kCenter, kConfirmTextY);
            g.drawString("DELETED", kCenter, kConfirmTextY + 32);
            button(g, kConfirmCancel, "CANCEL");
            button(g, kConfirmClear, "CLEAR", kButtonDanger);
            break;
        }
    }
    g.setTextDatum(middle_center);
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
