# Technische Dokumentation - PDS-T1000-TSA24

## 🔧 Technische Spezifikationen

### System-Anforderungen

#### Hardware
- **ESP32**: DevKit v1 oder äquivalent
- **PC**: Windows 10/11, 4GB RAM, DirectX 11
- **USB-Kabel**: Für ESP32-Verbindung
- **Sensoren**: Ultraschall HC-SR04, ESP32-CAM

#### Software
- **Compiler**: GCC 9.0+ oder MSVC 2019+
- **Python**: 3.8 - 3.11
- **ESP-IDF**: v4.4+
- **CMake**: 3.15+

## 🏗️ Architektur-Details

### Kommunikationsprotokoll

#### UART-Nachrichten
```
Format: [HEADER][LENGTH][DATA][CRC]
- HEADER: 2 Bytes (0xAA, 0x55)
- LENGTH: 1 Byte (Datenlänge)
- DATA: Variable Länge
- CRC: 1 Byte Checksumme
```

#### Nachrichtentypen
```cpp
#define MSG_POSITION    0x01  // Positionsdaten
#define MSG_SENSOR      0x02  // Sensordaten
#define MSG_COMMAND     0x03  // Fahrbefehle
#define MSG_STATUS      0x04  // Systemstatus
```

### Koordinaten-System

#### Koordinatenfilter-Algorithmus
```
Kalman-Filter Implementation:
- Zustandsvektor: [x, y, vx, vy]
- Messrauschen: σ² = 0.1m
- Prozessrauschen: σ² = 0.01m/s²
- Update-Rate: 50Hz
```

#### Datenformat
```json
{
  "timestamp": "2025-08-06T10:30:45.123Z",
  "coordinates": {
    "x": 1250.5,
    "y": 890.2,
    "heading": 45.7,
    "velocity": 0.8
  },
  "filtered": true,
  "confidence": 0.95
}
```

## 🖥️ Software-Module

### 1. Hauptanwendung (`main.cpp`)

#### Initialisierung
```cpp
// Raylib-Fenster erstellen
InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "PDS Vehicle Simulator");
SetTargetFPS(60);

// UART-Verbindung initialisieren
uart_init(UART_PORT, BAUD_RATE);

// Python-Engine starten
py_runner_init();
```

#### Hauptschleife
```cpp
while (!WindowShouldClose()) {
    // Input verarbeiten
    handle_input();
    
    // ESP32-Daten empfangen
    receive_uart_data();
    
    // Simulation aktualisieren
    update_simulation(GetFrameTime());
    
    // Koordinaten filtern
    filter_coordinates();
    
    // Rendering
    BeginDrawing();
    render_scene();
    EndDrawing();
}
```

### 2. Fahrzeugsimulation (`car_simulation.cpp`)

#### Physik-Engine
```cpp
class VehiclePhysics {
    Vector2 position;
    Vector2 velocity;
    float heading;
    float angular_velocity;
    
    void update(float deltaTime) {
        // Kinematisches Modell
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        heading += angular_velocity * deltaTime;
        
        // Reibung anwenden
        velocity = Vector2Scale(velocity, FRICTION_FACTOR);
    }
};
```

#### Kollisionserkennung
```cpp
bool check_collision(Rectangle vehicle, Rectangle obstacle) {
    return CheckCollisionRecs(vehicle, obstacle);
}

void handle_collision_response(Vector2& velocity, Vector2 normal) {
    // Elastischer Stoß
    velocity = Vector2Reflect(velocity, normal);
    velocity = Vector2Scale(velocity, RESTITUTION);
}
```

### 3. Koordinatenfilter (`coordinate_filter.cpp`)

#### Kalman-Filter Implementation
```cpp
class KalmanFilter {
    Matrix4 state;        // [x, y, vx, vy]
    Matrix4 covariance;   // Fehlerkovarianz
    Matrix4 process_noise;
    Matrix2 measurement_noise;
    
public:
    void predict(float dt);
    void update(Vector2 measurement);
    Vector2 get_position();
};
```

#### Outlier-Detection
```cpp
bool is_outlier(Vector2 measurement, Vector2 predicted) {
    float distance = Vector2Distance(measurement, predicted);
    return distance > OUTLIER_THRESHOLD;
}
```

### 4. Bilderkennung (`Farberkennung.py`)

#### OpenCV-Pipeline
```python
def detect_objects(frame):
    # Farbfilterung
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    mask = cv2.inRange(hsv, lower_color, upper_color)
    
    # Konturen finden
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # Objekte klassifizieren
    objects = []
    for contour in contours:
        if cv2.contourArea(contour) > MIN_AREA:
            x, y, w, h = cv2.boundingRect(contour)
            objects.append({
                'type': classify_object(contour),
                'position': (x + w//2, y + h//2),
                'confidence': calculate_confidence(contour)
            })
    
    return objects
```

