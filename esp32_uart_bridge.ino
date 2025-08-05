/*
 * ESP32 UART zu ESP-NOW Bridge für Heck2-Koordinaten
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
    char vehicle_type[8];  // "HECK2"
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
    
    Serial.println("ESP-NOW initialized, waiting for UART data...");
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("✓ Data sent successfully to test vehicle");
    } else {
        Serial.println("✗ Failed to send data");
    }
}

bool parseCoordinates(String data, coordinate_data_t* coords) {
    // Parse "HECK2:X:110.000000;Y:275.000000;"
    if (data.indexOf("HECK2:") >= 0) {
        int xPos = data.indexOf("X:");
        int yPos = data.indexOf("Y:");
        
        if (xPos >= 0 && yPos >= 0) {
            String xStr = data.substring(xPos + 2, data.indexOf(";", xPos));
            String yStr = data.substring(yPos + 2, data.indexOf(";", yPos));
            
            coords->x = xStr.toFloat();
            coords->y = yStr.toFloat();
            coords->timestamp = millis();
            strcpy(coords->vehicle_type, "HECK2");
            
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
                Serial.printf("Parsed HECK2 coordinates: X=%.2f, Y=%.2f\n", coords.x, coords.y);
                
                // Sende an Testfahrzeug
                esp_err_t result = esp_now_send(testVehicleMAC, (uint8_t*)&coords, sizeof(coords));
                
                if (result == ESP_OK) {
                    Serial.println("→ Forwarding to test vehicle 74:4D:BD:A1:BF:04");
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
