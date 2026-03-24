# main/CMakeLists.txt
idf_component_register(SRCS "main.cpp" "http_server_native.cpp"
                    INCLUDE_DIRS "."
                    EMBED_FILES 
                        "index.html"
                        "login.html"
                        "login.css"
                        "login.js"
                    REQUIRES 
                        mdns
                        esp_wifi
                        esp_netif
                        esp_event
                        nvs_flash
                        lwip
                        esp_timer
                        driver
                        esp_http_server
                        http_parser
                        esp_system
                        freertos
                        newlib
                        esp_common
                        esp_rom
                        soc
                        hal
                        json                     # ← Changed from cJSON to json
)