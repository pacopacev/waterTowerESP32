#include "http_server.h"

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




HTTPServer http_server;

static const char* TAG = "WIFI_TEST";

// Replace with your WiFi credentialss
#define WIFI_SSID "ELOPARWFNT"
#define WIFI_PASS "24680135790!!**"



#define SWITCH_PIN GPIO_NUM_15
#define DEBOUNCE_TIME_MS 50

static bool switch_state = false;        // Current stable switch state
static bool last_raw_state = false;      // Last raw reading
static int64_t last_change_time = 0;     // ✅ FIXED: Added missing semicolon



// Connection status flag
static bool wifi_connected = false;

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
LED led(13, false);  // false = active LOW

// LED control task
static void led_control_task(void *pvParameter) {
    bool last_connection_state = false;
    
    while (1) {
        if (wifi_connected) {
            // CONSTANT ON when connected
            if (!last_connection_state) {
                led.on();
                last_connection_state = true;
                ESP_LOGI(TAG, "LED: CONSTANT ON (connected)");
                ESP_LOGI(TAG, "Connected123");
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else {
            // BLINK when not connected
            last_connection_state = false;
            led.on();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            led.off();
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

static void http_server_task(void *pvParameter) {
    // Wait for WiFi to be connected
    while (!wifi_connected) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    ESP_LOGI(TAG, "Starting HTTP server...");
    http_server.start();  // ADD THIS LINE - actually start the server!
    
    // Keep task alive
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

// Define the function OUTSIDE any loop or function
// FIXED versions:
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
// ... (all your code above is fine until app_main)

extern "C" void app_main(void) {

    



    gpio_reset_pin(GPIO_NUM_27);
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);

    gpio_reset_pin(GPIO_NUM_26);
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect to WiFi
    wifi_init_sta();

    // ✅ CREATE TASKS ONCE - outside the loop!
    xTaskCreate(&http_server_task, "http_server_task", 4096, NULL, 5, NULL);
    xTaskCreate(&led_control_task, "led_control_task", 2048, NULL, 5, NULL);

    // Initialize GPIO12 to OFF
    gpio_set_level(GPIO_NUM_27, 0);  // Start OFF Pump
    gpio_set_level(GPIO_NUM_26, 0);  // Start OFF Fan
    ESP_LOGI(TAG, "GPIO12 initialized OFF (will be controlled by HTTP)");
    
    // bool led_initialized = false;
    
    // Main loop

    // Initialize GPIO
    gpio_reset_pin(SWITCH_PIN);                 // Reset pin to default state
gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);  // Set as input
gpio_set_pull_mode(SWITCH_PIN, GPIO_PULLUP_ONLY);


    while (1) {
        // Read raw switch state (active LOW)
    bool raw_state = (gpio_get_level(SWITCH_PIN) == 0);
    int64_t now = esp_timer_get_time() / 1000;
    
    // Detect any change in raw state
    if (raw_state != last_raw_state) {
        last_raw_state = raw_state;
        last_change_time = now;
    }
    
    // If state has been stable for debounce period, update switch_state
    if ((now - last_change_time) > DEBOUNCE_TIME_MS) {
        if (raw_state != switch_state) {
            switch_state = raw_state;
            
            // State changed and is stable
            if (switch_state) {
                ESP_LOGI(TAG, "🔌 Switch HELD - ON");
                set_pump(true);
                led.on();
            } else {
                ESP_LOGI(TAG, "🔌 Switch RELEASED - OFF");
                set_pump(false);
                led.off();
            }
        }
    }
    
    // Log current status (shows correct state even when held)
    ESP_LOGI(TAG, "Switch: %s, Pump: %s", 
             switch_state ? "HELD (ON)" : "RELEASED (OFF)",
             switch_state ? "ON" : "OFF");
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
}