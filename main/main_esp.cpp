/**
 * @file main_esp.cpp
 * @brief ESP-NOW Fahrzeugkommunikation - Vereinfachte Version
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
#include "esp_log.h"
#include "esp_now.h"
#include "nvs_flash.h"

// Logging Tag für Debug-Ausgaben
static const char *TAG = "ESP_NOW_VEHICLE";

//MAC-Adressen der Fahrzeuge
static uint8_t vehicle_mac_1[] = {0x48, 0xCA, 0x43, 0x2E, 0x34, 0x44}; //Erste Mac_Adresse
static uint8_t vehicle_mac_2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Zweite Mac_Adresse
static uint8_t vehicle_mac_3[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Dritte Mac_Adresse
static uint8_t vehicle_mac_4[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Vierte Mac_Adresse

// Broadcast MAC-Adresse für Nachrichten an alle Fahrzeuge
static uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/**
 * @brief Nachrichtenstruktur für Fahrzeugkommunikation
 * 
 * Diese Struktur enthält alle notwendigen Informationen für die
 * Kommunikation zwischen den Fahrzeugen: ID, Richtung, Geschwindigkeit
 * und einen Zeitstempel zur Synchronisation.
 */
struct message {
    uint8_t vehicle_id;        // Fahrzeug-ID (1-4)
    uint8_t direction;         // Richtung (0-3) // 0 = Rechts, 1 = Links, 2 = Vorwärts, 3 = Rückwärts
    uint8_t speed;            // Geschwindigkeit (0-255)
    uint32_t timestamp;        // Zeitstempel der Nachricht
};

/**
 * @brief Callback-Funktion für gesendete ESP-NOW Nachrichten
 * 
 * Diese Funktion wird automatisch aufgerufen, wenn eine ESP-NOW Nachricht
 * gesendet wurde. Sie informiert über Erfolg oder Misserfolg der Übertragung.
 * 
 * @param mac_addr MAC-Adresse des Empfängers
 * @param status Sendestatus (ESP_NOW_SEND_SUCCESS oder ESP_NOW_SEND_FAIL)
 */
static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Nachricht erfolgreich gesendet");
    } else {
        ESP_LOGE(TAG, "Fehler beim Senden der Nachricht");
    }
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
 * @brief Funktion zum Senden von Nachrichten über ESP-NOW
 * 
 * Diese Funktion sendet eine Nachricht als Broadcast an alle Fahrzeuge
 * in Reichweite. Die Nachricht wird an die Broadcast-MAC-Adresse gesendet.
 * 
 * @param msg Zeiger auf die zu sendende Nachricht
 */
void send_message(struct message *msg) {
    // Prüfe, ob die Nachricht gültig ist
    if (msg == NULL) {
        ESP_LOGE(TAG, "Ungültige Nachricht (NULL-Zeiger)");
        return;
    }
    
    // Sende die Nachricht als Broadcast an alle Fahrzeuge
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t *)msg, sizeof(struct message));
    
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Nachricht von Fahrzeug %d wird gesendet:", msg->vehicle_id);
        ESP_LOGI(TAG, "  - Richtung: %d (entspricht %.1f°)", msg->direction, msg->direction * 360.0f / 255.0f);
        ESP_LOGI(TAG, "  - Geschwindigkeit: %d", msg->speed);
    } else {
        ESP_LOGE(TAG, "Fehler beim Senden der Nachricht: %s", esp_err_to_name(result));
    }
}

//Funktion zum Erstellen der Message
/**
 * @brief Funktion zum Erstellen einer Nachricht
 * 
 * Diese Funktion füllt eine Nachrichtenstruktur mit den übergebenen
 * Fahrzeugdaten und fügt einen aktuellen Zeitstempel hinzu.
 * 
 * @param vehicle_id Eindeutige Fahrzeug-ID (1-4)
 * @param direction Fahrtrichtung (0-255, entspricht 0-360°)
 * @param speed Geschwindigkeit (0-255)
 * @param msg Zeiger auf die zu füllende Nachrichtenstruktur
 */
void create_msg(uint8_t vehicle_id, uint8_t direction, uint8_t speed, struct message *msg) {
    // Prüfe, ob die Message-Struktur gültig ist
    if (msg == NULL) {
        ESP_LOGE(TAG, "Ungültige Message-Struktur (NULL-Zeiger)");
        return;
    }
    
    // Prüfe, ob die Fahrzeug-ID im gültigen Bereich liegt
    if (vehicle_id < 1 || vehicle_id > 4) {
        ESP_LOGW(TAG, "Warnung: Fahrzeug-ID %d liegt außerhalb des empfohlenen Bereichs (1-4)", vehicle_id);
    }
    
    // Fülle die Nachrichtenstruktur mit den übergebenen Werten
    msg->vehicle_id = vehicle_id;
    msg->direction = direction;  // 0-255 entspricht 0-360°
    msg->speed = speed;          // 0-255 für Geschwindigkeit
    msg->timestamp = esp_log_timestamp();  // Aktueller Zeitstempel in Millisekunden
    
    ESP_LOGI(TAG, "Nachricht erstellt für Fahrzeug %d: Richtung=%d, Geschwindigkeit=%d", 
             vehicle_id, direction, speed);
}

//Loop zum Senden von Nachrichten
/**
 * @brief Hauptschleife für kontinuierliches Senden von Nachrichten
 * 
 * Diese Funktion simuliert ein Fahrzeug, das regelmäßig seine Position
 * und Geschwindigkeitsdaten an andere Fahrzeuge sendet. In einer echten
 * Anwendung würden hier Sensordaten verwendet.
 */
void message_loop() {
    struct message msg;
    uint8_t my_vehicle_id = 1;  // ID dieses Fahrzeugs (1-4) - ANPASSEN FÜR JEDES FAHRZEUG
    
    ESP_LOGI(TAG, "Starte Nachrichten-Loop für Fahrzeug %d", my_vehicle_id);
    
    // Simulierte Fahrzeugbewegung
    uint8_t direction = 0;   // Startrichtung
    uint8_t speed = 100;     // Konstante Geschwindigkeit
    
    while (1) {
        // Erstelle eine neue Nachricht mit aktuellen Fahrzeugdaten
        create_msg(my_vehicle_id, direction, speed, &msg);
        
        // Sende die Nachricht an alle anderen Fahrzeuge
        send_message(&msg);
        
        // Simuliere Fahrzeugbewegung (Richtungsänderung)
        direction += 1;  // Erhöhe Richtung um 1 (entspricht 90° bei 0-3 Mapping)
        if (direction > 3) {
            direction = 0;  // Beginne wieder von vorne
        }
        
        // Variiere die Geschwindigkeit leicht
        speed = 80 + (esp_log_timestamp() % 40);  // Geschwindigkeit zwischen 80-120

        // Warte 1 Sekunde bis zur nächsten Nachricht
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Hauptfunktion der ESP32-Anwendung
 * 
 * Initialisiert das System und startet die Fahrzeugkommunikation.
 */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== ESP-NOW Fahrzeugkommunikation gestartet ===");
    
    // NVS (Non-Volatile Storage) initialisieren - wird für WiFi benötigt
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialisiert");
    
    // ESP-NOW System initialisieren
    init_esp_now();
    
    // Kurze Wartezeit für vollständige Initialisierung
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "System bereit - starte Fahrzeugkommunikation");
    
    // Starte die Hauptschleife für Nachrichten
    message_loop();
}