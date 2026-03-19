#include "mdns.h"
#include <iostream>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "control.h"
#include "lwip/netif.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <time.h>
#include "esp_system.h"
#include "http_server_native.h"
#include "mdns.h"









static const char* TAG = "WIFI";

// Replace with your WiFi credentialss
#define WIFI_SSID "ELOPARWFNT"
#define WIFI_PASS "24680135790!!**"

#define BUTTON_PIN_PUMP GPIO_NUM_4
#define BUTTON_PIN_FAN GPIO_NUM_2
#define DEBOUNCE_TIME_MS 50

// Connection status flag
static bool wifi_connected = false;


void check_power_status() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch(reason) {
        case ESP_RST_POWERON:
            ESP_LOGI(TAG, "Just powered ON!");
            break;
        case ESP_RST_SW:
            ESP_LOGI(TAG, "Software reset");
            break;
        case ESP_RST_PANIC:
            ESP_LOGI(TAG, "Panic reset (crash)");
            break;
        default:
            ESP_LOGI(TAG, "Other reset reason: %d", reason);
    }
}

// Call this in app_main() after initialization



// LED Class Definition
class LED {
private:
    const int gpio_pin_wireless;
    const int on_state;
    const int off_state;
    bool current_state;
    
public:
    // Constructor
    LED(int pin_wireless, bool active_high = true) 
        : gpio_pin_wireless(pin_wireless), 
          on_state(active_high ? 1 : 0),
          off_state(active_high ? 0 : 1),
          current_state(false) {
        
        // Initialize GPIO in constructor
        gpio_reset_pin((gpio_num_t)gpio_pin_wireless);

        gpio_set_direction((gpio_num_t)gpio_pin_wireless, GPIO_MODE_OUTPUT);

        gpio_set_level((gpio_num_t)gpio_pin_wireless, off_state);

        ESP_LOGI(TAG, "LED initialized on GPIO %d (active %s)", 
                 gpio_pin_wireless, active_high ? "HIGH" : "LOW");

    
                 
    }
    
    // Turn LED on
    void on() {
        gpio_set_level((gpio_num_t)gpio_pin_wireless, on_state);
        current_state = true;
    }
    
    // Turn LED off
    void off() {
        gpio_set_level((gpio_num_t)gpio_pin_wireless, off_state);
    
        current_state = false;
    }
    
    // Toggle LED
    void toggle() {
        if (current_state) {
            off();
        } else {
            on();
        }
    }
    
    // Get current state
    bool is_on() const {
        return current_state;
    }
};

// Create the LED object (OK at global scope)
LED led(13, true);  // false = active LOW

// LED control task
// LED control task - SIMPLIFIED AND FIXED
// LED control task - GUARANTEED WORKING
static void led_control_task(void *pvParameter) {
    ESP_LOGI(TAG, "LED control task started");
    
    while (1) {
        if (wifi_connected) {
            // CONSTANT ON when connected - FORCE it ON every cycle
            led.on();
            ESP_LOGD(TAG, "LED set ON (connected)");
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Small delay but keep ON
        } else {
            // BLINK when not connected
            led.on();
            ESP_LOGD(TAG, "LED ON (blink)");
            vTaskDelay(500 / portTICK_PERIOD_MS);
            
            led.off();
            ESP_LOGD(TAG, "LED OFF (blink)");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

// WiFi event handler
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data) {
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to WiFi...");
        esp_wifi_connect();
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected! Retrying...");
        wifi_connected = false;
        esp_wifi_connect();
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "==================================");
        ESP_LOGI(TAG, "✅ WIFI CONNECTED SUCCESSFULLY!");
        ESP_LOGI(TAG, "🌐 IP ADDRESS: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "==================================");
        wifi_connected = true;
    }
}

void wifi_init_sta() {
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // 🔧 SET STATIC IP HERE
    esp_netif_dhcpc_stop(sta_netif);  // Stop DHCP client
    
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192,168,1,90);      // Your desired IP
    IP4_ADDR(&ip_info.gw, 192,168,1,1);        // Gateway (your router)
    IP4_ADDR(&ip_info.netmask, 255,255,255,0); // Subnet mask
    
    esp_netif_set_ip_info(sta_netif, &ip_info);
    ESP_LOGI(TAG, "Static IP set to 192.168.1.90");

    // Rest of your WiFi initialization...
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    // Configure WiFi
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete. Connecting to %s with static IP 192.168.1.90...", WIFI_SSID);
}
void start_mdns_service(void) {
    // Initialize mDNS
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS init failed: %d", err);
        return;
    }

        // Set hostname (this will resolve as watertower.local)
    mdns_hostname_set("watertower");
    
    // Set default instance name
    mdns_instance_name_set("Water Tower Control System");

    // Add HTTP service
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
}
static void http_server_task(void *pvParameter) {
    while (!wifi_connected) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    start_native_server();  
    while (1) vTaskDelay(10000 / portTICK_PERIOD_MS);
}



void set_pump(bool on) {
    if (on) {
        gpio_set_level(GPIO_NUM_27, 1);  // ON 
        ESP_LOGI(TAG, "Pump turned ON");
    } else {
        gpio_set_level(GPIO_NUM_27, 0);  // OFF
        ESP_LOGI(TAG, "Pump turned OFF");
    }
}

