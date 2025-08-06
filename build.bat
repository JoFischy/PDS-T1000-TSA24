@echo off
echo Building PDS-T1000-TSA24...

g++ -std=c++17 -Wall -Wextra -Iexternal/raylib/src -Iinclude -Isrc/pybind11/include -I"C:/Program Files/Python311/include" src/main.cpp src/py_runner.cpp src/car_simulation.cpp src/auto.cpp src/point.cpp src/renderer.cpp src/coordinate_filter.cpp -Lexternal/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -L"C:/Program Files/Python311/libs" -lpython311 -o main

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo ✅ main.exe ready
) else (
    echo ❌ Build failed!
    exit /b 1
)
