# Entwickler-Anleitung - PDS-T1000-TSA24

## 🎯 Entwicklungsrichtlinien

### Code-Standards
- **C++**: C++17 Standard verwenden
- **Python**: PEP 8 Style Guide befolgen
- **C (ESP32)**: ESP-IDF Coding Style
- **Kommentare**: Deutsch für Projektspezifisches, Englisch für generische Funktionen

### Git-Workflow
```bash
# Feature-Branch erstellen
git checkout -b feature/neue-funktion

# Entwicklung mit kleinen Commits
git add .
git commit -m "feat: Neue Koordinatenfilterung hinzugefügt"

# Push und Pull Request
git push origin feature/neue-funktion
```

## 🏗️ Projekt-Architektur

### Modulübersicht

```
PDS-T1000-TSA24/
├── src/                 # C++ Hauptanwendung
│   ├── main.cpp        # Raylib Hauptschleife
│   ├── car_simulation/ # Fahrzeugphysik
│   ├── rendering/      # Grafik-Engine
│   ├── communication/ # UART/Netzwerk
│   └── vision/        # Computer Vision
├── include/           # Header-Dateien
├── esp_vehicle/      # ESP32 Firmware
│   ├── main/         # Hauptprogramm
│   ├── components/   # Custom-Komponenten
│   └── config/       # Konfiguration
├── external/         # Externe Bibliotheken
│   ├── raylib/      # Grafik-Engine
│   └── pybind11/    # Python-Integration
└── tools/           # Entwicklungstools
```

### Abhängigkeiten

#### C++ Dependencies
```cmake
# CMakeLists.txt
find_package(raylib REQUIRED)
find_package(pybind11 REQUIRED)

target_link_libraries(${PROJECT_NAME} 
    raylib 
    pybind11::embed
    ${Python3_LIBRARIES}
)
```

#### Python Dependencies
```txt
opencv-python==4.8.1.78
numpy==1.24.3
matplotlib==3.7.1
scipy==1.11.1
```

## 🛠️ Build-System

### CMake-Konfiguration
```cmake
cmake_minimum_required(VERSION 3.15)
project(PDS_Vehicle_System)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug-Konfiguration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
endif()

# Release-Konfiguration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()
```

### Build-Skripte

#### Windows (`build.bat`)
```batch
@echo off
echo Building PDS Vehicle System...

if not exist build mkdir build
cd build

cmake -G "MinGW Makefiles" ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

mingw32-make -j4
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
cd ..
```

## 💻 Entwicklungsumgebung

### VS Code Konfiguration

#### `.vscode/settings.json`
```json
{
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/include",
        "${workspaceFolder}/external/raylib/include",
        "${workspaceFolder}/external/pybind11/include"
    ],
    "C_Cpp.default.defines": [
        "DEBUG_BUILD=1"
    ],
    "python.defaultInterpreterPath": "./venv/Scripts/python.exe",
    "files.associations": {
        "*.h": "c",
        "*.hpp": "cpp"
    }
}
```

#### `.vscode/launch.json`
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Main Application",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/main.exe",
            "args": ["--debug"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "externalTerminal",
            "MIMode": "gdb",
            "miDebuggerPath": "C:/mingw64/bin/gdb.exe"
        },
        {
            "name": "Debug Python Vision",
            "type": "python",
            "request": "launch",
            "program": "${workspaceFolder}/src/Farberkennung.py",
            "console": "integratedTerminal"
        }
    ]
}
```

## 🧪 Testing-Framework

### Unit-Tests mit Catch2
```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "coordinate_filter.h"

