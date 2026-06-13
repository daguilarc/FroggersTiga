# GPL-3.0-or-later — see LICENSE
RACK_DIR ?= $(error Set RACK_DIR to your VCV Rack-SDK path, e.g. export RACK_DIR=~/Rack-SDK)

SLUG = FroggersTiga

FLAGS += -I../src/core -std=c++17

SOURCES += src/plugin.cpp

include $(RACK_DIR)/plugin.mk
