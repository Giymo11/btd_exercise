idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(requires esp_event esp_stubs protocol_examples_common nvs_flash esp_http_server json esp_wifi esp_netif)
endif()

# idf_component_register(SRCS "del_btd_vibration.c" "btd_movement.cpp" "btd_audio.cpp" "btd_button.cpp" "btd_controller.cpp" "btd_battery.cpp" "main.c" "decode_jpeg.c" "tcp_client_v4.c" "tcp_client_main.c"
#                                 INCLUDE_DIRS "."
#                                 REQUIRES ${requires})

set(srcs 
    "btd_bandpass.c"
    "btd_battery.cpp"
    "btd_movement.cpp"
    "btd_audio.cpp"
    "btd_vibrator.cpp"
    "btd_display.cpp"
    "btd_config.c"
    "btd_wifi.c"
    "btd_imu.cpp"
    "btd_button.cpp"
    "btd_controller.cpp"
    "btd_webui.cpp"
    "btd_http.c"
    "btd_qr.cpp"
    "btd_stats.c"
	)

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")