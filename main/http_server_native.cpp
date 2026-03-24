// http_server_native.cpp
#include "esp_http_server.h"
#include "esp_log.h"
#include "control.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>

// Forward declarations - add these BEFORE auth_middleware
static esp_err_t pump_status_handler(httpd_req_t *req);
static esp_err_t pump_on_handler(httpd_req_t *req);
static esp_err_t pump_off_handler(httpd_req_t *req);
static esp_err_t fan_status_handler(httpd_req_t *req);
static esp_err_t fan_on_handler(httpd_req_t *req);
static esp_err_t fan_off_handler(httpd_req_t *req);
static esp_err_t temp_status_handler(httpd_req_t *req);
static esp_err_t login_api_handler(httpd_req_t *req);
static esp_err_t login_page_handler(httpd_req_t *req);
static esp_err_t logout_page_handler(httpd_req_t *req);
static esp_err_t login_css_handler(httpd_req_t *req);
static esp_err_t login_js_handler(httpd_req_t *req);
// static esp_err_t dashboard_handler(httpd_req_t *req);
static esp_err_t root_handler(httpd_req_t *req);
static bool check_auth(httpd_req_t *req);

// Credentials
#define AUTH_USERNAME "admin"
#define AUTH_PASSWORD "admin"

static char session_token[64] = {0};
// static bool is_authenticated = false;

// ... rest of your code ...;

// Declare the embedded file
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

// Declare login page and other files (add these to your CMakeLists.txt EMBED_FILES)
extern const char login_html_start[] asm("_binary_login_html_start");
extern const char login_html_end[] asm("_binary_login_html_end");
extern const char login_css_start[] asm("_binary_login_css_start");
extern const char login_css_end[] asm("_binary_login_css_end");
extern const char login_js_start[] asm("_binary_login_js_start");
extern const char login_js_end[] asm("_binary_login_js_end");

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

// ========== AUTHENTICATION MIDDLEWARE ==========

// Check if request is authenticated
static bool check_auth(httpd_req_t *req) {
    char cookie[100];
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie, sizeof(cookie)) == ESP_OK) {
        if (strstr(cookie, session_token) != NULL) {
            return true;
        }
    }
    return false;
}

// Authentication middleware for API endpoints
static esp_err_t auth_middleware(httpd_req_t *req) {
    if (!check_auth(req)) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, "{\"error\":\"Unauthorized\"}", 23);
        return ESP_FAIL;
    }
    
    // Forward to the actual handler based on URI
    if (strcmp(req->uri, "/api/pump/status") == 0) {
        return pump_status_handler(req);
    } else if (strcmp(req->uri, "/api/pump/on") == 0) {
        return pump_on_handler(req);
    } else if (strcmp(req->uri, "/api/pump/off") == 0) {
        return pump_off_handler(req);
    } else if (strcmp(req->uri, "/api/fan/status") == 0) {
        return fan_status_handler(req);
    } else if (strcmp(req->uri, "/api/fan/on") == 0) {
        return fan_on_handler(req);
    } else if (strcmp(req->uri, "/api/fan/off") == 0) {
        return fan_off_handler(req);
    } else if (strcmp(req->uri, "/api/temp/status") == 0) {
        return temp_status_handler(req);
    }
    
    return ESP_FAIL;
}

// ========== API HANDLERS ==========

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

// Fan ON
static esp_err_t fan_on_handler(httpd_req_t *req) {
    g_fan_status = true;
    set_fan(true);
    const char* response = "{\"success\":true,\"fan\":true}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// Fan OFF
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

// ========== AUTHENTICATION HANDLERS ==========

// Login API handler - verifies credentials
static esp_err_t login_api_handler(httpd_req_t *req) {
    char content[200];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    
    content[ret] = '\0';
    ESP_LOGI(TAG, "Login data received: %s", content);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *username = cJSON_GetObjectItem(root, "username");
    cJSON *password = cJSON_GetObjectItem(root, "password");
    
    bool valid = false;
        if (username && cJSON_IsString(username) && 
        password && cJSON_IsString(password)) {
        
        if (strcmp(username->valuestring, AUTH_USERNAME) == 0 &&
            strcmp(password->valuestring, AUTH_PASSWORD) == 0) {
            valid = true;
            // Remove: is_authenticated = true;
            snprintf(session_token, sizeof(session_token), "session_%ld", (long)time(NULL));
        }
    }
    
    cJSON_Delete(root);
    
    // Create clean JSON response
    char response[128];
    if (valid) {
        snprintf(response, sizeof(response), 
                 "{\"success\":true,\"redirect\":\"/index.html\"}");
        
        // Set cookie
        char cookie_header[128];
        snprintf(cookie_header, sizeof(cookie_header), "%s; Path=/", session_token);
        httpd_resp_set_hdr(req, "Set-Cookie", cookie_header);
        
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        ESP_LOGI(TAG, "Login successful, response: %s", response);
    } else {
        snprintf(response, sizeof(response), "{\"success\":false}");
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, response, strlen(response));
        ESP_LOGI(TAG, "Login failed");
    }
    
    return ESP_OK;
}