TEST_CASE("Koordinatenfilter Grundfunktionalität", "[filter]") {
    CoordinateFilter filter;
    
    SECTION("Einzelne Koordinate verarbeiten") {
        Point input{100.0f, 200.0f};
        Point output = filter.process(input);
        
        REQUIRE(output.x == Approx(100.0f).epsilon(0.1f));
        REQUIRE(output.y == Approx(200.0f).epsilon(0.1f));
    }
    
    SECTION("Rauschen filtern") {
        std::vector<Point> noisy_data = {
            {100.0f, 100.0f},
            {101.0f, 99.0f},
            {99.0f, 101.0f},
            {100.5f, 100.2f}
        };
        
        for (const auto& point : noisy_data) {
            filter.process(point);
        }
        
        Point smoothed = filter.get_smoothed_position();
        REQUIRE(smoothed.x == Approx(100.1f).epsilon(0.5f));
        REQUIRE(smoothed.y == Approx(100.1f).epsilon(0.5f));
    }
}
```

### Integration-Tests
```cpp
TEST_CASE("UART Kommunikation", "[uart]") {
    MockUART uart;
    VehicleController controller(&uart);
    
    SECTION("Nachricht senden") {
        PositionData pos{125.5f, 89.2f, 45.0f};
        controller.send_position(pos);
        
        REQUIRE(uart.get_sent_message_count() == 1);
        REQUIRE(uart.get_last_message_type() == MSG_POSITION);
    }
    
    SECTION("Nachricht empfangen") {
        CommandData cmd{MOVE_FORWARD, 0.8f};
        uart.simulate_incoming_message(MSG_COMMAND, &cmd, sizeof(cmd));
        
        controller.process_messages();
        
        REQUIRE(controller.get_current_command().type == MOVE_FORWARD);
        REQUIRE(controller.get_current_command().value == Approx(0.8f));
    }
}
```

## 🐛 Debugging

### Debug-Makros
```cpp
#ifdef DEBUG_BUILD
    #define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define DEBUG_ASSERT(condition) assert(condition)
    #define DEBUG_BREAK() __debugbreak()
#else
    #define DEBUG_LOG(fmt, ...)
    #define DEBUG_ASSERT(condition)
    #define DEBUG_BREAK()
#endif
```

### Profiling
```cpp
class Profiler {
    std::chrono::high_resolution_clock::time_point start_time;
    std::string name;
    
public:
    Profiler(const std::string& function_name) : name(function_name) {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    ~Profiler() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        DEBUG_LOG("Profile [%s]: %lld μs", name.c_str(), duration.count());
    }
};

