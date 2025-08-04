/**
 * @file main_esp.cpp
 * @brief ESP-NOW Fahrzeug-Kommunikation - Vereinfachte Version
 * 
 * Dieses Programm implementiert eine einfache ESP-NOW Kommunikation
 * zwischen mehreren Fahrzeugen mit Richtungs- und Geschwindigkeitsdaten.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "esp_timer.h"

// TAG für ESP-LOG
static const char* TAG = "ESP_NOW_VEHICLE";

// MAC-Adressen der anderen Fahrzeuge (Platzhalter)
static uint8_t vehicle_mac_1[] = {0x48, 0xCA, 0x43, 0x2E, 0x34, 0x44}; //Erste Mac_Adresse
static uint8_t vehicle_mac_2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Zweite Mac_Adresse
static uint8_t vehicle_mac_3[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Dritte Mac_Adresse
static uint8_t vehicle_mac_4[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Vierte Mac_Adresse

// Broadcast MAC-Adresse für das Senden an alle Geräte
static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/**
 * @brief Struktur für Fahrzeugdaten
 * 
 * Diese Struktur enthält alle wichtigen Informationen eines Fahrzeugs,
 * die über ESP-NOW an andere Fahrzeuge gesendet werden.
 */
typedef struct {
    uint8_t vehicle_id;     // Eindeutige Fahrzeug-ID (1-4)
    float speed;            // Geschwindigkeit in km/h
    float direction;        // Fahrtrichtung in Grad (0-359)
    float x_position;       // X-Position auf dem Spielfeld
    float y_position;       // Y-Position auf dem Spielfeld
    uint32_t timestamp;     // Zeitstempel der Daten
} vehicle_data_t;

// Globale Variable für die eigenen Fahrzeugdaten
static vehicle_data_t my_vehicle = {
    .vehicle_id = 1,        // Fahrzeug-ID (sollte für jedes Fahrzeug unterschiedlich sein)
    .speed = 0.0,
    .direction = 0.0,
    .x_position = 0.0,
    .y_position = 0.0,
    .timestamp = 0
};

/**
 * @brief Callback-Funktion für ESP-NOW Sendeereignisse
 * 
 * Diese Funktion wird automatisch aufgerufen, wenn eine ESP-NOW Nachricht
 * gesendet wurde. Sie informiert über Erfolg oder Misserfolg der Übertragung.
 * 
 * @param tx_info Sender-Information (WiFi TX Info)
 * @param status Sendestatus (ESP_NOW_SEND_SUCCESS oder ESP_NOW_SEND_FAIL)
 */
static void on_data_sent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Nachricht erfolgreich gesendet");
    } else {
        ESP_LOGE(TAG, "Fehler beim Senden der Nachricht");
    }
}

/**
 * @brief Callback-Funktion für ESP-NOW Empfangsereignisse
 * 
 * Diese Funktion wird aufgerufen, wenn eine ESP-NOW Nachricht empfangen wird.
 * Sie verarbeitet die empfangenen Fahrzeugdaten von anderen Fahrzeugen.
 * 
 * @param recv_info Empfänger-Informationen
 * @param data Empfangene Daten
 * @param data_len Länge der empfangenen Daten
 */
static void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
    if (data_len != sizeof(vehicle_data_t)) {
        ESP_LOGW(TAG, "Ungültige Datenlänge empfangen: %d", data_len);
        return;
    }
    
    vehicle_data_t *received_data = (vehicle_data_t*)data;
    
    ESP_LOGI(TAG, "Fahrzeugdaten empfangen:");
    ESP_LOGI(TAG, "  Fahrzeug-ID: %d", received_data->vehicle_id);
    ESP_LOGI(TAG, "  Geschwindigkeit: %.2f km/h", received_data->speed);
    ESP_LOGI(TAG, "  Richtung: %.2f°", received_data->direction);
    ESP_LOGI(TAG, "  Position: (%.2f, %.2f)", received_data->x_position, received_data->y_position);
    ESP_LOGI(TAG, "  Zeitstempel: %lu", received_data->timestamp);
    
    // Hier können die empfangenen Daten weiterverarbeitet werden
    // Zum Beispiel für Kollisionserkennung oder Koordination
}

/**
 * @brief Funktion zum Initialisieren von ESP-NOW
 * 
 * Diese Funktion initialisiert das komplette ESP-NOW System:
 * - WiFi im Station-Modus
 * - ESP-NOW Protokoll
 * - Callback-Funktionen
 * - Broadcast-Peer für das Senden an alle Fahrzeuge
 */
