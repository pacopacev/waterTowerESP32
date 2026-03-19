// http_server_native.cpp
#include "esp_http_server.h"
#include "esp_log.h"
#include "control.h"

// Declare the embedded file
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static const char* TAG = "HTTP_SERVER";
static httpd_handle_t server = NULL;

// Global status variables
static bool g_pump_status = false;
static bool g_fan_status = false;
static float g_water_temp = 0.0f;
static bool g_temp_valid = false;

void update_server_status(bool pump, bool fan, float temp, bool temp_valid) {
    g_pump_status = pump;
    g_fan_status = fan;
    g_water_temp = temp;
    g_temp_valid = temp_valid;
}

// Root handler - serves index.html directly
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

// Pump status
static esp_err_t pump_status_handler(httpd_req_t *req) {
    char response[128];
    snprintf(response, sizeof(response), 
             "{\"pump\":\"%s\", \"pump_status\":%s}",
             g_pump_status ? "ON" : "OFF",
             g_pump_status ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Pump ON
static esp_err_t pump_on_handler(httpd_req_t *req) {
    g_pump_status = true;
    set_pump(true);
    const char* response = "{\"success\":true,\"pump\":true}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Pump OFF
static esp_err_t pump_off_handler(httpd_req_t *req) {
    g_pump_status = false;
    set_pump(false);
    const char* response = "{\"success\":true,\"pump\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Fan status
static esp_err_t fan_status_handler(httpd_req_t *req) {
    char response[128];
    snprintf(response, sizeof(response), 
             "{\"fan\":\"%s\", \"fan_status\":%s}",
             g_fan_status ? "ON" : "OFF",
             g_fan_status ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Fan ON - FIXED
static esp_err_t fan_on_handler(httpd_req_t *req) {
    g_fan_status = true;
    set_fan(true);
    const char* response = "{\"success\":true,\"fan\":true}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Fan OFF - FIXED
static esp_err_t fan_off_handler(httpd_req_t *req) {
    g_fan_status = false;
    set_fan(false);
    const char* response = "{\"success\":true,\"fan\":false}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Temperature status
static esp_err_t temp_status_handler(httpd_req_t *req) {
    char response[128];
    snprintf(response, sizeof(response), 
             "{\"water_temperature\":%.2f,\"water_temperature_valid\":%s}",
             g_water_temp,
             g_temp_valid ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

void start_native_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 10;
    config.stack_size = 4096;
    config.task_priority = 5;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register all endpoints
        httpd_uri_t uris[] = {
            {"/", HTTP_GET, root_handler, NULL},
            {"/api/pump/status", HTTP_GET, pump_status_handler, NULL},
            {"/api/pump/on", HTTP_POST, pump_on_handler, NULL},
            {"/api/pump/off", HTTP_POST, pump_off_handler, NULL},
            {"/api/fan/status", HTTP_GET, fan_status_handler, NULL},
            {"/api/fan/on", HTTP_POST, fan_on_handler, NULL},
            {"/api/fan/off", HTTP_POST, fan_off_handler, NULL},
            {"/api/temp/status", HTTP_GET, temp_status_handler, NULL}
        };
        
        for (int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++) {
            httpd_register_uri_handler(server, &uris[i]);
        }
        
        ESP_LOGI(TAG, "HTTP Server started on port 80");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}