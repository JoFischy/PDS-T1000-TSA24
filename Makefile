
# Vereinfachtes Makefile für Raylib-Projekt mit Python-Integration
CC = g++
CFLAGS = -std=c++17 -Wall -Wextra
RAYLIB_PATH = external/raylib
INCLUDE_PATH = -I$(RAYLIB_PATH)/src -Iinclude
LIBRARY_PATH = -L$(RAYLIB_PATH)/src
LIBRARIES = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Python Integration (adjust paths as needed)
PYTHON_INCLUDE = -I/usr/include/python3.11 -Isrc/pybind11/include
PYTHON_LIBS = -lpython3.11

# Hauptprogramm
TARGET = main
SOURCES = src/main.cpp src/py_runner.cpp src/car_simulation.cpp

# Build-Regel
$(TARGET): $(SOURCES)
        $(CC) $(CFLAGS) $(INCLUDE_PATH) $(PYTHON_INCLUDE) $(SOURCES) $(LIBRARY_PATH) $(LIBRARIES) $(PYTHON_LIBS) -o $(TARGET)

# Farberkennung testen
test-farberkennung:
        python src/Farberkennung.py

# Hauptprogramm starten
run: $(TARGET)
        .\main.exe

# Cleanup
clean:
        del /f main.exe 2>nul || true

# Standard-Target
all: $(TARGET)

.PHONY: clean test-farberkennung run all
