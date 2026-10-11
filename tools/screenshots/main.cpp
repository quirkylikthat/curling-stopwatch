// Screenshot generator: renders every screen the README describes to a PNG,
// on a PC, with the firmware's own Renderer.
//
// What is real and what is not:
//   - Real: ui/Renderer.cpp (compiled unchanged), app/ViewBuilder, app/Menu,
//     app/UserSettings and the core timing logic. Each scene is produced by
//     driving those with the same taps the watch would see, so a screenshot is
//     what the firmware would draw for that state.
//   - Not real: the panel. This is the 466x466 framebuffer, not a photo of the
//     AMOLED, so colour and brightness differ from the device.
//
// Usage: screenshots <output-dir> [--square]   (see tools/screenshots/Makefile)
// Output is round (transparent corners, like the glass) unless --square is given.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include "app/Menu.h"
#include "app/UserSettings.h"
#include "app/ViewBuilder.h"
#include "ui/MenuLayout.h"
#include "ui/Renderer.h"

using namespace curl;

namespace {

constexpr Micros s(double seconds) { return static_cast<Micros>(seconds * 1e6 + 0.5); }

constexpr uint8_t kSplit = 0, kHogHog = 1, kBoth = 2;  // indexes into config::kProfiles

// A scripted watch: a timer, a history and a virtual clock.
struct Sim {
    SequenceTimer timer{config::kMinTapGapUs};
    History history;
    uint8_t profile = kSplit;
    Micros clock = s(100);
    int8_t battery = 80;
    bool charging = false;

    void select(uint8_t p) {
        profile = p;
        timer.setProfile(config::kProfiles[p], p);
    }

    // One complete throw, as timed by taps; stored in the history like the app does.
    void throwStone(uint8_t p, double first, double second = 0) {
        select(p);
        const double t0 = static_cast<double>(clock) / 1e6;
        timer.onTap(s(t0));
        timer.onTap(s(t0 + first));
        if (second > 0) timer.onTap(s(t0 + first + second));
        history.push(timer.takeMeasurement());
        clock = s(t0 + first + second + 25);
    }

    // The three earlier throws shown along the bottom of the timing screen.
    void seed(uint8_t p) {
        switch (p) {
            case kSplit:
                throwStone(kSplit, 3.95);
                throwStone(kSplit, 3.60);
                throwStone(kSplit, 4.10);
                break;
            case kHogHog:
                throwStone(kHogHog, 14.31);
                throwStone(kHogHog, 14.05);
                throwStone(kHogHog, 14.22);
                break;
            default:
                throwStone(kBoth, 3.95, 14.31);
                throwStone(kBoth, 3.60, 14.05);
                throwStone(kBoth, 4.10, 14.22);
                break;
        }
        timer.cancel();  // back to Idle (ready), history kept
    }

