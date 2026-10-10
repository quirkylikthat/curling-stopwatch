# Host-side unit tests for the hardware-independent code (src/core, ViewBuilder).
#
# Needs any C++17 compiler: g++ (Linux / WSL / MSYS2) or clang++ (macOS).
#   make -C test          build + run
#   make -C test clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-missing-field-initializers

SRC_DIR  := ../src
SOURCES  := $(wildcard $(SRC_DIR)/core/*.cpp) $(SRC_DIR)/app/ViewBuilder.cpp $(wildcard *.cpp)
BUILD    := build
BIN      := $(BUILD)/run_tests

.PHONY: all test clean
all: test

$(BIN): $(SOURCES) $(wildcard *.h) $(wildcard $(SRC_DIR)/*.h $(SRC_DIR)/*/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

test: $(BIN)
	$(abspath $(BIN))

clean:
	rm -rf $(BUILD)