// Login page handler
static esp_err_t login_page_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, login_html_start, login_html_end - login_html_start);
    return ESP_OK;
}

static esp_err_t logout_page_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Set-Cookie", "session_token=; Max-Age=0; Path=/");
    
    // Return JSON response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"success\":true}", 16);
    return ESP_OK;
}

// CSS handler for login page
static esp_err_t login_css_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, login_css_start, login_css_end - login_css_start);
    return ESP_OK;
}

// JS handler for login page
static esp_err_t login_js_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, login_js_start, login_js_end - login_js_start);
    return ESP_OK;
}

// Root handler - redirects to dashboard if authenticated, otherwise to login
static esp_err_t root_handler(httpd_req_t *req) {
    if (!check_auth(req)) {
        // Not authenticated - redirect to login
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login");
        httpd_resp_send(req, NULL, 0);
        return ESP_FAIL;
    }
    
    // Authenticated - serve index.html
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t index_handler(httpd_req_t *req) {
    if (!check_auth(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login");
        httpd_resp_send(req, NULL, 0);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

// Dashboard handler - serves main control page
// Dashboard handler - serves main control page (index.html)
// static esp_err_t dashboard_handler(httpd_req_t *req) {
//     if (!check_auth(req)) {
//         httpd_resp_set_status(req, "302 Found");
//         httpd_resp_set_hdr(req, "Location", "/login");
//         httpd_resp_send(req, NULL, 0);
//         return ESP_FAIL;
//     }
    
//     httpd_resp_set_type(req, "text/html");
//     httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
//     return ESP_OK;
// }

// ========== SERVER START ==========

void start_native_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 20;
    config.stack_size = 4096;
    config.task_priority = 5;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Protected endpoints (require authentication)
        httpd_uri_t root_uri = { "/", HTTP_GET, root_handler, NULL };
        httpd_register_uri_handler(server, &root_uri);
        
        httpd_uri_t index_uri = { "/index.html", HTTP_GET, index_handler, NULL };
        httpd_register_uri_handler(server, &index_uri);
        
        // Public endpoints
        httpd_uri_t login_page_uri = { "/login", HTTP_GET, login_page_handler, NULL };
        httpd_register_uri_handler(server, &login_page_uri);

        httpd_uri_t logout_page_uri = { "/logout", HTTP_POST, logout_page_handler, NULL };
        httpd_register_uri_handler(server, &logout_page_uri);
        
        httpd_uri_t login_api_uri = { "/api/login", HTTP_POST, login_api_handler, NULL };
        httpd_register_uri_handler(server, &login_api_uri);
        
        httpd_uri_t login_css_uri = { "/login.css", HTTP_GET, login_css_handler, NULL };
        httpd_register_uri_handler(server, &login_css_uri);
        
        httpd_uri_t login_js_uri = { "/login.js", HTTP_GET, login_js_handler, NULL };
        httpd_register_uri_handler(server, &login_js_uri);
        
        // Protected API endpoints
        httpd_uri_t uris[] = {
            {"/api/pump/status", HTTP_GET, auth_middleware, NULL},
            {"/api/pump/on", HTTP_POST, auth_middleware, NULL},
            {"/api/pump/off", HTTP_POST, auth_middleware, NULL},
            {"/api/fan/status", HTTP_GET, auth_middleware, NULL},
            {"/api/fan/on", HTTP_POST, auth_middleware, NULL},
            {"/api/fan/off", HTTP_POST, auth_middleware, NULL},
            {"/api/temp/status", HTTP_GET, auth_middleware, NULL}
        };
        
        for (int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++) {
            httpd_register_uri_handler(server, &uris[i]);
        }
        
        ESP_LOGI(TAG, "HTTP Server started on port 80 with authentication");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}