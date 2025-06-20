idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(requires esp_event esp_stubs protocol_examples_common nvs_flash)
endif()

# idf_component_register(SRCS "main.c" "decode_jpeg.c" "tcp_client_v4.c" "tcp_client_main.c"
#                                 INCLUDE_DIRS "."
#                                 REQUIRES ${requires})

set(srcs 
    "btd_bandpass.c"
    "btd_exercise2.cpp"
    "btd_config.c"
    "btd_wifi.c"
    "btd_vibration.c"
	)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")