void init_esp_now() {
    ESP_LOGI(TAG, "Initialisiere ESP-NOW...");
    
    // WiFi initialisieren (Station-Modus, kein Internet nötig)
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // ESP-NOW initialisieren
    ESP_ERROR_CHECK(esp_now_init());
    
    // Callback-Funktionen registrieren
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
    
    // Broadcast-Peer hinzufügen für das Senden an alle Fahrzeuge
    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, broadcast_mac, 6);
    peer_info.channel = 0;  // Automatische Kanalwahl
    peer_info.encrypt = false;  // Keine Verschlüsselung
    
    esp_err_t result = esp_now_add_peer(&peer_info);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Broadcast-Peer erfolgreich hinzugefügt");
    } else {
        ESP_LOGE(TAG, "Fehler beim Hinzufügen des Broadcast-Peers: %s", esp_err_to_name(result));
    }
    
    ESP_LOGI(TAG, "ESP-NOW erfolgreich initialisiert");
}

/**
 * @brief Funktion zum Senden der eigenen Fahrzeugdaten
 * 
 * Diese Funktion sendet die aktuellen Fahrzeugdaten per ESP-NOW
 * an alle anderen Fahrzeuge im Netzwerk.
 */
void send_vehicle_data() {
    // Aktuelle Zeit als Zeitstempel setzen
    my_vehicle.timestamp = esp_timer_get_time() / 1000; // Millisekunden
    
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t*)&my_vehicle, sizeof(vehicle_data_t));
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Fahrzeugdaten gesendet: ID=%d, Speed=%.2f, Dir=%.2f", 
                 my_vehicle.vehicle_id, my_vehicle.speed, my_vehicle.direction);
    } else {
        ESP_LOGE(TAG, "Fehler beim Senden der Fahrzeugdaten: %s", esp_err_to_name(result));
    }
}

/**
 * @brief Funktion zum Aktualisieren der Fahrzeugdaten
 * 
 * Diese Funktion aktualisiert die lokalen Fahrzeugdaten.
 * In einer echten Anwendung würden diese Daten von Sensoren kommen.
 * 
 * @param speed Neue Geschwindigkeit
 * @param direction Neue Richtung
 * @param x_pos Neue X-Position
 * @param y_pos Neue Y-Position
 */
void update_vehicle_data(float speed, float direction, float x_pos, float y_pos) {
    my_vehicle.speed = speed;
    my_vehicle.direction = direction;
    my_vehicle.x_position = x_pos;
    my_vehicle.y_position = y_pos;
    
    ESP_LOGI(TAG, "Fahrzeugdaten aktualisiert: Speed=%.2f, Dir=%.2f, Pos=(%.2f,%.2f)", 
             speed, direction, x_pos, y_pos);
}

/**
 * @brief Haupttask für die ESP-NOW Kommunikation
 * 
 * Dieser Task läuft kontinuierlich und sendet in regelmäßigen Abständen
 * die Fahrzeugdaten an andere Fahrzeuge.
 */
void esp_now_task(void *parameter) {
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // Senden alle 1000ms
    
    while (1) {
        // Fahrzeugdaten senden
        send_vehicle_data();
        
        // Simuliere veränderte Fahrzeugdaten für Testzwecke
        static float test_speed = 0.0;
        static float test_direction = 0.0;
        
        test_speed += 5.0;
        if (test_speed > 50.0) test_speed = 0.0;
        
        test_direction += 10.0;
        if (test_direction >= 360.0) test_direction = 0.0;
        
        update_vehicle_data(test_speed, test_direction, test_speed * 0.1, test_direction * 0.1);
        
        // Warten bis zum nächsten Sendevorgang
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

/**
 * @brief Hauptfunktion der Anwendung
 * 
 * Diese Funktion wird beim Start des ESP32 aufgerufen und
 * initialisiert das komplette ESP-NOW System.
 */
extern "C" {
    void app_main() {
        ESP_LOGI(TAG, "ESP-NOW Fahrzeug-Kommunikation gestartet");
        
        // ESP-NOW initialisieren
        init_esp_now();
        
        // Task für die ESP-NOW Kommunikation erstellen
        xTaskCreate(esp_now_task, "esp_now_task", 4096, NULL, 5, NULL);
        
        ESP_LOGI(TAG, "Alle Tasks gestartet - System läuft");
    }
}