#define PROFILE(name) Profiler prof(name)
```

### Memory Leak Detection
```cpp
#ifdef DEBUG_BUILD
    #include <crtdbg.h>
    
    void enable_memory_leak_detection() {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
#endif
```

## 📊 Performance-Optimierung

### Rendering-Optimierungen
```cpp
// Objekt-Culling
bool is_visible(const RenderObject& obj, const Camera2D& camera) {
    Rectangle view_rect = get_camera_view_rectangle(camera);
    return CheckCollisionRecs(obj.bounds, view_rect);
}

// Batch-Rendering
class BatchRenderer {
    std::vector<Vertex> vertex_buffer;
    std::vector<unsigned int> index_buffer;
    
public:
    void add_quad(const Rectangle& rect, const Color& color) {
        // Vertices hinzufügen
    }
    
    void flush() {
        // Batch rendern
        rlDrawVertexArray(0, vertex_buffer.size());
        vertex_buffer.clear();
        index_buffer.clear();
    }
};
```

### Speicher-Pools
```cpp
template<typename T, size_t N>
class StaticPool {
    alignas(T) char storage[sizeof(T) * N];
    std::bitset<N> used;
    
public:
    T* allocate() {
        for (size_t i = 0; i < N; ++i) {
            if (!used[i]) {
                used[i] = true;
                return reinterpret_cast<T*>(&storage[i * sizeof(T)]);
            }
        }
        return nullptr;
    }
    
    void deallocate(T* ptr) {
        size_t index = (reinterpret_cast<char*>(ptr) - storage) / sizeof(T);
        if (index < N) {
            used[index] = false;
        }
    }
};
```

## 🔧 Utility-Funktionen

### Mathematik-Bibliothek
```cpp
namespace Math {
    constexpr float PI = 3.14159265359f;
    constexpr float DEG2RAD = PI / 180.0f;
    constexpr float RAD2DEG = 180.0f / PI;
    
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    Vector2 rotate_vector(Vector2 v, float angle) {
        float cos_a = cosf(angle);
        float sin_a = sinf(angle);
        return {
            v.x * cos_a - v.y * sin_a,
            v.x * sin_a + v.y * cos_a
        };
    }
    
    bool approximately_equal(float a, float b, float epsilon = 0.001f) {
        return fabsf(a - b) < epsilon;
    }
}
```

### Konfigurationssystem
```cpp
class Config {
    std::map<std::string, std::string> values;
    
public:
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);
                values[key] = value;
            }
        }
        return true;
    }
    
    template<typename T>
    T get(const std::string& key, T default_value = T{}) {
        auto it = values.find(key);
        if (it == values.end()) return default_value;
        
        if constexpr (std::is_same_v<T, int>) {
            return std::stoi(it->second);
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(it->second);
        } else if constexpr (std::is_same_v<T, bool>) {
            return it->second == "true" || it->second == "1";
        }
        return default_value;
    }
};
```

## 📚 Dokumentation

### Code-Dokumentation
```cpp
/**
 * @brief Filtert eingehende Koordinaten mit Kalman-Filter
 * 
 * Diese Klasse implementiert einen Kalman-Filter zur Glättung von
 * verrauschten Positionsdaten vom ESP32-Fahrzeug.
 * 
 * @example
 * CoordinateFilter filter;
 * Point raw_position = uart_receive_position();
 * Point filtered = filter.process(raw_position);
 */
class CoordinateFilter {
private:
    KalmanState state;          ///< Interner Kalman-Filter Zustand
    float process_noise;        ///< Prozessrauschen-Parameter
    float measurement_noise;    ///< Messrauschen-Parameter
    
public:
    /**
     * @brief Konstruktor mit Standard-Parametern
     * @param proc_noise Prozessrauschen (Standard: 0.01)
     * @param meas_noise Messrauschen (Standard: 0.1)
     */
    CoordinateFilter(float proc_noise = 0.01f, float meas_noise = 0.1f);
    
    /**
     * @brief Verarbeitet einen neuen Messpunkt
     * @param measurement Rohe Koordinaten vom Sensor
     * @return Gefilterte Koordinaten
     * @throws std::invalid_argument bei ungültigen Koordinaten
     */
    Point process(const Point& measurement);
};
```

### README-Template für neue Module
```markdown
# Modul-Name

## Übersicht
Kurze Beschreibung des Moduls und seiner Funktionalität.

## API-Referenz
### Klassen
- `ClassName` - Beschreibung

### Funktionen  
- `function_name()` - Beschreibung

## Verwendung
```cpp
// Code-Beispiel
```

## Tests
- Unit-Tests: `test_module.cpp`
- Integration-Tests: `integration_test.cpp`

## Abhängigkeiten
- Externe Bibliothek A
- Internes Modul B
```

## 🚀 Deployment

### Release-Build
```powershell
# Release-Build erstellen
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

# Executable optimieren
strip main.exe

# Distribution package erstellen
cpack
```

### Continuous Integration
```yaml
# .github/workflows/build.yml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup MinGW
      run: choco install mingw
      
    - name: Build Project
      run: |
        mkdir build
        cd build
        cmake -G "MinGW Makefiles" ..
        mingw32-make -j4
        
    - name: Run Tests
      run: |
        cd build
        ./tests.exe
```

---

**Maintainer**: Development Team TSA24  
**Coding Standards**: C++17, PEP 8  
**Letztes Update**: August 2025

**Beitragen**: Pull Requests willkommen! Bitte Tests hinzufügen und Dokumentation aktualisieren.