void set_fan(bool on) {
    if (on) {
        gpio_set_level(GPIO_NUM_26, 1);  // ON
        ESP_LOGI(TAG, "Fan turned ON");
    } else {
        gpio_set_level(GPIO_NUM_26, 0);  // OFF
        ESP_LOGI(TAG, "Fan turned OFF");
    }
}


extern "C" void app_main(void) {


    check_power_status();

    // ✅ 1. FIRST: Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "NVS initialized successfully");


    // Configure button pin as input ONLY (no internal pull-up)
    gpio_reset_pin(BUTTON_PIN_PUMP);
    gpio_reset_pin(BUTTON_PIN_FAN);
    gpio_set_direction(BUTTON_PIN_PUMP, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_PIN_FAN, GPIO_MODE_INPUT);
    // NO internal pull-up - we're using external 10kΩ resistor

    bool stable_state_pump = true;
    bool last_raw_state_pump = true;
    int64_t last_change_time_pump = 0;

    bool stable_state_fan = true;
    bool last_raw_state_fan = true;
    int64_t last_change_time_fan = 0;
    // int press_count = 0;

    // ✅ 2. THEN: Initialize GPIO pins
    gpio_reset_pin(GPIO_NUM_27);//pump
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);

    gpio_reset_pin(GPIO_NUM_26);//fan
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);

    // ✅ 3. THEN: Connect to WiFi
    wifi_init_sta();

    // ✅ 4. THEN: Create tasks
    start_mdns_service(); 
    xTaskCreate(&http_server_task, "http_server_task", 4096, NULL, 5, NULL);
    xTaskCreate(&led_control_task, "led_control_task", 2048, NULL, 5, NULL);

    // Initialize outputs
    gpio_set_level(GPIO_NUM_27, 0);//pump
    gpio_set_level(GPIO_NUM_26, 0);//fan
    ESP_LOGI(TAG, "GPIO outputs initialized");
    
    bool led_initialized = false;
    
    // Main loop
    while (1) {
        // Read raw button state
        // With external pull-up: 1 = released, 0 = pressed
        bool raw_state_pump = gpio_get_level(BUTTON_PIN_PUMP);
        bool raw_state_fan = gpio_get_level(BUTTON_PIN_FAN);
        int64_t now = esp_timer_get_time() / 1000;
        // Show raw reading every second for debugging
        static int64_t last_log_pump = 0;
        static int64_t last_log_fan = 0;  

         
        

        srand(time(NULL));
        float temp = 10.0f + (float)(rand() % 251) / 10.0f;

        // float temp = 66;
        bool valid = (temp > 0 && temp < 20); // Check if reading is reasonable
    
         
    
        update_server_status(!raw_state_pump, !raw_state_fan, temp, valid);

        if (now - last_log_pump > 1000) {
            ESP_LOGI(TAG, "Raw GPIO%d = %d (%s)", 
                     BUTTON_PIN_PUMP, 
                     raw_state_pump, 
                     raw_state_pump == 0 ? "PRESSED" : "RELEASED");
                     
            last_log_pump = now;
        }

        if (now - last_log_fan > 1000) {
            ESP_LOGI(TAG, "Raw GPIO%d = %d (%s)", 
                     BUTTON_PIN_FAN, 
                     raw_state_fan, 
                     raw_state_fan == 0 ? "PRESSED" : "RELEASED");
                     
            last_log_fan = now;
        }
        
        // Debounce logic
        if (raw_state_pump != last_raw_state_pump) {
            last_raw_state_pump = raw_state_pump;
            last_change_time_pump = now;
        }
        
        if ((now - last_change_time_pump) > DEBOUNCE_TIME_MS) {
            if (raw_state_pump != stable_state_pump) {
                stable_state_pump = raw_state_pump;
                
                // if (stable_state_pump == 0) {
                //     press_count++;
                //     ESP_LOGI(TAG, "🔴 Button PRESSED! (Press #%d)", press_count);
                // } else {
                //     ESP_LOGI(TAG, "🟢 Button RELEASED! (Was pressed for %dms)", 
                //              (int)(now - last_change_time_pump));
                // }
            }
        }

        if (raw_state_fan != last_raw_state_fan) {
            last_raw_state_fan = raw_state_fan;
            last_change_time_fan = now;
        }
        
        if ((now - last_change_time_fan) > DEBOUNCE_TIME_MS) {
            if (raw_state_fan != stable_state_fan) {
                stable_state_fan = raw_state_fan;
                
                // if (stable_state_pump == 0) {
                //     press_count++;
                //     ESP_LOGI(TAG, "🔴 Button PRESSED! (Press #%d)", press_count);
                // } else {
                //     ESP_LOGI(TAG, "🟢 Button RELEASED! (Was pressed for %dms)", 
                //              (int)(now - last_change_time_pump));
                // }
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "Water tower running... WiFi: %s, WiFi LED: %s", 
                 wifi_connected ? "CONNECTED ✅" : "DISCONNECTED ❌",
                 led.is_on() ? "ON" : "OFF");
                 
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        
        if (!led_initialized) {
            ESP_LOGI(TAG, "System ready");
            led_initialized = true;
        }
    }
}