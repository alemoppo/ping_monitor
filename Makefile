CC = gcc
CXX = g++

TARGET = bin\ping_monitor.exe

VENDOR_DIR = vendor
SDL2_DIR = $(VENDOR_DIR)\SDL2-2.32.10\x86_64-w64-mingw32
IMGUI_DIR = $(VENDOR_DIR)\imgui

IMGUI_SOURCES = \
    $(IMGUI_DIR)\imgui.cpp \
    $(IMGUI_DIR)\imgui_draw.cpp \
    $(IMGUI_DIR)\imgui_tables.cpp \
    $(IMGUI_DIR)\imgui_widgets.cpp \
    $(IMGUI_DIR)\backends\imgui_impl_sdl2.cpp \
    $(IMGUI_DIR)\backends\imgui_impl_opengl3.cpp

SRC_DIR = src
SRC = $(filter-out $(SRC_DIR)/icon_inject.c, $(wildcard $(SRC_DIR)/*.c))

CFLAGS = -Wall -Wextra -O2 \
    -I$(IMGUI_DIR) \
    -I$(IMGUI_DIR)/backends \
    -I$(SDL2_DIR)/include/SDL2 \
    -I$(SRC_DIR)

CXXFLAGS = $(CFLAGS)

LDFLAGS = -L$(SDL2_DIR)/lib \
    -Wl,-subsystem,windows \
    -lmingw32 -lSDL2main -lSDL2 \
    -lopengl32 -lgdi32 -limm32 -lsetupapi

CPP = "C:\Program Files\mingw64\bin\cpp.exe"
WINDRES = "C:\Program Files\mingw64\bin\windres.exe"

all: $(TARGET)

$(TARGET): $(SRC) $(IMGUI_SOURCES) Makefile src\icon_data.h ico.png
	@if not exist bin mkdir bin
	@copy /Y $(SDL2_DIR)\bin\SDL2.dll bin\SDL2.dll >nul 2>&1
	$(CXX) -o $@ $(SRC) $(IMGUI_SOURCES) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

run: $(TARGET)
	$(TARGET)

clean:
	@if exist bin del /F /Q $(TARGET)
	@if exist bin del /F /Q bin\SDL2.dll
	@if exist app.res del /F /Q app.res
	@if exist icon_inject.exe del /F /Q icon_inject.exe
	@if exist bin rmdir /S /Q bin

.PHONY: all clean run
