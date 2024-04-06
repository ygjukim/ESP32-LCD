set(srcs "src/Goodix.cpp")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "src"
                       REQUIRES "driver" "esp_timer")