    ViewModel view(bool timedOut = false, bool showHistory = false, const Menu* menu = nullptr,
                   const UserSettings* user = nullptr) const {
        ViewModel vm;
        buildView({timer, config::kProfiles[profile], history, clock, battery, charging, timedOut,
                   showHistory, nullptr, menu, user},
                  vm);
        return vm;
    }
};

struct Scene {
    std::string file;     // without .png
    std::string feature;  // README section it illustrates
    std::string caption;
    bool splash;
    ViewModel vm;
};

std::vector<Scene> gScenes;

void add(const char* file, const char* feature, const char* caption, const ViewModel& vm) {
    gScenes.push_back({file, feature, caption, false, vm});
}

void addSplash(const char* file, const char* feature, const char* caption) {
    Scene sc{file, feature, caption, true, {}};
    gScenes.push_back(sc);
}

// ---- Scenes ---------------------------------------------------------------------

void timingModes() {
    const char* feat = "Timing modes / The timing screen";
    struct Mode {
        uint8_t p;
        const char* key;
        const char* name;
    } modes[] = {
        {kSplit, "split", "SPLIT"}, {kHogHog, "hogHog", "HOG-HOG"}, {kBoth, "both", "SPLIT+H2H"}};

    for (const Mode& m : modes) {
        std::string k = m.key;
        {  // Ready: prompt, empty dots, last 3 results.
            Sim sim;
            sim.seed(m.p);
            add((k + "-ready").c_str(), feat,
                (std::string(m.name) + ": ready, waiting for the first tap").c_str(), sim.view());
        }
        {  // Running: live value in tenths.
            Sim sim;
            sim.seed(m.p);
            sim.select(m.p);
            const double t0 = static_cast<double>(sim.clock) / 1e6;
            sim.timer.onTap(s(t0));
            const double live = m.p == kHogHog ? 8.45 : 2.37;
            sim.clock = s(t0 + live);
            add((k + "-running").c_str(), feat,
                (std::string(m.name) + ": timing, live value").c_str(), sim.view());
            if (m.p == kBoth) {  // near hog tapped: hog-to-hog runs, the split sits underneath
                sim.timer.onTap(s(t0 + 3.80));
                sim.clock = s(t0 + 3.80 + 5.05);
                add("both-running-h2h", feat,
                    "SPLIT+H2H: near hog tapped; hog-to-hog running, split shown below with its "
                    "band",
                    sim.view());
            }
        }
        {  // Result: final time in hundredths, weight band for splits.
            Sim sim;
            sim.seed(m.p);
            if (m.p == kSplit) sim.throwStone(kSplit, 3.82);
            if (m.p == kHogHog) sim.throwStone(kHogHog, 14.31);
            if (m.p == kBoth) sim.throwStone(kBoth, 3.95, 14.12);
            add((k + "-result").c_str(), feat, (std::string(m.name) + ": result").c_str(),
                sim.view());
        }
    }

    {  // No second tap in time.
        Sim sim;
        sim.seed(kSplit);
        add("timed-out", feat, "Prompt after a run timed out with nothing captured",
            sim.view(/*timedOut=*/true));
    }
    {  // Timed out after the split in SPLIT+H2H: the split is kept.
        Sim sim;
        sim.seed(kBoth);
        sim.select(kBoth);
        const double t0 = static_cast<double>(sim.clock) / 1e6;
        sim.timer.onTap(s(t0));
        sim.timer.onTap(s(t0 + 3.90));
        sim.timer.tick(s(t0 + 3.90 + 40));
        sim.history.push(sim.timer.takeMeasurement());
        sim.clock = s(t0 + 50);
        add("both-timed-out-after-split", feat,
            "SPLIT+H2H: far hog never tapped; the split is kept, marked TIMED OUT", sim.view());
    }
}

void weightBands() {
    const char* feat = "Weight bands";
    struct B {
        double t;
        const char* key;
    } bands[] = {{2.80, "peel"}, {3.10, "takeout"}, {3.40, "board"},
                 {3.60, "hack"}, {3.95, "draw"},    {4.50, "guard"}};
    for (const B& b : bands) {
        Sim sim;
        sim.seed(kSplit);
        sim.throwStone(kSplit, b.t);
        char cap[64];
        snprintf(cap, sizeof(cap), "Split of %.2f s -> %s", b.t, b.key);
        add((std::string("band-") + b.key).c_str(), feat, cap, sim.view());
    }
}

void battery() {
    const char* feat = "The timing screen (battery arc)";
    struct Bat {
        int8_t pct;
        bool charging;
        const char* key;
        const char* cap;
    } cases[] = {{80, false, "battery-ok", "Battery 80%: green arc"},
                 {12, false, "battery-low", "Battery 12% (15% or less): red arc"},
                 {55, true, "battery-charging", "Charging: blue arc"}};
    for (const Bat& c : cases) {
        Sim sim;
        sim.seed(kSplit);
        sim.battery = c.pct;
        sim.charging = c.charging;
        add(c.key, feat, c.cap, sim.view());
    }
}

void historyScreen() {
    const char* feat = "History screen";
    {
        Sim sim;
        // 12 throws: only the newest 10 are shown; split-only, hog-only and both.
        sim.throwStone(kSplit, 4.30);
        sim.throwStone(kHogHog, 13.80);
        sim.throwStone(kSplit, 3.18);
        sim.throwStone(kBoth, 3.95, 14.31);
        sim.throwStone(kSplit, 3.40);
        sim.throwStone(kHogHog, 14.05);
        sim.throwStone(kBoth, 3.60, 14.22);
        sim.throwStone(kSplit, 2.85);
        sim.throwStone(kBoth, 4.10, 14.31);
        sim.throwStone(kSplit, 3.82);
        sim.throwStone(kHogHog, 14.31);
        sim.throwStone(kBoth, 3.95, 14.12);
        sim.timer.cancel();
        add("history-full", feat,
            "Last 10 throws, newest first; split coloured by band, '-' where a throw had no "
            "value, #n matches the USB log",
            sim.view(false, true));
    }
    {
        Sim sim;
        add("history-empty", feat, "No throws yet", sim.view(false, true));
    }
}

void menuScenes() {
    const char* feat = "Menu";
    using namespace menu_layout;
    UserSettings user = settings::defaults();
    Sim sim;
    for (int i = 0; i < 7; ++i) sim.throwStone(kSplit, 3.5 + i * 0.1);
    sim.timer.cancel();

    Menu menu;
    menu.open();
    add("menu-main", feat, "Menu (blue hold 2 s)", sim.view(false, false, &menu, &user));

    menu.tap(kMainItems[1].cx, kMainItems[1].cy, user);  // Export
    add("menu-export", feat, "Export (placeholder)", sim.view(false, false, &menu, &user));
    menu.back();

    menu.tap(kMainItems[2].cx, kMainItems[2].cy, user);  // Clear history
    add("menu-clear-confirm", feat, "Clear history asks first",
        sim.view(false, false, &menu, &user));
    menu.back();

    menu.tap(kMainItems[0].cx, kMainItems[0].cy, user);  // Settings
    add("menu-settings", feat, "Settings", sim.view(false, false, &menu, &user));

    menu.tap(kSettingsItems[0].cx, kSettingsItems[0].cy, user);  // Brightness
    settings::setBrightness(user, 128);
    add("menu-brightness", feat, "Settings > Brightness: slider at 50%",
        sim.view(false, false, &menu, &user));
    menu.back();

    menu.tap(kSettingsItems[1].cx, kSettingsItems[1].cy, user);  // Split bands
    add("menu-bands-top", feat, "Settings > Split bands: top of the list",
        sim.view(false, false, &menu, &user));

    // Drag the list up (finger moves up the screen) to reach SHIFT ALL.
    const int midY = (kBandViewTop + kBandViewBottom) / 2;
    menu.down(kCenter, midY, user);
    menu.drag(kCenter, midY - kBandMaxScroll - 40, user);
    add("menu-bands-bottom", feat, "Split bands scrolled down to SHIFT ALL",
        sim.view(false, false, &menu, &user));
    if (menu.bandsScroll() != kBandMaxScroll) {
        fprintf(stderr, "warning: bands list scrolled to %d, expected %d\n", menu.bandsScroll(),
                kBandMaxScroll);
    }
}

void buildScenes() {
    addSplash("splash", "Boot", "Boot splash");
    timingModes();
    weightBands();
    battery();
    historyScreen();
    menuScenes();
}

// ---- Output ---------------------------------------------------------------------

// miniz's PNG encoder, bundled with M5GFX (declared only in its .c file).
extern "C" void* tdefl_write_image_to_png_file_in_memory_ex(const void* pImage, int w, int h,
                                                            int num_chans, size_t* pLen_out,
                                                            unsigned level, int flip);

// Saves the framebuffer as an RGBA PNG. `round`: pixels outside the circular glass
// become transparent, with a one-pixel anti-aliased edge.
bool writePng(const std::string& path, bool round) {
    const int w = M5.Display.width();
    const int h = M5.Display.height();
    std::vector<uint8_t> rgb(static_cast<size_t>(w) * 3);
    std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);
    const float r = w / 2.0f;
    for (int y = 0; y < h; ++y) {
        M5.Display.readRectRGB(0, y, w, 1, rgb.data());
        for (int x = 0; x < w; ++x) {
            uint8_t* px = &rgba[(static_cast<size_t>(y) * w + x) * 4];
            px[0] = rgb[x * 3];
            px[1] = rgb[x * 3 + 1];
            px[2] = rgb[x * 3 + 2];
            float coverage = 1.0f;
            if (round) {
                const float dx = x + 0.5f - r;
                const float dy = y + 0.5f - r;
                coverage = r - sqrtf(dx * dx + dy * dy) + 0.5f;  // 1 px edge ramp
                coverage = coverage < 0 ? 0 : (coverage > 1 ? 1 : coverage);
            }
            px[3] = static_cast<uint8_t>(coverage * 255.0f + 0.5f);
        }
    }

