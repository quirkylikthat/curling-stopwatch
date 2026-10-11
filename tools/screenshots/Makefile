# Renders every README screen to a PNG on a PC (no watch needed).
#
# Compiles the firmware's real src/ui/Renderer.cpp against M5GFX's software
# back end, with a tiny M5Unified stand-in (shim/M5Unified.h). Needs g++ and
# make (Linux / WSL / MSYS2 / macOS) and the M5GFX sources, which PlatformIO
# downloads into .pio/libdeps on the first firmware build. Elsewhere:
#   make M5GFX_DIR=/path/to/M5GFX
#
#   make -C tools/screenshots            build + write PNGs to docs/screenshots
#   make -C tools/screenshots OUT=dir    write somewhere else
#   make -C tools/screenshots SQUARE=1   keep the square frame (default: round, transparent corners)
#   make -C tools/screenshots clean

CXX ?= g++
CC  ?= gcc

ROOT      := ../..
SRC       := $(ROOT)/src
OUT       ?= $(ROOT)/docs/screenshots
BUILD     := $(ROOT)/build/screenshots
M5GFX_DIR ?= $(firstword $(wildcard $(ROOT)/.pio/libdeps/*/M5GFX))

ifeq ($(M5GFX_DIR),)
$(error M5GFX not found. Build the firmware once (pio run) or pass M5GFX_DIR=/path/to/M5GFX)
endif
LGFX := $(M5GFX_DIR)/src

# LGFX_LINUX_FB selects M5GFX's headless desktop back end (sprites only; no
# SDL needed).
DEFS     := -DLGFX_LINUX_FB
# -isystem: M5GFX headers don't get our -Werror.
INCLUDES := -isystem $(LGFX) -Ishim -I$(SRC)

# Our code: warnings are errors, as in the firmware build.
CXXFLAGS := -std=c++17 -O1 $(DEFS) $(INCLUDES) -Wall -Wextra -Werror -Wno-missing-field-initializers
# Library code: quiet.
LIBCXXFLAGS := -std=c++17 -O1 $(DEFS) -I$(LGFX) -w
LIBCFLAGS   := -O1 $(DEFS) -I$(LGFX) -w

OUR_SRC := $(SRC)/ui/Renderer.cpp $(wildcard $(SRC)/core/*.cpp) \
           $(addprefix $(SRC)/app/,ViewBuilder.cpp UserSettings.cpp Menu.cpp) main.cpp
OUR_OBJ := $(patsubst %.cpp,$(BUILD)/our/%.o,$(notdir $(OUR_SRC)))

LIB_CXX := $(wildcard $(LGFX)/lgfx/v1/*.cpp)
LIB_C   := $(wildcard $(LGFX)/lgfx/utility/*.c) $(wildcard $(LGFX)/lgfx/Fonts/*/*.c)
LIB_OBJ := $(patsubst %.cpp,$(BUILD)/lib/%.o,$(notdir $(LIB_CXX))) \
           $(patsubst %.c,$(BUILD)/lib/%.o,$(notdir $(LIB_C)))

BIN := $(BUILD)/screenshots

.PHONY: all run clean
all: run

run: $(BIN)
	@mkdir -p $(OUT)
	$(BIN) $(OUT) $(if $(SQUARE),--square)

$(BIN): $(OUR_OBJ) $(LIB_OBJ)
	$(CXX) -o $@ $^

# Our sources live in several directories; find each by name.
vpath %.cpp $(SRC)/ui $(SRC)/core $(SRC)/app .
vpath %.cpp $(LGFX)/lgfx/v1
vpath %.c $(LGFX)/lgfx/utility $(wildcard $(LGFX)/lgfx/Fonts/*)

$(BUILD)/our/%.o: %.cpp $(wildcard $(SRC)/*.h $(SRC)/*/*.h shim/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# The library objects are cached: they only rebuild if M5GFX changes.
$(BUILD)/lib/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(LIBCXXFLAGS) -c $< -o $@

$(BUILD)/lib/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(LIBCFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
