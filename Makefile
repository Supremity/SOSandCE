# --- Project Configuration ---
# Based on your structure: source/SOSandCE_CPP/src
BASE_DIR  := source/SOSandCE_CPP
SRC_DIR   := $(BASE_DIR)/src
INC_DIR   := $(BASE_DIR)/include
BUILD_DIR := build

CXX       := g++
CXXFLAGS  := -std=c++17 -O3 -Wall -Wextra
TARGET    := sosandce

# --- Dependency Discovery (SDL2 family) ---
# pkg-config handles the flags for standard apt-installed libs
PKG_LIBS := sdl2 SDL2_image SDL2_mixer SDL2_ttf
CXXFLAGS += $(shell pkg-config --cflags $(PKG_LIBS))
LDFLAGS  := $(shell pkg-config --libs $(PKG_LIBS))

# --- RmlUi Configuration (Manual paths for your custom build) ---
# Point these to where you cloned and built RmlUi
RML_DIR      := $(HOME)/RmlUi
RML_INC      := -I$(RML_DIR)/Include
RML_LIB_PATH := -L$(RML_DIR)/Build
# Based on your 'ls' output: librmlui.so and librmlui_debugger.so
RML_LIBS     := -lrmlui -lrmlui_debugger

# --- Include Paths ---
INCLUDES := -I$(INC_DIR) -I$(INC_DIR)/imgui $(RML_INC)

# --- Source Discovery ---
# Finds all .cpp files in any subdirectory of src/
SOURCES := $(shell find $(SRC_DIR) -name "*.cpp")

# ImGui sources (located in your include folder)
IMGUI_SRC := $(INC_DIR)/imgui/imgui.cpp \
             $(INC_DIR)/imgui/imgui_draw.cpp \
             $(INC_DIR)/imgui/imgui_tables.cpp \
             $(INC_DIR)/imgui/imgui_widgets.cpp \
             $(INC_DIR)/imgui/imgui_impl_sdl2.cpp \
             $(INC_DIR)/imgui/imgui_impl_sdlrenderer2.cpp

ALL_SOURCES := $(SOURCES) $(IMGUI_SRC)

# --- Object Files ---
# This maps the .cpp file paths to .o files inside the build directory
OBJS := $(ALL_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# --- Build Rules ---

.PHONY: all clean run

all: $(TARGET)

# Linking the final binary
# -Wl,-rpath tells the program where to find the RmlUi .so files at runtime
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS) $(RML_LIB_PATH) $(RML_LIBS) -Wl,-rpath,$(RML_DIR)/Build

# Compiling C++ files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the application
run: all
	./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(TARGET)
