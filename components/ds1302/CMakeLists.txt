idf_component_register(SRCS "ds1302.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer)