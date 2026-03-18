#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "BUTTON_TEST";

#define BUTTON_PIN_PUMP GPIO_NUM_4  // Change to your button pin
#define DEBOUNCE_TIME_MS 50

extern "C" void app_main(void) {
    // Configure button pin as input ONLY (no internal pull-up)
    gpio_reset_pin(BUTTON_PIN_PUMP);
    gpio_set_direction(BUTTON_PIN_PUMP, GPIO_MODE_INPUT);
    // NO internal pull-up - we're using external 10kΩ resistor
    
    ESP_LOGI(TAG, "Button test started on GPIO%d with EXTERNAL 10kΩ pull-up", BUTTON_PIN_PUMP);
    ESP_LOGI(TAG, "Connect: 3.3V ──── 10kΩ ──── GPIO%d ──── Button ──── GND", BUTTON_PIN_PUMP);
    
    bool stable_state = true;
    bool last_raw_state = true;
    int64_t last_change_time = 0;
    int press_count = 0;
    
    while (1) {
        // Read raw button state
        // With external pull-up: 1 = released, 0 = pressed
        bool raw_state = gpio_get_level(BUTTON_PIN_PUMP);
        int64_t now = esp_timer_get_time() / 1000;
        
        // Show raw reading every second for debugging
        static int64_t last_log = 0;
        if (now - last_log > 1000) {
            ESP_LOGI(TAG, "Raw GPIO%d = %d (%s)", 
                     BUTTON_PIN_PUMP, 
                     raw_state, 
                     raw_state == 0 ? "PRESSED" : "RELEASED");
            last_log = now;
        }
        
        // Debounce logic
        if (raw_state != last_raw_state) {
            last_raw_state = raw_state;
            last_change_time = now;
        }
        
        if ((now - last_change_time) > DEBOUNCE_TIME_MS) {
            if (raw_state != stable_state) {
                stable_state = raw_state;
                
                // if (stable_state == 0) {
                //     press_count++;
                //     ESP_LOGI(TAG, "🔴 Button PRESSED! (Press #%d)", press_count);
                // } else {
                //     ESP_LOGI(TAG, "🟢 Button RELEASED! (Was pressed for %dms)", 
                //              (int)(now - last_change_time));
                // }
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}