    size_t len = 0;
    void* png = tdefl_write_image_to_png_file_in_memory_ex(rgba.data(), w, h, 4, &len, 6, 0);
    if (!png || len == 0) return false;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        lgfx::heap_free(png);
        return false;
    }
    const bool ok = fwrite(png, 1, len, f) == len;
    fclose(f);
    lgfx::heap_free(png);
    return ok;
}

bool writeIndex(const std::string& dir, bool round) {
    FILE* f = fopen((dir + "/index.md").c_str(), "w");
    if (!f) return false;
    fprintf(f,
            "# Screenshots\n\n"
            "Generated by `make -C tools/screenshots`; do not edit by hand. Each image is the "
            "466x466 framebuffer the firmware's `Renderer` draws for that state, not a photo of "
            "the AMOLED.%s\n\n",
            round ? " The corners are transparent, like the round glass." : "");
    std::string last;
    for (const Scene& sc : gScenes) {
        if (sc.feature != last) {
            fprintf(f, "\n## %s\n\n", sc.feature.c_str());
            last = sc.feature;
        }
        fprintf(f, "![%s](%s.png)  \n`%s.png`: %s\n\n", sc.caption.c_str(), sc.file.c_str(),
                sc.file.c_str(), sc.caption.c_str());
    }
    fclose(f);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    const bool square = argc == 3 && std::string(argv[2]) == "--square";
    if (argc != 2 && !square) {
        fprintf(stderr, "usage: %s <output-dir> [--square]\n", argv[0]);
        return 2;
    }
    const std::string dir = argv[1];
    const bool round = !square;

    buildScenes();

    Renderer renderer;
    renderer.begin();

    int failures = 0;
    for (const Scene& sc : gScenes) {
        if (sc.splash) {
            renderer.splash(config::kFirmwareVersion);
        } else {
            renderer.render(sc.vm);
        }
        const std::string path = dir + "/" + sc.file + ".png";
        if (!writePng(path, round)) {
            fprintf(stderr, "failed to write %s\n", path.c_str());
            ++failures;
        }
    }
    if (!writeIndex(dir, round)) ++failures;

    printf("%zu screenshots -> %s%s\n", gScenes.size(), dir.c_str(),
           failures ? " (with errors)" : "");
    return failures ? 1 : 0;
}
