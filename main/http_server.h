// http_server.h - CORRECT version
#pragma once

#include "esp_http_server.h"

class HTTPServer {
private:
    httpd_handle_t server = NULL;
    int water_level = 0;
    bool pump_status = false;
    bool fan_status = false;

    float water_temperature = 0.0f;        // Store actual temperature
    bool water_temperature_valid = false;  // Flag if reading is valid
    
    static esp_err_t get_status_handler(httpd_req_t *req);
    static esp_err_t post_pump_handler(httpd_req_t *req);
    
    // Pump handlers
    static esp_err_t post_pump_on_handler(httpd_req_t *req);
    static esp_err_t post_pump_off_handler(httpd_req_t *req);

    static esp_err_t get_pump_status_handler(httpd_req_t *req);
    
    // Fan handlers
    static esp_err_t post_fan_on_handler(httpd_req_t *req);
    static esp_err_t post_fan_off_handler(httpd_req_t *req);

    static esp_err_t get_fan_status_handler(httpd_req_t *req);
    
    static esp_err_t root_handler(httpd_req_t *req);


    static esp_err_t get_water_temperature_status_handler(httpd_req_t *req);
    
public:
    void start();
    void stop();
    void update_level(int level) { water_level = level; }
  
    bool get_pump_status() const { return pump_status; }
    bool get_fan_status() const { return fan_status; }

    void set_water_temperature_status(float temp, bool valid) {
        water_temperature = temp;
        water_temperature_valid = valid;
    }

     void set_pump_status(bool status) { 
        pump_status = status; 
    }

    void set_fan_status(bool status) { 
        fan_status = status; 
    }
};