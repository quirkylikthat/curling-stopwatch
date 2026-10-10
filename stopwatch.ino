// Curling Timer for the M5Stack StopWatch (C152)
//
// This file stays deliberately thin. All code lives under src/:
//   src/core/  pure timing logic (unit-tested on a PC, no Arduino)
//   src/hal/   hardware access (buttons, haptics, battery, settings)
//   src/ui/    drawing on the round AMOLED
//   src/app/   glue: event handling + what to show
//   src/Config.h  every tunable in one place
//
// Board: Tools > Board > M5Stack > M5StopWatch. See README.md for setup.

#include "src/app/App.h"

curl::App app;

void setup() { app.begin(); }

void loop() { app.loop(); }
