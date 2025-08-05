/**
 * @file main.cpp
 * @brief ESP-NOW Fahrzeug-Kommunikation mit UART-Datenempfang
 * 
 * Dieses Programm implementiert eine ESP-NOW Kommunikation zwischen
 * mehreren Fahrzeugen mit serieller Datenübertragung vom PC.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// TAG für ESP-LOG
static const char* TAG = "ESP_NOW_VEHICLE";

// UART Konfiguration
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024
#define UART_RX_PIN GPIO_NUM_3
#define UART_TX_PIN GPIO_NUM_1

// MAC-Adressen der anderen Fahrzeuge (Platzhalter)
static uint8_t vehicle_mac_1[] = {0x48, 0xCA, 0x43, 0x2E, 0x34, 0x44}; //Erste Mac_Adresse
static uint8_t vehicle_mac_2[] = {0x74, 0x4D, 0xBD, 0xA1, 0xBF, 0x04}; //Zweite Mac_Adresse, Test Fahrzeug
static uint8_t vehicle_mac_3[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Dritte Mac_Adresse
static uint8_t vehicle_mac_4[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Vierte Mac_Adresse

// Struktur für Koordinatendaten
typedef struct {
    float x;
    float y;
    uint32_t timestamp;
} coordinate_data_t;

// Callback beim Sendeversuch
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "Send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback beim Datenempfang
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    ESP_LOGI(TAG, "Received %d bytes from " MACSTR, len, MAC2STR(recv_info->src_addr));
    // Hier können empfangene Daten verarbeitet werden
}

// UART Initialisierung
void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "UART initialized");
}

// UART Daten parsen (Format: "X:12.34;Y:56.78;")
bool parse_coordinates(const char* data, coordinate_data_t* coords) {
    char* x_pos = strstr(data, "X:");
    char* y_pos = strstr(data, "Y:");
    
    if (x_pos && y_pos) {
        coords->x = atof(x_pos + 2);
        coords->y = atof(y_pos + 2);
        coords->timestamp = esp_timer_get_time() / 1000; // ms
        return true;
    }
    return false;
}

// UART Task - läuft kontinuierlich und empfängt serielle Daten
void uart_task(void *pvParameters) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    coordinate_data_t coords;
    
    ESP_LOGI(TAG, "UART task started");
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminieren
            ESP_LOGI(TAG, "Received UART data: %s", (char*)data);
            
            // Koordinaten parsen
            if (parse_coordinates((char*)data, &coords)) {
                ESP_LOGI(TAG, "Parsed coordinates: X=%.2f, Y=%.2f", coords.x, coords.y);
                
                // Koordinaten über ESP-NOW an andere Fahrzeuge senden
                esp_err_t result = esp_now_send(vehicle_mac_2, (uint8_t*)&coords, sizeof(coordinate_data_t));
                if (result == ESP_OK) {
                    ESP_LOGI(TAG, "Coordinates sent via ESP-NOW");
                } else {
                    ESP_LOGE(TAG, "Error sending coordinates: %s", esp_err_to_name(result));
                }
            } else {
                ESP_LOGW(TAG, "Failed to parse coordinates from: %s", (char*)data);
            }
        }
    }
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "Starting ESP-NOW Vehicle Communication...");
    
    // NVS Flash initialisieren
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // WLAN im Station-Modus starten (für ESP-NOW benötigt)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi initialized");

    // ESP-NOW initialisieren
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(OnDataSent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));
    ESP_LOGI(TAG, "ESP-NOW initialized");

    // Peer hinzufügen (andere Fahrzeuge)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, vehicle_mac_2, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(vehicle_mac_2)) {
        ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
        ESP_LOGI(TAG, "Peer added: " MACSTR, MAC2STR(vehicle_mac_2));
    }

    // UART initialisieren
    init_uart();

    // UART Task starten
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "System initialized. Waiting for UART data...");
    
    // Hauptschleife - ESP-NOW Status ausgeben
    while(1) {
        ESP_LOGI(TAG, "System running...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
