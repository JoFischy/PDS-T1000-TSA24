/*
 * ESP32 UART zu ESP-NOW Bridge für ALLE Koordinaten
 * Empfängt UART-Daten und sendet sie über ESP-NOW an Testfahrzeug
 */

#include <esp_now.h>
#include <WiFi.h>

// MAC-Adresse des Testfahrzeugs
uint8_t testVehicleMAC[] = {0x74, 0x4D, 0xBD, 0xA1, 0xBF, 0x04};

// Struktur für Koordinatendaten
typedef struct {
    float x;
    float y;
    uint32_t timestamp;
    char vehicle_type[8];  // "Heck1", "Heck2", "Front", etc.
} coordinate_data_t;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 UART-ESP-NOW Bridge starting...");
    
    // WiFi in Station Mode
    WiFi.mode(WIFI_STA);
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    
    // ESP-NOW initialisieren
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Callback für Sendestatus
    esp_now_register_send_cb(OnDataSent);
    
    // Peer hinzufügen (Testfahrzeug)
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, testVehicleMAC, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
    
    Serial.println("ESP-NOW initialized, waiting for LIVE coordinate data...");
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("✓ Coordinates sent successfully to test vehicle");
    } else {
        Serial.println("✗ Failed to send coordinates");
    }
}

bool parseCoordinates(String data, coordinate_data_t* coords) {
    // Parse neues Format: "COORD:Heck1:269.00,171.00" oder "COORD:Heck2:110.00,275.00"
    if (data.startsWith("COORD:")) {
        int firstColon = data.indexOf(":", 6);  // Nach "COORD:"
        int secondColon = data.indexOf(":", firstColon + 1);
        int comma = data.indexOf(",");
        
        if (firstColon >= 0 && secondColon >= 0 && comma >= 0) {
            // Extrahiere Fahrzeugtyp (z.B. "Heck1", "Heck2")
            String vehicleType = data.substring(6, secondColon);  // Von "COORD:" bis zweiter ":"
            
            // Extrahiere Koordinaten
            String xStr = data.substring(secondColon + 1, comma);
            String yStr = data.substring(comma + 1);
            
            coords->x = xStr.toFloat();
            coords->y = yStr.toFloat();
            coords->timestamp = millis();
            
            // Kopiere Fahrzeugtyp
            vehicleType.toCharArray(coords->vehicle_type, sizeof(coords->vehicle_type));
            
            return true;
        }
    }
    return false;
}

void loop() {
    if (Serial.available()) {
        String receivedData = Serial.readStringUntil('\n');
        receivedData.trim();
        
        if (receivedData.length() > 0) {
            Serial.print("Received: ");
            Serial.println(receivedData);
            
            coordinate_data_t coords;
            if (parseCoordinates(receivedData, &coords)) {
                Serial.printf("Parsed %s coordinates: X=%.2f, Y=%.2f\n", coords.vehicle_type, coords.x, coords.y);
                
                // Sende an Testfahrzeug
                esp_err_t result = esp_now_send(testVehicleMAC, (uint8_t*)&coords, sizeof(coords));
                
                if (result == ESP_OK) {
                    Serial.printf("→ Forwarded %s to test vehicle 74:4D:BD:A1:BF:04\n", coords.vehicle_type);
                } else {
                    Serial.printf("Error sending: %s\n", esp_err_to_name(result));
                }
            } else {
                Serial.println("Failed to parse coordinates");
            }
        }
    }
    
    delay(10);  // Kleine Pause
}
