/**
 * @file main.cpp
 * @brief ESP-NOW Fahrzeug-Kommunikation für Direction/Speed Control
 * 
 * Dieses Programm empfängt Direction/Speed-Befehle über UART und 
 * sendet diese an alle konfigurierten Fahrzeuge über ESP-NOW.
 * Format: "direction,speed" (z.B. "1,200" oder "5,0" für Stopp)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// MAC address formatting macros
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

// TAG für ESP-LOG
static const char* TAG = "ESP_NOW_DIRECTION_CONTROLLER";

// UART Konfiguration
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024
#define UART_RX_PIN GPIO_NUM_3
#define UART_TX_PIN GPIO_NUM_1

// MAC-Adressen der 4 Fahrzeuge
static uint8_t vehicle_mac_1[] = {0x48, 0xCA, 0x43, 0x2E, 0x34, 0x44}; // Fahrzeug 1 (Test)
static uint8_t vehicle_mac_2[] = {0x74, 0x4D, 0xBD, 0xA1, 0xBF, 0x04}; // Fahrzeug 2 
static uint8_t vehicle_mac_3[] = {0x74, 0x4D, 0xBD, 0xA0, 0x72, 0x1C}; // Fahrzeug 3
static uint8_t vehicle_mac_4[] = {0xDC, 0xDA, 0x0C, 0x20, 0xF2, 0x64}; // Fahrzeug 4

// Struktur für Direction/Speed-Daten
typedef struct {
    int id;         // Fahrzeug-ID
    int direction;  // 1-5 (1:Vor, 2:Zurück, 3:Links, 4:Rechts, 5:Stopp)
    int speed;      // 120-255 (oder 0 für Stopp)
    uint32_t timestamp; // Zeitstempel
} direction_data_t;

// Array der Fahrzeug-MAC-Adressen für einfache Iteration
static uint8_t* vehicle_macs[] = {
    vehicle_mac_1,
    vehicle_mac_2,
    vehicle_mac_3,
    vehicle_mac_4
};

// Anzahl der Fahrzeuge
#define NUM_VEHICLES 4

// Mindestabstand zwischen Nachrichten (ms)
#define MIN_MESSAGE_INTERVAL 100

// Letzte Sendezeit für Rate-Limiting
static uint32_t last_send_time = 0;

// Callback beim Sendeversuch
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    const char* status_str = (status == ESP_NOW_SEND_SUCCESS) ? "✅ Erfolg" : "❌ Fehler";
    ESP_LOGI(TAG, "Send to " MACSTR ": %s", MAC2STR(mac_addr), status_str);
}

// Callback beim Datenempfang
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    ESP_LOGI(TAG, "Received %d bytes from " MACSTR, len, MAC2STR(recv_info->src_addr));
    
    if (len == sizeof(direction_data_t)) {
        direction_data_t* received_cmd = (direction_data_t*)incomingData;
        ESP_LOGI(TAG, "Received command: ID=%d, Direction=%d, Speed=%d, timestamp=%" PRIu32, 
                 received_cmd->id, received_cmd->direction, received_cmd->speed, received_cmd->timestamp);
    }
}

// UART Initialisierung
void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {
            .backup_before_sleep = 0
        }
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "UART initialized for direction/speed commands");
}

// Sende Direction/Speed-Daten an ein spezifisches Fahrzeug
void send_to_vehicle(int direction, int speed, int target_vehicle) {
    // Rate-Limiting: Mindestabstand zwischen Nachrichten
    uint32_t current_time = esp_timer_get_time() / 1000;
    if (current_time - last_send_time < MIN_MESSAGE_INTERVAL) {
        ESP_LOGW(TAG, "⏳ Rate-Limit: Warte %d ms zwischen Nachrichten", MIN_MESSAGE_INTERVAL);
        vTaskDelay((MIN_MESSAGE_INTERVAL - (current_time - last_send_time)) / portTICK_PERIOD_MS);
    }
    
    // Validiere Fahrzeug-ID
    if (target_vehicle < 1 || target_vehicle > NUM_VEHICLES) {
        ESP_LOGE(TAG, "❌ Ungültige Fahrzeug-ID: %d (erlaubt: 1-4)", target_vehicle);
        return;
    }
    
    direction_data_t cmd_data = {
        .id = target_vehicle,
        .direction = direction,
        .speed = speed,
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000) // ms
    };
    
    ESP_LOGI(TAG, "📡 Sende an Fahrzeug %d: Direction=%d, Speed=%d", target_vehicle, direction, speed);
    
    esp_err_t result = esp_now_send(vehicle_macs[target_vehicle - 1], (uint8_t*)&cmd_data, sizeof(direction_data_t));
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "✅ Fahrzeug %d: Befehl gesendet", target_vehicle);
    } else {
        ESP_LOGE(TAG, "❌ Fahrzeug %d: Fehler beim Senden - %s", target_vehicle, esp_err_to_name(result));
    }
    
    last_send_time = esp_timer_get_time() / 1000;
}

// Validiere Direction/Speed-Befehle
bool validate_command(int direction, int speed) {
    // Direction muss zwischen 1 und 5 sein
    if (direction < 1 || direction > 5) {
        ESP_LOGW(TAG, "❌ Ungültige Direction: %d (erlaubt: 1-5)", direction);
        return false;
    }
    
    // Für Stopp (Direction 5) ist Speed egal, ansonsten 120-255
    if (direction == 5) {
        return true; // Stopp ist immer gültig
    }
    
    if (speed < 120 || speed > 255) {
        ESP_LOGW(TAG, "❌ Ungültiger Speed: %d (erlaubt: 120-255 oder Direction=5)", speed);
        return false;
    }
    
    return true;
}

// UART Task - empfängt Direction/Speed-Befehle (Format: "direction,speed,vehicle")
void uart_task(void *pvParameters) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    
    ESP_LOGI(TAG, "🚀 UART task started - warte auf Direction/Speed-Befehle");
    ESP_LOGI(TAG, "📝 Format: 'direction,speed,vehicle'");
    ESP_LOGI(TAG, "📝 Beispiel: '1,200,3' (Fahrzeug 3 vorwärts mit Speed 200)");
    ESP_LOGI(TAG, "📋 Directions: 1=Vor, 2=Zurück, 3=Links, 4=Rechts, 5=Stopp");
    ESP_LOGI(TAG, "⚡ Speed: 120-255 (oder 0 bei Stopp)");
    ESP_LOGI(TAG, "🚗 Vehicle: 1-4 (erforderlich)");
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminieren
            ESP_LOGD(TAG, "UART empfangen: %s", (char*)data);
            
            int direction, speed, target_vehicle;
            
            // Parse Direction,Speed,Vehicle aus UART-Daten
            int parsed = sscanf((char*)data, "%d,%d,%d", &direction, &speed, &target_vehicle);
            
            if (parsed == 3) { // Alle drei Werte erforderlich
                ESP_LOGI(TAG, "📥 Empfangen: Direction=%d, Speed=%d, Vehicle=%d", 
                         direction, speed, target_vehicle);
                
                // Validierung
                if (validate_command(direction, speed)) {
                    // Bei Stopp (Direction 5) setze Speed auf 0
                    if (direction == 5) {
                        speed = 0;
                    }
                    
                    ESP_LOGI(TAG, "✅ Befehl gültig -> sende an Fahrzeug %d", target_vehicle);
                    send_to_vehicle(direction, speed, target_vehicle);
                } else {
                    ESP_LOGW(TAG, "❌ Ungültiger Befehl ignoriert");
                }
            } else {
                ESP_LOGW(TAG, "❌ Parse-Fehler. Erwartetes Format: 'direction,speed,vehicle'");
                ESP_LOGW(TAG, "   Beispiele: '1,200,2' (Fahrzeug 2 vorwärts), '5,0,4' (Fahrzeug 4 stopp)");
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    
    free(data);
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "🚀 Starting ESP-NOW Direction/Speed Controller...");
    
    // NVS Flash initialisieren
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // WLAN im Station-Modus starten (für ESP-NOW benötigt)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "📶 WiFi initialized");

    // ESP-NOW initialisieren
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(OnDataSent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));
    ESP_LOGI(TAG, "📡 ESP-NOW initialized");

    // Alle 4 Fahrzeuge als Peers hinzufügen
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    for (int i = 0; i < NUM_VEHICLES; i++) {
        memcpy(peerInfo.peer_addr, vehicle_macs[i], 6);
        
        if (!esp_now_is_peer_exist(vehicle_macs[i])) {
            esp_err_t result = esp_now_add_peer(&peerInfo);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "✅ Fahrzeug %d als Peer hinzugefügt: " MACSTR, 
                         i + 1, MAC2STR(vehicle_macs[i]));
            } else {
                ESP_LOGE(TAG, "❌ Fehler beim Hinzufügen von Fahrzeug %d: %s", 
                         i + 1, esp_err_to_name(result));
            }
        }
    }

    // UART initialisieren
    init_uart();

    // UART Task starten
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "✅ System initialized. Warte auf Direction/Speed-Befehle über UART...");
    ESP_LOGI(TAG, "🎯 Bereit für Befehle an 4 Fahrzeuge!");
    
    // Hauptschleife - System-Status ausgeben
    while(1) {
        ESP_LOGI(TAG, "🔄 System läuft - bereit für Befehle...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
