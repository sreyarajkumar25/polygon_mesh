# Cross-platform build (Mac / Linux). Windows: use ComputerGraphicsLab.cbp in Code::Blocks.

CXX      ?= c++
CXXFLAGS ?= -std=c++11 -Wall -I.
LDFLAGS  ?=

UNAME_S := $(shell uname -s 2>/dev/null)

SRCS = main.cpp objects.cpp transformations.cpp
ifeq ($(UNAME_S),Darwin)
  SRCS += mac_drawable.mm
endif
TARGET = bin/ComputerGraphicsLab

ifeq ($(UNAME_S),Darwin)
  CXXFLAGS += -DGL_SILENCE_DEPRECATION
  LDFLAGS  += -framework OpenGL -framework GLUT -framework Cocoa
else ifeq ($(UNAME_S),Linux)
  CXXFLAGS += -DGL_SILENCE_DEPRECATION
  LDFLAGS  += -lGL -lGLU -lglut
else
  $(warning Unknown OS; try: make LDFLAGS="-framework OpenGL -framework GLUT")
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) objects.h transformations.h
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
