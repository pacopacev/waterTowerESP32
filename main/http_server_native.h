// http_server_native.h
#pragma once

#include <stdbool.h>

// Server functions
void start_native_server(void);
void update_server_status(bool pump, bool fan, float temp, bool temp_valid);

// These variables will be defined by the embedded files
// They are declared as extern here so they can be used in the .cpp file
extern const char index_html_start[];
extern const char index_html_end[];
extern const char login_html_start[];
extern const char login_html_end[];
extern const char login_css_start[];
extern const char login_css_end[];
extern const char login_js_start[];
extern const char login_js_end[];

// Optional: If you have main.css and main.js, add them too
// extern const char main_css_start[];
// extern const char main_css_end[];
// extern const char main_js_start[];
// extern const char main_js_end[];