## 🔌 ESP32-Firmware

### Hauptkomponenten

#### UART-Handler
```c
void uart_task(void *params) {
    uint8_t buffer[UART_BUFFER_SIZE];
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, buffer, sizeof(buffer), 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            process_uart_message(buffer, len);
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
```

#### Sensor-Handler
```c
void sensor_task(void *params) {
    sensor_data_t data;
    
    while (1) {
        // Ultraschall lesen
        data.distance = read_ultrasonic();
        
        // IMU lesen
        read_imu(&data.accel, &data.gyro);
        
        // Daten senden
        send_sensor_data(&data);
        
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
```

### Konfiguration
```c
// WiFi-Konfiguration
#define WIFI_SSID "ESP32_Vehicle"
#define WIFI_PASS "vehicle123"

// UART-Konfiguration
#define UART_BAUD_RATE 115200
#define UART_BUFFER_SIZE 1024

// Sensor-Pins
#define ULTRASONIC_TRIG_PIN 18
#define ULTRASONIC_ECHO_PIN 19
#define MOTOR_LEFT_PIN 21
#define MOTOR_RIGHT_PIN 22
```

## 📊 Performance-Optimierung

### Rendering-Optimierung
```cpp
// Frustum Culling
bool is_in_view(Vector2 position, float radius) {
    Camera2D camera = get_camera();
    Rectangle view = get_view_rectangle(camera);
    return CheckCollisionCircleRec(position, radius, view);
}

// Level-of-Detail
int get_detail_level(float distance) {
    if (distance < 100) return 3;      // Hoch
    if (distance < 500) return 2;      // Mittel
    return 1;                          // Niedrig
}
```

### Speicher-Management
```cpp
// Objektpool für häufige Allokationen
template<typename T>
class ObjectPool {
    std::vector<T> pool;
    std::stack<T*> available;
    
public:
    T* acquire() {
        if (available.empty()) {
            pool.emplace_back();
            return &pool.back();
        }
        T* obj = available.top();
        available.pop();
        return obj;
    }
    
    void release(T* obj) {
        available.push(obj);
    }
};
```

## 🧪 Testing

### Unit-Tests
```cpp
// Koordinatenfilter-Test
TEST(CoordinateFilter, BasicFiltering) {
    CoordinateFilter filter;
    Vector2 input = {100.0f, 100.0f};
    Vector2 output = filter.process(input);
    
    EXPECT_NEAR(output.x, input.x, 1.0f);
    EXPECT_NEAR(output.y, input.y, 1.0f);
}

// Physik-Test
TEST(VehiclePhysics, MovementIntegration) {
    VehiclePhysics physics;
    physics.set_velocity({10.0f, 0.0f});
    physics.update(1.0f);
    
    Vector2 pos = physics.get_position();
    EXPECT_NEAR(pos.x, 10.0f, 0.1f);
}
```

### Integration-Tests
```cpp
// UART-Kommunikation testen
TEST(UART, MessageTransmission) {
    uart_message_t msg = {MSG_POSITION, sizeof(position_data_t), &position, 0};
    uart_send_message(&msg);
    
    uart_message_t received;
    ASSERT_TRUE(uart_receive_message(&received, 1000));
    EXPECT_EQ(received.type, MSG_POSITION);
}
```

## 🔍 Debugging

### Logging-System
```cpp
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

#define LOG(level, format, ...) \
    if (level >= CURRENT_LOG_LEVEL) { \
        printf("[%s] " format "\n", level_strings[level], ##__VA_ARGS__); \
    }
```

### Debug-Visualisierung
```cpp
void draw_debug_info() {
    if (debug_mode) {
        // Kollisions-Boxen
        DrawRectangleLines(collision_box.x, collision_box.y, 
                          collision_box.width, collision_box.height, RED);
        
        // Geschwindigkeitsvektoren
        DrawLineV(position, Vector2Add(position, Vector2Scale(velocity, 10)), BLUE);
        
        // Sensor-Reichweite
        DrawCircleLines(sensor_pos.x, sensor_pos.y, sensor_range, GREEN);
    }
}
```

## 📈 Metriken und Monitoring

### Performance-Metriken
```cpp
struct PerformanceMetrics {
    float fps;
    float frame_time;
    float update_time;
    float render_time;
    int message_count;
    float message_latency;
};

void update_metrics() {
    static auto last_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    
    metrics.frame_time = std::chrono::duration<float>(current_time - last_time).count();
    metrics.fps = 1.0f / metrics.frame_time;
    
    last_time = current_time;
}
```

---

**Autor**: Technisches Team TSA24  
**Version**: 1.0.0  
**Letztes Update**: August 2025
