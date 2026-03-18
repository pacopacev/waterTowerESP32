// main/http_server.cpp
#include "http_server.h"
#include "control.h"
#include "esp_log.h"
#include "cJSON.h"
#include <iostream>
#include "webpage.h"

static const char* TAG = "HTTP_SERVER";
static HTTPServer* instance = nullptr;

extern "C" {
    void set_pump(bool on);
    void set_fan(bool on);
}

// GET /api/status - Get current status
esp_err_t HTTPServer::get_status_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "water_level", instance->water_level);
    cJSON_AddBoolToObject(root, "pump_on", instance->pump_status);
    cJSON_AddBoolToObject(root, "fan_on", instance->fan_status);
    cJSON_AddStringToObject(root, "status", "online");
    
    const char *response = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    cJSON_Delete(root);
    free((void*)response);
    return ESP_OK;
}

// POST /api/pump - Control pump (original - keep for backward compatibility)
esp_err_t HTTPServer::post_pump_handler(httpd_req_t *req) {
    char content[100];
    int ret = httpd_req_recv(req, content, sizeof(content));

    if (ret < 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
        return ESP_FAIL;
    }
    
    cJSON *root = cJSON_Parse(content);
    cJSON *action = cJSON_GetObjectItem(root, "action");
    
    if (cJSON_IsString(action)) {
        if (strcmp(action->valuestring, "on") == 0) {
            instance->pump_status = true;
            set_pump(true);  // ✅ Fixed: call global function directly
            ESP_LOGI(TAG, "Pump turned ON");
        } else if (strcmp(action->valuestring, "off") == 0) {
            instance->pump_status = false;
            set_pump(false);  // ✅ Fixed: call global function directly
            ESP_LOGI(TAG, "Pump turned OFF");
        }
    }
    
    cJSON_Delete(root);
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddBoolToObject(response, "pump", instance->pump_status);
    
    const char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    cJSON_Delete(response);
    free((void*)resp_str);
    return ESP_OK;
}

// Fan ON handler
esp_err_t HTTPServer::post_fan_on_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Fan ON request method: %d", req->method);
    
    if (req->method == HTTP_GET) {
        instance->fan_status = true;
        set_fan(true);  // ✅ Fixed: call global function directly
        
        const char* html = "<html><body><h2>Fan turned ON</h2><a href='/'>Back</a></body></html>";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
        
    } else if (req->method == HTTP_POST) {
        char content[200];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
            return ESP_FAIL;
        }
        
        content[ret] = '\0';
        ESP_LOGI(TAG, "Fan ON POST data: %s", content);
        
        cJSON *root = cJSON_Parse(content);
        if (!root) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        cJSON *success = cJSON_GetObjectItem(root, "success");
        bool success_value = true;
        
        if (success && cJSON_IsBool(success)) {
            success_value = cJSON_IsTrue(success);
        }
        
        instance->fan_status = success_value;
        set_fan(success_value);  // ✅ Fixed: call global function directly
        
        cJSON_Delete(root);
        
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddBoolToObject(response, "fan", instance->fan_status);
        
        const char *resp_str = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        
        cJSON_Delete(response);
        free((void*)resp_str);
        
        return ESP_OK;
        
    } else {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
        return ESP_FAIL;
    }
}

// Fan OFF handler
esp_err_t HTTPServer::post_fan_off_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Fan OFF request method: %d", req->method);
    
    if (req->method == HTTP_GET) {
        instance->fan_status = false;
        set_fan(false);  // ✅ Fixed: call global function directly
        
        const char* html = "<html><body><h2>Fan turned OFF</h2><a href='/'>Back</a></body></html>";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
        
    } else if (req->method == HTTP_POST) {
        char content[200];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
            return ESP_FAIL;
        }
        
        content[ret] = '\0';
        ESP_LOGI(TAG, "Fan OFF POST data: %s", content);
        
        cJSON *root = cJSON_Parse(content);
        if (!root) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        cJSON *success = cJSON_GetObjectItem(root, "success");
        bool success_value = false;
        
        if (success && cJSON_IsBool(success)) {
            success_value = cJSON_IsTrue(success);
        }
        
        instance->fan_status = success_value;
        set_fan(success_value);  // ✅ Fixed: call global function directly
        
        cJSON_Delete(root);
        
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddBoolToObject(response, "fan", instance->fan_status);
        
        const char *resp_str = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        
        cJSON_Delete(response);
        free((void*)resp_str);
        
        return ESP_OK;
        
    } else {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
        return ESP_FAIL;
    }
}

// Pump ON handler
// Pump ON handler - FIXED
esp_err_t HTTPServer::post_pump_on_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Pump ON request method: %d", req->method);
    
    if (req->method == HTTP_GET) {
        instance->pump_status = true;     // Update status
        set_pump(true);                    // ✅ Call global function (no instance->)
        
        const char* html = "<html><body><h2>Pump turned ON</h2><a href='/'>Back</a></body></html>";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
        
    } else if (req->method == HTTP_POST) {
        // POST handling code (already correct)...
        char content[200];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
            return ESP_FAIL;
        }
        
        content[ret] = '\0';
        ESP_LOGI(TAG, "Pump ON POST data: %s", content);
        
        cJSON *root = cJSON_Parse(content);
        if (!root) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        cJSON *success = cJSON_GetObjectItem(root, "success");
        bool success_value = true;
        
        if (success && cJSON_IsBool(success)) {
            success_value = cJSON_IsTrue(success);
        }
        
        instance->pump_status = success_value;
        set_pump(success_value);  // ✅ Call global function
        
        cJSON_Delete(root);
        
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddBoolToObject(response, "pump", instance->pump_status);
        
        const char *resp_str = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        
        cJSON_Delete(response);
        free((void*)resp_str);
        
        return ESP_OK;
        
    } else {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
        return ESP_FAIL;
    }
}

