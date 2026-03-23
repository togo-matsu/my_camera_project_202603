idf_component_register(SRCS "camera.c" "http_server.c" "mqtt.c" "sensor.c" "app_main.c" "wifi.c" "main.c"
                        INCLUDE_DIRS "."
                        EMBED_TXTFILES "certs/AmazonRootCA1.pem"
                                       "certs/certificate.pem.crt"
                                       "certs/private.pem.key")