// Pump OFF handler
esp_err_t HTTPServer::post_pump_off_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Pump OFF request method: %d", req->method);
    
    if (req->method == HTTP_GET) {
        instance->pump_status = false;
        set_pump(false);  // ✅ Fixed: call global function directly
        
        const char* html = "<html><body><h2>Pump turned OFF</h2><a href='/'>Back</a></body></html>";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, html, strlen(html));
        return ESP_OK;
        
    } else if (req->method == HTTP_POST) {
        char content[200];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data received");
            return ESP_FAIL;
        }
        
        content[ret] = '\0';
        ESP_LOGI(TAG, "Pump OFF POST data: %s", content);
        
        cJSON *root = cJSON_Parse(content);
        if (!root) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }
        
        cJSON *success = cJSON_GetObjectItem(root, "success");
        bool success_value = false;
        
        if (success && cJSON_IsBool(success)) {
            success_value = cJSON_IsTrue(success);
        }
        
        instance->pump_status = success_value;
        set_pump(success_value);  // ✅ Fixed: call global function directly
        
        cJSON_Delete(root);
        
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddBoolToObject(response, "pump", instance->pump_status);
        
        const char *resp_str = cJSON_Print(response);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        
        cJSON_Delete(response);
        free((void*)resp_str);
        
        return ESP_OK;
        
    } else {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
        return ESP_FAIL;
    }
}

// GET handler for pump status
esp_err_t HTTPServer::get_pump_status_handler(httpd_req_t *req) {
    char response[64];
    snprintf(response, sizeof(response), "{\"pump\":\"%s\", \"pump_status\":%s}", 
             instance->pump_status ? "ON" : "OFF",
             instance->pump_status ? "true" : "false");
             
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// GET handler for fan status
esp_err_t HTTPServer::get_fan_status_handler(httpd_req_t *req) {
    char response[64];
    snprintf(response, sizeof(response), "{\"fan\":\"%s\", \"fan_status\":%s}", 
             instance->fan_status ? "ON" : "OFF",
             instance->fan_status ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t HTTPServer::get_water_temperature_status_handler(httpd_req_t *req) {
    char response[128];  // Increased buffer size for float
    
    snprintf(response, sizeof(response), 
             "{\"water_temperature\": %.2f, \"water_temperature_valid\": %s}", 
             instance->water_temperature,  // Store as float, not bool
             instance->water_temperature_valid ? "true" : "false");  // Separate validity flag
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Root handler for control panel
esp_err_t HTTPServer::root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
    return ESP_OK;
}

void HTTPServer::start() {
    std::cout << "Starting HTTP server" << std::endl;
    instance = this;
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 20;
    config.server_port = 80;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Root handler
        httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &root_uri);
        
        // Status handlers
        httpd_uri_t status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = get_status_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &status_uri);
        
        // Pump handlers
        httpd_uri_t pump_uri = { .uri = "/api/pump", .method = HTTP_POST, .handler = post_pump_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_uri);
        
        httpd_uri_t pump_on_get = { .uri = "/api/pump/on", .method = HTTP_GET, .handler = post_pump_on_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_on_get);
        
        httpd_uri_t pump_on_post = { .uri = "/api/pump/on", .method = HTTP_POST, .handler = post_pump_on_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_on_post);
        
        httpd_uri_t pump_off_get = { .uri = "/api/pump/off", .method = HTTP_GET, .handler = post_pump_off_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_off_get);
        
        httpd_uri_t pump_off_post = { .uri = "/api/pump/off", .method = HTTP_POST, .handler = post_pump_off_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_off_post);
        
        httpd_uri_t pump_status_uri = { .uri = "/api/pump/status", .method = HTTP_GET, .handler = get_pump_status_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &pump_status_uri);
        
        // Fan handlers
        httpd_uri_t fan_on_get = { .uri = "/api/fan/on", .method = HTTP_GET, .handler = post_fan_on_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &fan_on_get);
        
        httpd_uri_t fan_on_post = { .uri = "/api/fan/on", .method = HTTP_POST, .handler = post_fan_on_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &fan_on_post);
        
        httpd_uri_t fan_off_get = { .uri = "/api/fan/off", .method = HTTP_GET, .handler = post_fan_off_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &fan_off_get);
        
        httpd_uri_t fan_off_post = { .uri = "/api/fan/off", .method = HTTP_POST, .handler = post_fan_off_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &fan_off_post);
        
        httpd_uri_t fan_status_uri = { .uri = "/api/fan/status", .method = HTTP_GET, .handler = get_fan_status_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &fan_status_uri);

        httpd_uri_t temp_status_uri = { .uri = "/api/temp/status", .method = HTTP_GET, .handler = get_water_temperature_status_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &temp_status_uri);
        
        ESP_LOGI(TAG, "HTTP Server started with pump and fan endpoints");
    }
}